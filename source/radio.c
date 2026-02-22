
#include "radio.h"

#include <driverlib/chipinfo.h>
#include <driverlib/interrupt.h>
#include <driverlib/osc.h>
#include <driverlib/prcm.h>
#include <driverlib/rf_common_cmd.h>
#include <driverlib/rf_data_entry.h>
#include <driverlib/rf_ieee_cmd.h>
#include <driverlib/rf_ieee_mailbox.h>
#include <driverlib/rf_mailbox.h>
#include <driverlib/rfc.h>
#include <inc/hw_aon_rtc.h>
#include <inc/hw_ccfg.h>
#include <inc/hw_fcfg1.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_prcm.h>
#include <inc/hw_rfc_dbell.h>
#include <inc/hw_rfc_pwr.h>
#include <inc/hw_types.h>
#include <stdio.h>
#include <string.h>

#include "oepl-proto.h"

/* ----------------------------- Constants --------------------------------- */

#define MAC_ADDR_LENGTH 8
#define UNKNOWN_EUI64_BYTE 0xFF

/* Receive buffer size: must hold max 802.15.4 frame + metadata */
#define RX_BUF_SIZE 144

/* Length field size in data entry (1 byte) */
#define DATA_ENTRY_LENSZ_BYTE 1

/* IEEE 802.15.4 constants */
#define IEEE802154_MAC_MIN_BE 1
#define IEEE802154_MAC_MAX_BE 5
#define IEEE802154_MAC_MAX_CSMA_BACKOFFS 4

/* TX power lookup table (values from SmartRF Studio) */
typedef struct {
  int dbm;
  uint16_t value;
} output_config_t;

static const output_config_t rgOutputPower[] = {
    {5, 0x9330},  {4, 0x9324},  {3, 0x5a1c},   {2, 0x4e18},   {1, 0x4214},   {0, 0x3161},   {-3, 0x2558},
    {-6, 0x1d52}, {-9, 0x194e}, {-12, 0x144b}, {-15, 0x0ccb}, {-18, 0x0cc9}, {-21, 0x0cc7},
};
#define OUTPUT_CONFIG_COUNT (sizeof(rgOutputPower) / sizeof(rgOutputPower[0]))

/* ----------------------------- Radio state ------------------------------- */

typedef enum {
  RADIO_STATE_DISABLED = 0,
  RADIO_STATE_SLEEP,
  RADIO_STATE_RECEIVE,
} RadioState;

static volatile RadioState sState = RADIO_STATE_DISABLED;

/* ----------------------------- IEEE overrides ----------------------------- */

/* Overrides for IEEE 802.15.4, differential mode */
static uint32_t sIEEEOverrides[] = {
    0x00354038, /* Synth: Set RTRIM (POTAILRESTRIM) to 5 */
    0x4001402D, /* Synth: Correct CKVD latency setting (address) */
    0x00608402, /* Synth: Correct CKVD latency setting (value) */
    0x000784A3, /* Synth: Set FREF = 3.43 MHz (24 MHz / 7) */
    0xA47E0583, /* Synth: Set loop bandwidth after lock to 80 kHz (K2) */
    0xEAE00603, /* Synth: Set loop bandwidth after lock to 80 kHz (K3, LSB) */
    0x00010623, /* Synth: Set loop bandwidth after lock to 80 kHz (K3, MSB) */
    0x002B50DC, /* Adjust AGC DC filter */
    0x05000243, /* Increase synth programming timeout */
    0x002082C3, /* Increase synth programming timeout */
    0xFFFFFFFF, /* End of override list */
};

/* ----------------------------- RAT offset -------------------------------- */

/*
 * Offset of the radio timer from the RTC.
 * Preserved across RF core power cycles.
 */
static uint32_t sRatOffset = 0;

/* ----------------------------- RF command structures ---------------------- */

