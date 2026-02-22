#include "periph.h"

#include <driverlib/aon_event.h>
#include <driverlib/aon_ioc.h>
#include <driverlib/aon_rtc.h>
#include <driverlib/aon_wuc.h>
#include <driverlib/aux_wuc.h>
#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/ioc.h>
#include <driverlib/osc.h>
#include <driverlib/prcm.h>
#include <driverlib/pwr_ctrl.h>
#include <driverlib/ssi.h>
#include <driverlib/sys_ctrl.h>
#include <driverlib/uart.h>
#include <driverlib/vims.h>
#include <driverlib/watchdog.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_sysctl.h>
#include <inc/hw_types.h>
#include <stddef.h>
#include <stdio.h>

#define WATCHDOG_RELOAD_MS 2000

/* ================================================================== */
/*  System                                                            */
/* ================================================================== */

void periphInit(void) {
  AONRTCEnable();
  periphReInit();
}

void periphReInit(void) {
  PRCMPowerDomainOn(PRCM_DOMAIN_SERIAL | PRCM_DOMAIN_PERIPH);
  while (PRCMPowerDomainStatus(PRCM_DOMAIN_SERIAL | PRCM_DOMAIN_PERIPH) != PRCM_DOMAIN_POWER_ON);
  PRCMPeripheralRunEnable(PRCM_PERIPH_GPIO);
  PRCMLoadSet();
  while (!PRCMLoadGet());

  WatchdogReloadSet((48000 / 32) * WATCHDOG_RELOAD_MS);
  WatchdogResetEnable();
  WatchdogEnable();

  // disable unused pins
  pinMode(LED1, DISABLED);
  pinMode(LED2, DISABLED);
  pinMode(WAKEUP, DISABLED);
  pinMode(NFC_FD, DISABLED);
  pinMode(NFC_PWR, DISABLED);
  pinMode(NFC_SCL, DISABLED);
  pinMode(NFC_SDA, DISABLED);

  uartInit(115200);
}

void periphDeinit(void) {
  uartDeinit();
}

/*
 * Enter CC26xx standby (deep sleep) with RTC CH0 wakeup.
 * Follows the Contiki-ng LPM deep_sleep() sequence.
 */