static volatile __attribute__((aligned(4))) rfc_CMD_SYNC_START_RAT_t sStartRatCmd;
static volatile __attribute__((aligned(4))) rfc_CMD_RADIO_SETUP_t sRadioSetupCmd;
static volatile __attribute__((aligned(4))) rfc_CMD_FS_POWERDOWN_t sFsPowerdownCmd;
static volatile __attribute__((aligned(4))) rfc_CMD_SYNC_STOP_RAT_t sStopRatCmd;
static volatile __attribute__((aligned(4))) rfc_CMD_CLEAR_RX_t sClearReceiveQueueCmd;
static volatile __attribute__((aligned(4))) rfc_CMD_IEEE_RX_t sReceiveCmd;
static volatile __attribute__((aligned(4))) rfc_CMD_IEEE_CSMA_t sCsmacaBackoffCmd;
static volatile __attribute__((aligned(4))) rfc_CMD_IEEE_TX_t sTransmitCmd;

/* RX statistics output */
static __attribute__((aligned(4))) rfc_ieeeRxOutput_t sRfStats;

/* Two receive buffer entries with room for 1 max IEEE802.15.4 frame each */
static uint8_t sRxBuf0[RX_BUF_SIZE] __attribute__((aligned(4)));
static uint8_t sRxBuf1[RX_BUF_SIZE] __attribute__((aligned(4)));

/* The RX data queue */
static dataQueue_t sRxDataQueue = {0};

/* ----------------------------- Exported globals -------------------------- */

uint8_t channelList[6] = {11, 15, 20, 25, 26, 27};

uint8_t mLastLqi = 0;
uint8_t mLastRSSI = 0;

/* ====================== Internal RF core helpers ========================= */

/**
 * Initialize the RX/TX buffers and set up the circular RX data queue.
 */
static void rfCoreInitBufs(void) {
  rfc_dataEntry_t* entry;

  memset(sRxBuf0, 0x00, RX_BUF_SIZE);
  memset(sRxBuf1, 0x00, RX_BUF_SIZE);

  entry = (rfc_dataEntry_t*)sRxBuf0;
  entry->pNextEntry = sRxBuf1;
  entry->config.lenSz = DATA_ENTRY_LENSZ_BYTE;
  entry->length = sizeof(sRxBuf0) - sizeof(rfc_dataEntry_t);

  entry = (rfc_dataEntry_t*)sRxBuf1;
  entry->pNextEntry = sRxBuf0;
  entry->config.lenSz = DATA_ENTRY_LENSZ_BYTE;
  entry->length = sizeof(sRxBuf1) - sizeof(rfc_dataEntry_t);
}

/**
 * Initialize the IEEE RX command with default parameters.
 *
 * Configures frame filtering off (promiscuous-like), auto-ACK enabled,
 * accept all frame types, CCA options, and appends RSSI + correlation.
 */
static void rfCoreInitReceiveParams(void) {
  static const rfc_CMD_IEEE_RX_t cReceiveCmd = {
      .commandNo = CMD_IEEE_RX,
      .status = IDLE,
      .pNextOp = NULL,
      .startTime = 0u,
      .startTrigger = {.triggerType = TRIG_NOW},
      .condition = {.rule = COND_NEVER},
      .channel = 11, /* default channel, overridden by radioEnable */
      .rxConfig =
          {
              .bAutoFlushCrc = 1,
              .bAutoFlushIgn = 0,
              .bIncludePhyHdr = 0,
              .bIncludeCrc = 0,
              .bAppendRssi = 1,
              .bAppendCorrCrc = 1,
              .bAppendSrcInd = 0,
              .bAppendTimestamp = 0,
          },
      .frameFiltOpt =
          {
              .frameFiltEn = 0, /* no filtering: we handle matching in SW */
              .frameFiltStop = 1,
              .autoAckEn = 0, /* no auto-ACK: protocol handles responses */
              .slottedAckEn = 0,
              .autoPendEn = 0,
              .defaultPend = 0,
              .bPendDataReqOnly = 0,
              .bPanCoord = 0,
              .maxFrameVersion = 3,
              .bStrictLenFilter = 1,
          },
      .frameTypes =
          {
              .bAcceptFt0Beacon = 1,
              .bAcceptFt1Data = 1,
              .bAcceptFt2Ack = 1,
              .bAcceptFt3MacCmd = 1,
              .bAcceptFt4Reserved = 1,
              .bAcceptFt5Reserved = 1,
              .bAcceptFt6Reserved = 1,
              .bAcceptFt7Reserved = 1,
          },
      .ccaOpt =
          {
              .ccaEnEnergy = 1,
              .ccaEnCorr = 1,
              .ccaEnSync = 1,
              .ccaCorrOp = 1,
              .ccaSyncOp = 0,
              .ccaCorrThr = 3,
          },
      .ccaRssiThr = -90,
      .endTrigger = {.triggerType = TRIG_NEVER},
      .endTime = 0u,
  };

  sReceiveCmd = cReceiveCmd;
  sReceiveCmd.pRxQ = &sRxDataQueue;
  sReceiveCmd.pOutput = &sRfStats;
  sReceiveCmd.localPanID = PROTO_PAN_ID;
}

/** Send direct abort command to the RF core. */
static uint_fast8_t rfCoreExecuteAbortCmd(void) { return (RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_ABORT)) & 0xFF); }

/** Send direct ping command to verify the RF core is alive. */
static uint_fast8_t rfCoreExecutePingCmd(void) { return (RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_PING)) & 0xFF); }

/** Send immediate command to clear (reset) the RX queue. */
static uint_fast8_t rfCoreClearReceiveQueue(dataQueue_t* aQueue) {
  sClearReceiveQueueCmd.commandNo = CMD_CLEAR_RX;
  sClearReceiveQueueCmd.pQueue = aQueue;
  return (RFCDoorbellSendTo((uint32_t)&sClearReceiveQueueCmd) & 0xFF);
}

/** Set the RF core mode for IEEE 802.15.4 based on chip type. */
static void rfCoreSetModeSelect(void) {
  switch (ChipInfo_GetChipType()) {
    case CHIP_TYPE_CC2650:
      HWREG(PRCM_BASE + PRCM_O_RFCMODESEL) = PRCM_RFCMODESEL_CURR_MODE5;
      break;
    case CHIP_TYPE_CC2630:
      HWREG(PRCM_BASE + PRCM_O_RFCMODESEL) = PRCM_RFCMODESEL_CURR_MODE2;
      break;
    default:
      break;
  }
}

/**
 * Power on the RF core.
 *
 * Switches to XOSC HF, enables the RF power domain, sets mode to IEEE,
 * initializes RX buffers, boots the CPE, and verifies it with a ping.
 *
 * @return CMDSTA_Done on success.
 */
static uint_fast8_t rfCorePowerOn(void) {
  bool interruptsWereDisabled;

  /* Request XOSC HF for radio operation */
  if (OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_XOSC_HF) {
    OSCClockSourceSet(OSC_SRC_CLK_MF | OSC_SRC_CLK_HF, OSC_XOSC_HF);
  }

  rfCoreSetModeSelect();

  /* Set up RX data queue as circular, no last entry */
  sRxDataQueue.pCurrEntry = sRxBuf0;
  sRxDataQueue.pLastEntry = NULL;
  rfCoreInitBufs();

  /* Wait for XOSC HF to be ready */
  if (OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_XOSC_HF) {
    OSCHfSourceSwitch();
  }

  interruptsWereDisabled = IntMasterDisable();

  /* Enable RF core power domain */
  PRCMPowerDomainOn(PRCM_DOMAIN_RFCORE);
  while (PRCMPowerDomainStatus(PRCM_DOMAIN_RFCORE) != PRCM_DOMAIN_POWER_ON);

  PRCMDomainEnable(PRCM_DOMAIN_RFCORE);
  PRCMLoadSet();
  while (!PRCMLoadGet());

  /* Disable RF core interrupts – we poll status registers directly */
  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = 0x0;
  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x0;

  if (!interruptsWereDisabled) IntMasterEnable();

  /* Enable CPE clocks */
  HWREG(RFC_PWR_NONBUF_BASE + RFC_PWR_O_PWMCLKEN) =
      (RFC_PWR_PWMCLKEN_RFC_M | RFC_PWR_PWMCLKEN_CPE_M | RFC_PWR_PWMCLKEN_CPERAM_M);

  /* Verify RF core is alive */
  return rfCoreExecutePingCmd();
}