void sleepStandby(uint32_t seconds) {
  if (seconds == 0) return;

  // --- Deinit peripherals before sleep ---
  periphDeinit();

  // Disable watchdog — it doesn't run during standby
  WatchdogResetDisable();

  // --- Configure RTC channel 0 as wakeup source ---
  uint32_t now = AONRTCCurrentCompareValueGet();
  AONRTCCompareValueSet(AON_RTC_CH0, now + (seconds << 16));
  AONRTCEventClear(AON_RTC_CH0);
  AONRTCChannelEnable(AON_RTC_CH0);
  AONRTCCombinedEventConfig(AON_RTC_CH0);

  // --- Route RTC CH0 to MCU wakeup controller ---
  AONEventMcuWakeUpSet(AON_EVENT_MCU_WU0, AON_EVENT_RTC_CH0);

  // --- Enable RTC combined interrupt in NVIC (needed for WFI wakeup) ---
  IntEnable(INT_AON_RTC_COMB);

  // --- Disable global interrupts so we run wakeup code before any ISR ---
  IntMasterDisable();

  // --- Freeze IOs on the MCU–AON boundary ---
  AONIOCFreezeEnable();

  // --- Power off SERIAL and PERIPH domains ---
  PRCMPowerDomainOff(PRCM_DOMAIN_SERIAL | PRCM_DOMAIN_PERIPH);

  // --- Switch HF clock to RCOSC (requires AUX OSCCTRL clock) ---
  //     Must power up AUX and enable OSC clocks first.
  AONWUCAuxWakeupEvent(AONWUC_AUX_WAKEUP);
  while (!(AONWUCPowerStatusGet() & AONWUC_AUX_POWER_ON));
  AUXWUCClockEnable(AUX_WUC_OSCCTRL_CLOCK | AUX_WUC_SMPH_CLOCK);
  while (AUXWUCClockStatus(AUX_WUC_OSCCTRL_CLOCK | AUX_WUC_SMPH_CLOCK) != AUX_WUC_CLOCK_READY);
  OSCHF_SwitchToRcOscTurnOffXosc();

  // --- Power down AUX domain fully ---
  AONWUCAuxPowerDownConfig(AONWUC_NO_CLOCK);
  AONWUCAuxSRamConfig(0);
  AONWUCAuxWakeupEvent(AONWUC_AUX_ALLOW_SLEEP);
  AUXWUCPowerCtrl(AUX_WUC_POWER_OFF);
  while (AONWUCPowerStatusGet() & AONWUC_AUX_POWER_ON);

  // --- Configure MCU power-down: no clock ---
  AONWUCMcuPowerDownConfig(AONWUC_NO_CLOCK);

  // --- Full SRAM retention ---
  AONWUCMcuSRamConfig(MCU_RAM0_RETENTION | MCU_RAM1_RETENTION | MCU_RAM2_RETENTION | MCU_RAM3_RETENTION);

  // --- Power off RFCORE, CPU, VIMS and SYSBUS ---
  PRCMPowerDomainOff(PRCM_DOMAIN_RFCORE | PRCM_DOMAIN_CPU | PRCM_DOMAIN_VIMS | PRCM_DOMAIN_SYSBUS);

  // --- Request uLDO during standby ---
  PRCMMcuUldoConfigure(true);

  // --- Disable JTAG domain ---
  AONWUCJtagPowerOff();

  // --- Allow MCU and AUX domain power-down ---
  AONWUCDomainPowerDownEnable();

  // --- Configure recharge controller ---
  SysCtrlSetRechargeBeforePowerDown(XOSC_IN_HIGH_POWER_MODE);

  // --- Request uLDO as power source during standby ---
  PowerCtrlSourceSet(PWRCTRL_PWRSRC_ULDO);

  // --- Sync AON writes ---
  SysCtrlAonSync();

  // --- Turn off VIMS last so we can use cache as long as possible ---
  PRCMCacheRetentionDisable();
  VIMSModeSet(VIMS_BASE, VIMS_MODE_OFF);

  // --- Enter deep sleep (WFI) ---
  PRCMDeepSleep();

  // ============ CPU resumes here after RTC wakeup ============

  // --- Sync and adjust recharge ---
  SysCtrlAonSync();
  SysCtrlAdjustRechargeAfterPowerDown(0);

  // --- Release uLDO request (HW auto-switches to GLDO/DCDC) ---
  PRCMMcuUldoConfigure(false);

  // --- Re-enable VIMS cache ---
  VIMSModeSet(VIMS_BASE, VIMS_MODE_ENABLED);
  PRCMCacheRetentionEnable();

  // --- Unfreeze IOs ---
  AONIOCFreezeDisable();
  SysCtrlAonSync();

  // --- Check operating conditions, optimally choose DCDC versus GLDO ---
  SysCtrl_DCDC_VoltageConditionalControl();

  // --- Power up AUX so oscillator functions work (e.g. XOSC_HF for radio) ---
  AONWUCAuxWakeupEvent(AONWUC_AUX_WAKEUP);
  while (!(AONWUCPowerStatusGet() & AONWUC_AUX_POWER_ON));

  // --- Re-enable AUX clocks (OSCCTRL + SMPH) needed for DDI/oscillator access ---
  AUXWUCClockEnable(AUX_WUC_OSCCTRL_CLOCK | AUX_WUC_SMPH_CLOCK);
  while (AUXWUCClockStatus(AUX_WUC_OSCCTRL_CLOCK | AUX_WUC_SMPH_CLOCK) != AUX_WUC_CLOCK_READY);

  // --- Re-enable power domains and peripherals ---
  periphReInit();

  // --- Clear RTC wakeup event and disable NVIC interrupt ---
  AONRTCEventClear(AON_RTC_CH0);
  AONRTCChannelDisable(AON_RTC_CH0);
  IntPendClear(INT_AON_RTC_COMB);
  IntDisable(INT_AON_RTC_COMB);

  // --- Re-enable global interrupts ---
  IntMasterEnable();
}

uint32_t millis(void) { return ((AONRTCCurrent64BitValueGet() * 1000) >> 32); }

void yield(void) { WatchdogIntClear(); }

/* RTC-based delay – accurate regardless of CPU clock */
void delay(uint32_t ms) {
  uint32_t start = millis();
  while ((millis() - start) < ms) yield();
}

/* ================================================================== */
/*  GPIO helpers                                                      */
/* ================================================================== */

void pinMode(uint32_t ioid, int mode) {
  if (ioid == IOID_UNUSED) return;

  switch (mode) {
    case INPUT:
      IOCPinTypeGpioInput(ioid);
      IOCIOPortPullSet(ioid, IOC_NO_IOPULL);
      break;
    case INPUT_PULLUP:
      IOCPinTypeGpioInput(ioid);
      IOCIOPortPullSet(ioid, IOC_IOPULL_UP);
      break;
    case OUTPUT:
      IOCPinTypeGpioOutput(ioid);
      break;
    case DISABLED:
    default:
      GPIO_clearDio(ioid);
      IOCPinTypeGpioOutput(ioid);
      break;
  }
}

void digitalWrite(uint32_t ioid, int value) {
  if (ioid == IOID_UNUSED) return;
  if (value)
    GPIO_setDio(ioid);
  else
    GPIO_clearDio(ioid);
}

int digitalRead(uint32_t ioid) {
  if (ioid == IOID_UNUSED) return 0;
  return GPIO_readDio(ioid);
}

/* ================================================================== */
/*  SPI (SSI0)                                                        */
/* ================================================================== */