/**
 * Power off the RF core.
 *
 * Tears down interrupts, disables the power domain, and switches
 * the HF clock back to RCOSC for lower power.
 */
static void rfCorePowerOff(void) {
  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = 0x0;
  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x0;

  PRCMDomainDisable(PRCM_DOMAIN_RFCORE);
  PRCMLoadSet();
  while (!PRCMLoadGet());

  PRCMPowerDomainOff(PRCM_DOMAIN_RFCORE);
  while (PRCMPowerDomainStatus(PRCM_DOMAIN_RFCORE) != PRCM_DOMAIN_POWER_OFF);

  /* Switch back to RCOSC for low power */
  if (OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_RCOSC_HF) {
    OSCClockSourceSet(OSC_SRC_CLK_MF | OSC_SRC_CLK_HF, OSC_RCOSC_HF);
    OSCHfSourceSwitch();
  }
}

/**
 * Send the RAT start + radio setup command chain.
 *
 * Starts the RAT timer and configures the radio for IEEE 802.15.4 mode.
 *
 * @return Command status (DONE_OK on success).
 */
static uint_fast16_t rfCoreSendEnableCmd(void) {
  uint8_t doorbellRet;
  bool interruptsWereDisabled;
  uint_fast16_t ret;

  static const rfc_CMD_SYNC_START_RAT_t cStartRatCmd = {
      .commandNo = CMD_SYNC_START_RAT,
      .startTrigger = {.triggerType = TRIG_NOW},
      .condition = {.rule = COND_STOP_ON_FALSE},
  };
  static const rfc_CMD_RADIO_SETUP_t cRadioSetupCmd = {
      .commandNo = CMD_RADIO_SETUP,
      .startTrigger = {.triggerType = TRIG_NOW},
      .condition = {.rule = COND_NEVER},
      .mode = 1, /* IEEE 802.15.4 */
  };

  /* Enable RTC -> RAT clock line */
  HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) = 1;

  sStartRatCmd = cStartRatCmd;
  sStartRatCmd.pNextOp = (rfc_radioOp_t*)&sRadioSetupCmd;
  sStartRatCmd.rat0 = sRatOffset;

  sRadioSetupCmd = cRadioSetupCmd;
  sRadioSetupCmd.txPower = rgOutputPower[0].value; /* max TX power */
  sRadioSetupCmd.pRegOverride = sIEEEOverrides;

  interruptsWereDisabled = IntMasterDisable();

  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = ~IRQ_LAST_COMMAND_DONE;

  doorbellRet = (RFCDoorbellSendTo((uint32_t)&sStartRatCmd) & 0xFF);
  if (doorbellRet != CMDSTA_Done) {
    ret = doorbellRet;
    goto exit;
  }

  /* Wait for the command chain to complete (with timeout) */
  {
    uint32_t timeout = 300000;
    while ((HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_LAST_COMMAND_DONE) == 0x00 && timeout > 0) {
      timeout--;
    }
    if (timeout == 0) {
      ret = 0xFFFF;
      goto exit;
    }
  }

  ret = sRadioSetupCmd.status;

exit:
  if (!interruptsWereDisabled) IntMasterEnable();
  return ret;
}

/**
 * Send the FS powerdown + RAT stop command chain.
 *
 * Saves the RAT offset for next power-on and shuts down the synthesizer.
 *
 * @return Command status (DONE_OK on success).
 */
static uint_fast16_t rfCoreSendDisableCmd(void) {
  uint8_t doorbellRet;
  bool interruptsWereDisabled;
  uint_fast16_t ret;

  static const rfc_CMD_FS_POWERDOWN_t cFsPowerdownCmd = {
      .commandNo = CMD_FS_POWERDOWN,
      .startTrigger = {.triggerType = TRIG_NOW},
      .condition = {.rule = COND_ALWAYS},
  };
  static const rfc_CMD_SYNC_STOP_RAT_t cStopRatCmd = {
      .commandNo = CMD_SYNC_STOP_RAT,
      .startTrigger = {.triggerType = TRIG_NOW},
      .condition = {.rule = COND_NEVER},
  };

  HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) = 1;

  sFsPowerdownCmd = cFsPowerdownCmd;
  sFsPowerdownCmd.pNextOp = (rfc_radioOp_t*)&sStopRatCmd;

  sStopRatCmd = cStopRatCmd;

  interruptsWereDisabled = IntMasterDisable();

  HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = ~IRQ_LAST_COMMAND_DONE;

  doorbellRet = (RFCDoorbellSendTo((uint32_t)&sFsPowerdownCmd) & 0xFF);
  if (doorbellRet != CMDSTA_Done) {
    ret = doorbellRet;
    goto exit;
  }

  /* Wait for the command chain to complete (with timeout) */
  {
    uint32_t timeout = 300000;
    while ((HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_LAST_COMMAND_DONE) == 0x00 && timeout > 0) {
      timeout--;
    }
  }

  ret = sStopRatCmd.status;
  if (sStopRatCmd.status == DONE_OK) {
    sRatOffset = sStopRatCmd.rat0;
  }

exit:
  if (!interruptsWereDisabled) IntMasterEnable();
  return ret;
}

/** Start the persistent IEEE RX command. */
static uint_fast8_t rfCoreSendReceiveCmd(void) {
  sReceiveCmd.status = IDLE;
  return (RFCDoorbellSendTo((uint32_t)&sReceiveCmd) & 0xFF);
}

/**
 * Transmit a packet using CSMA-CA + TX command chain.
 *
 * @param aPsdu  Pointer to the PSDU (frame data, without CRC).
 * @param aLen   Length of the PSDU in bytes.
 * @return CMDSTA_Done on success.
 */
static uint_fast8_t rfCoreSendTransmitCmd(uint8_t* aPsdu, uint8_t aLen) {
  static const rfc_CMD_IEEE_CSMA_t cCsmacaBackoffCmd = {
      .commandNo = CMD_IEEE_CSMA,
      .status = IDLE,
      .startTrigger = {.triggerType = TRIG_NOW},
      .condition = {.rule = COND_ALWAYS},
      .macMaxBE = IEEE802154_MAC_MAX_BE,
      .macMaxCSMABackoffs = IEEE802154_MAC_MAX_CSMA_BACKOFFS,
      .csmaConfig = {.initCW = 1, .bSlotted = 0, .rxOffMode = 0},
      .NB = 0,
      .BE = IEEE802154_MAC_MIN_BE,
      .remainingPeriods = 0,
      .endTrigger = {.triggerType = TRIG_NEVER},
      .endTime = 0x00000000,
  };
  static const rfc_CMD_IEEE_TX_t cTransmitCmd = {
      .commandNo = CMD_IEEE_TX,
      .status = IDLE,
      .startTrigger = {.triggerType = TRIG_NOW},
      .condition = {.rule = COND_NEVER},
      .pNextOp = NULL,
  };

  sCsmacaBackoffCmd = cCsmacaBackoffCmd;
  sCsmacaBackoffCmd.randomState = (uint16_t)(HWREG(AON_RTC_BASE + AON_RTC_O_SEC) & 0xFFFF);
  sCsmacaBackoffCmd.pNextOp = (rfc_radioOp_t*)&sTransmitCmd;

  sTransmitCmd = cTransmitCmd;
  sTransmitCmd.payloadLen = aLen;
  sTransmitCmd.pPayload = aPsdu;

  return (RFCDoorbellSendTo((uint32_t)&sCsmacaBackoffCmd) & 0xFF);
}

/**
 * Read the IEEE MAC address from CCFG or FCFG.
 *
 * Reads the EUI-64 stored in the chip configuration area.
 * Falls back to the factory configuration if the customer configuration
 * is not programmed (all 0xFF).
 *
 * @param[out] mac  Buffer to receive the 8-byte MAC address (little-endian).
 */