#define SPI_BASE SSI0_BASE

bool spiOpen(SpiDevice* dev) {
  if (!dev) return false;

  PRCMPeripheralRunEnable(PRCM_PERIPH_SSI0);
  PRCMLoadSet();
  while (!PRCMLoadGet());

  SSIConfigSetExpClk(SPI_BASE, GET_MCU_CLOCK, SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, dev->bitRate, 8);
  IOCPinTypeSsiMaster(SPI_BASE, dev->pinMiso, dev->pinMosi, IOID_UNUSED, dev->pinClk);
  SSIEnable(SPI_BASE);

  uint32_t dummy;
  while (SSIDataGetNonBlocking(SPI_BASE, &dummy));

  if (dev->pinCs != IOID_UNUSED) {
    pinMode(dev->pinCs, OUTPUT);
    digitalWrite(dev->pinCs, HIGH);
  }
  return true;
}

bool spiClose(SpiDevice* dev) {
  if (!dev) return false;

  if (dev->pinCs != IOID_UNUSED) {
    pinMode(dev->pinCs, OUTPUT);
    digitalWrite(dev->pinCs, HIGH);
  }

  while (SSIBusy(SPI_BASE));
  SSIDisable(SPI_BASE);

  PRCMPeripheralRunDisable(PRCM_PERIPH_SSI0);
  PRCMLoadSet();
  while (!PRCMLoadGet());

  pinMode(dev->pinMosi, DISABLED);
  pinMode(dev->pinMiso, DISABLED);
  pinMode(dev->pinClk, DISABLED);

  return true;
}

void spiSelect(SpiDevice* dev) {
  if (dev && dev->pinCs != IOID_UNUSED) digitalWrite(dev->pinCs, LOW);
}

void spiDeselect(SpiDevice* dev) {
  if (dev && dev->pinCs != IOID_UNUSED) digitalWrite(dev->pinCs, HIGH);
}

bool spiTransfer(SpiDevice* dev, const uint8_t* wbuf, int wlen, uint8_t* rbuf, int rlen, int skipLen) {
  if (!dev) return false;

  int totalLen = wlen + rlen + skipLen;
  int readIdx = 0;
  for (int i = 0; i < totalLen; i++) {
    uint32_t txByte = (i < wlen && wbuf != NULL) ? wbuf[i] : 0x00;
    SSIDataPut(SPI_BASE, txByte);
    uint32_t rxByte;
    SSIDataGet(SPI_BASE, &rxByte);
    if (i >= wlen + skipLen && readIdx < rlen && rbuf != NULL) rbuf[readIdx++] = (uint8_t)rxByte;
  }
  return true;
}

bool spiWrite(SpiDevice* dev, const uint8_t* buf, int len) { return spiTransfer(dev, buf, len, NULL, 0, 0); }

bool spiRead(SpiDevice* dev, uint8_t* buf, int len) { return spiTransfer(dev, NULL, 0, buf, len, 0); }

/* ================================================================== */
/*  UART (UART0)                                                      */
/* ================================================================== */

#ifdef UART_DEBUG

#define UART_BASE UART0_BASE

void uartInit(uint32_t baudRate) {
  PRCMPeripheralRunEnable(PRCM_PERIPH_UART0);
  PRCMLoadSet();
  while (!PRCMLoadGet());

  IOCPinTypeUart(UART_BASE, UART_RX, UART_TX, IOID_UNUSED, IOID_UNUSED);
  UARTConfigSetExpClk(UART_BASE, GET_MCU_CLOCK, baudRate,
                      UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
  UARTEnable(UART_BASE);

  setvbuf(stdout, NULL, _IONBF, 0);
}

void uartDeinit(void) {
  while (UARTBusy(UART_BASE));
  UARTDisable(UART_BASE);

  PRCMPeripheralRunDisable(PRCM_PERIPH_UART0);
  PRCMLoadSet();
  while (!PRCMLoadGet());

  // Drive TX HIGH (UART idle/mark) so that AONIOCFreezeEnable() freezes the
  // line in its correct idle state. Driving it LOW would create a BREAK
  // condition during standby, corrupting the first byte after wakeup.
  GPIO_setDio(UART_TX);
  IOCPinTypeGpioOutput(UART_TX);
  pinMode(UART_RX, DISABLED);
}

void uartPutChar(uint8_t c) { UARTCharPut(UART_BASE, c); }

void uartPuts(const char* s) {
  while (*s) {
    uartPutChar((uint8_t)*s++);
  }
}

/* newlib _write() — routes printf to UART0 */
int _write(int fd, char* buf, int len) {
  (void)fd;
  for (int i = 0; i < len; i++) {
    uartPutChar((uint8_t)buf[i]);
  }
  return len;
}

#else

/* When UART is disabled, _write discards all output */
int _write(int fd, char* buf, int len) {
  (void)fd;
  (void)buf;
  return len;
}

#endif