static void rfCoreReadMacAddress(uint8_t* mac) {
  uint8_t* eui64;
  unsigned int i;

  /* Try the customer configuration first */
  eui64 = (uint8_t*)(CCFG_BASE + CCFG_O_IEEE_MAC_0);

  for (i = 0; i < MAC_ADDR_LENGTH; i++) {
    if (eui64[i] != UNKNOWN_EUI64_BYTE) break;
  }

  if (i >= MAC_ADDR_LENGTH) {
    /* CCFG is all 0xFF, fall back to factory configuration */
    eui64 = (uint8_t*)(FCFG1_BASE + FCFG1_O_MAC_15_4_0);
  }

  /*
   * The MAC address in the chip is stored in big-endian (network order).
   * Convert to little-endian for the protocol stack.
   */
  for (i = 0; i < MAC_ADDR_LENGTH; i++) {
    mac[i] = eui64[(MAC_ADDR_LENGTH - 1) - i];
  }
}

/* ======================= Public API implementation ======================= */

/**
 * Initialize the radio hardware and read the MAC address.
 *
 * Sets up internal data structures but does not power on the RF core yet.
 * Call radioEnable() to start receiving.
 *
 * @param[out] mac  Buffer to receive the 8-byte device MAC address.
 * @return true on success.
 */
bool radioInit(uint8_t* mac) {
  rfCoreInitReceiveParams();
  sState = RADIO_STATE_DISABLED;

  /* Read MAC address from chip */
  rfCoreReadMacAddress(mac);

  return true;
}

/**
 * Enable the radio and start receiving on the given channel.
 *
 * Powers on the RF core (if not already on), sets up IEEE 802.15.4 mode,
 * and begins listening on the specified channel.
 *
 * @param channel  IEEE 802.15.4 channel number (11-26).
 * @return true on success, false on RF core failure.
 */
bool radioEnable(uint_fast8_t channel) {
  if (sState == RADIO_STATE_RECEIVE) {
    /* Already receiving: switch channel if needed */
    return radioSetChannel(channel);
  }

  if (sState == RADIO_STATE_DISABLED) {
    /* Cold start: power on the RF core */
    if (rfCorePowerOn() != CMDSTA_Done) {
      rfCorePowerOff();
      return false;
    }
    if (rfCoreSendEnableCmd() != DONE_OK) {
      rfCorePowerOff();
      sState = RADIO_STATE_DISABLED;
      return false;
    }
    sState = RADIO_STATE_SLEEP;
  }

  /* Start RX on the requested channel */
  sReceiveCmd.channel = channel;
  if (rfCoreSendReceiveCmd() != CMDSTA_Done) return false;

  sState = RADIO_STATE_RECEIVE;
  return true;
}

/**
 * Switch channel while the radio is already receiving.
 *
 * Lightweight channel change: aborts the running RX command, clears the
 * receive queue, and restarts RX on the new channel without powering down
 * the RF core or re-running the setup command chain.
 *
 * The radio must already be in RECEIVE state (i.e. radioEnable() was called).
 *
 * @param channel  IEEE 802.15.4 channel number (11-26).
 * @return true on success, false if not receiving or RF core error.
 */
bool radioSetChannel(uint_fast8_t channel) {
  if (sState != RADIO_STATE_RECEIVE) return false;

  if (sReceiveCmd.status == ACTIVE && sReceiveCmd.channel == channel) return true; /* already on this channel */

  if (rfCoreExecuteAbortCmd() != CMDSTA_Done) return false;
  if (rfCoreClearReceiveQueue(&sRxDataQueue) != CMDSTA_Done) return false;

  sReceiveCmd.channel = channel;
  return (rfCoreSendReceiveCmd() == CMDSTA_Done);
}

/**
 * Disable the radio and power off the RF core.
 *
 * Stops any running RX command, shuts down the synthesizer and RAT timer,
 * and powers off the RF core to save power.
 *
 * @return true on success.
 */
bool radioDisable(void) {
  if (sState == RADIO_STATE_DISABLED) return true;

  if (sState == RADIO_STATE_RECEIVE) {
    rfCoreExecuteAbortCmd();
    sState = RADIO_STATE_SLEEP;
  }

  if (sState == RADIO_STATE_SLEEP) {
    rfCoreSendDisableCmd();
    rfCorePowerOff();
    sState = RADIO_STATE_DISABLED;
  }

  return true;
}

/**
 * Poll the receive queue for an incoming packet.
 *
 * Non-blocking: checks the RX data queue for a finished entry and copies
 * the frame data into the caller's buffer. Also updates mLastRSSI and
 * mLastLqi globals.
 *
 * @param[out] dstBuf  Buffer to receive the raw 802.15.4 frame (without CRC).
 * @param      maxLen  Maximum number of bytes to copy.
 * @return Number of frame bytes copied, or 0 if no packet available.
 */
int32_t radioRx(uint8_t* dstBuf, uint32_t maxLen) {
  rfc_dataEntryGeneral_t *curEntry, *startEntry;
  rfc_ieeeRxCorrCrc_t* crcCorr;
  int32_t frameLen = 0;

  if (sState != RADIO_STATE_RECEIVE) return 0;

  startEntry = (rfc_dataEntryGeneral_t*)sRxDataQueue.pCurrEntry;
  curEntry = startEntry;

  do {
    if (curEntry->status == DATA_ENTRY_FINISHED) {
      uint8_t* payload = &(curEntry->data);
      uint8_t len = payload[0]; /* length of data following this byte */

      /* Last 2 appended bytes are RSSI + CorrCrc */
      crcCorr = (rfc_ieeeRxCorrCrc_t*)&payload[len];
      uint8_t rssi = payload[len - 1];

      if (crcCorr->status.bCrcErr == 0 && len > 2) {
        /*
         * Frame data is at payload[1] through payload[len-2].
         * payload[len-1] = RSSI (appended), payload[len] = CorrCrc (appended).
         * Actual frame length = len - 2 (subtract RSSI + CorrCrc bytes).
         */
        uint8_t actualFrameLen = len - 2;

        if (actualFrameLen <= maxLen) {
          memcpy(dstBuf, &payload[1], actualFrameLen);
          frameLen = actualFrameLen;
          mLastRSSI = rssi;
          mLastLqi = crcCorr->status.corr;
        }
      }

      curEntry->status = DATA_ENTRY_PENDING;
      break;
    } else if (curEntry->status == DATA_ENTRY_UNFINISHED) {
      curEntry->status = DATA_ENTRY_PENDING;
    }

    curEntry = (rfc_dataEntryGeneral_t*)(curEntry->pNextEntry);
  } while (curEntry != startEntry);

  return frameLen;
}

/**
 * Transmit a packet.
 *
 * The packet format is: pkt[0] = total length (including 2-byte HW CRC),
 * followed by the frame data starting at pkt[1]. CSMA-CA + TX are submitted
 * as foreground commands while CMD_IEEE_RX is running in the background.
 * The RF core needs the active receiver for CCA (Clear Channel Assessment).
 * After TX completes the background RX automatically resumes.
 *
 * @param pkt  Packet buffer (pkt[0] = length, pkt[1..] = frame data).
 * @return true on successful transmission, false on failure.
 */
bool radioTx(uint8_t* pkt) {
  uint8_t totalLen = pkt[0];
  uint8_t* psdu = &pkt[1];

  if (sState != RADIO_STATE_RECEIVE) return false;

  /*
   * totalLen includes the 2-byte CRC that hardware generates.
   * Subtract 2 for the payload length passed to the TX command.
   */
  uint8_t payloadLen = totalLen - 2;
  if (payloadLen < 1) return false;

  /*
   * Submit CSMA-CA + TX as foreground commands while RX is active.
   * CMD_IEEE_CSMA requires the receiver running for CCA to work.
   */
  if (rfCoreSendTransmitCmd(psdu, payloadLen) != CMDSTA_Done) return false;

  /* Wait for the foreground command chain to complete (with timeout) */
  uint32_t timeout = 300000;
  while ((sTransmitCmd.status == IDLE || sTransmitCmd.status == ACTIVE) && timeout > 0) {
    timeout--;
  }
  while (sCsmacaBackoffCmd.status == ACTIVE && timeout > 0) {
    timeout--;
  }

  if (timeout == 0) {
    /* Timed out – abort any pending foreground commands */
    rfCoreExecuteAbortCmd();
    rfCoreClearReceiveQueue(&sRxDataQueue);
    rfCoreSendReceiveCmd();
    return false;
  }

  return (sTransmitCmd.status == IEEE_DONE_OK);
}
