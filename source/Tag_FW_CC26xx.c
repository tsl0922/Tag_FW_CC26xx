#include <driverlib/sys_ctrl.h>
#include <stdio.h>
#include <string.h>

#include "epd.h"
#include "periph.h"
#include "powermgt.h"
#include "radio.h"
#include "syncedproto.h"

enum { TAG_MODE_CHANSEARCH = 0, TAG_MODE_ASSOCIATED = 1 };

uint8_t currentTagMode = TAG_MODE_CHANSEARCH;

uint8_t channelSelect(void) {
  uint8_t result[sizeof(channelList)];
  memset(result, 0, sizeof(result));
  radioEnable(0);
  for (uint8_t c = 0; c < sizeof(channelList); c++) {
    uint8_t channel = channelList[c];
    printf(">> Detect channel: %d\n", channel);
    radioSetChannel(channel);
    if (detectAP(channel)) {
      if (mLastLqi > result[c]) result[c] = mLastLqi;
      printf("Channel: %d - LQI: %d RSSI %d\r\n", channel, mLastLqi, mLastRSSI);
    }
  }
  radioDisable();

  uint8_t highestLqi = 0;
  uint8_t highestSlot = 0;
  for (uint8_t c = 0; c < sizeof(result); c++) {
    if (result[c] > highestLqi) {
      highestSlot = channelList[c];
      highestLqi = result[c];
    }
  }
  mLastLqi = highestLqi;
  return highestSlot;
}

void executeCommand(uint8_t cmd) {
  printf("execute command: %d\r\n", cmd);
  switch (cmd) {
    case CMD_DO_REBOOT:
      SysCtrlSystemReset();
      break;
    case CMD_DO_SCAN:
      currentChannel = channelSelect();
      break;
    case CMD_ERASE_EEPROM_IMAGES:
      eraseImageBlocks();
      break;
    default:
      printf("skip command: %d\r\n", cmd);
      break;
  }
}

bool displayCustomImage(uint8_t imagetype) {
  uint8_t slot = findSlotDataTypeArg(imagetype << 3);
  if (slot != 0xFF) {
    uint8_t lut = getEepromImageDataArgument(slot) & 0x03;
    drawImageFromEeprom(slot, lut);
    return true;
  }
  return false;
}

uint16_t tagAssociated(void) {
  radioEnable(currentChannel);
  struct AvailDataInfo* avail;
  // Is there any reason why we should do a long (full) get data request
  // (including reason, status)?
  if ((longDataReqCounter > LONG_DATAREQ_INTERVAL) || wakeUpReason != WAKEUP_REASON_TIMED) {
    // check if we should do a voltage measurement (those are pretty expensive)
    if (voltageCheckCounter == VOLTAGE_CHECK_INTERVAL) {
      voltageCheckCounter = 0;
      battery_update();
    }
    voltageCheckCounter++;

    avail = getAvailDataInfo();
    if (avail != NULL) {
      // we got some data!
      longDataReqCounter = 0;
      wakeUpReason = WAKEUP_REASON_TIMED;
    }
  } else {
    avail = getShortAvailDataInfo();
  }

  addAverageValue();

  if (avail == NULL) {
    // no data :( this means no reply from AP
    printf("No data\r\n");
    nextCheckInFromAP = 0;  // let the power-saving algorithm determine the
                            // next sleep period
  } else {
    nextCheckInFromAP = avail->nextCheckIn;
    // got some data from the AP!
    if (avail->dataType != DATATYPE_NOUPDATE) {
      // data transfer
      printf("Got data transfer\r\n");
      if (processAvailDataInfo(avail)) {
        // succesful transfer, next wake time is determined by the
        // Next Checkin;
      } else {
        // failed transfer, let the algorithm determine next sleep interval
        // (not the AP)
        nextCheckInFromAP = 0;
      }
    } else {
      // no data transfer, just sleep.
      printf("No data transfer\r\n");
    }
  }

  uint16_t nextCheckin = getNextSleep();
  longDataReqCounter += nextCheckin;
  if (nextCheckin == INTERVAL_AT_MAX_ATTEMPTS) {
    // disconnected, obviously...
    currentChannel = 0;
    currentTagMode = TAG_MODE_CHANSEARCH;
  }
  radioDisable();

  if (nextCheckInFromAP & 0x7FFF) {
    // if the AP told us to sleep for a specific period, do so.
    if (nextCheckInFromAP & 0x8000) {
      return (nextCheckInFromAP & 0x7FFF);
    } else {
      return nextCheckInFromAP * 60;
    }
  } else {
    // sleep determined by algorithm
    return getNextSleep();
  }
}

uint16_t tagChanSearch(void) {
  if (((scanAttempts != 0) && (scanAttempts % VOLTAGEREADING_DURING_SCAN_INTERVAL == 0)) ||
      (scanAttempts > (INTERVAL_1_ATTEMPTS + INTERVAL_2_ATTEMPTS))) {
    battery_update();
  }

  currentChannel = channelSelect();

  // did we find a working channel?
  if (currentChannel) {
    // now associated!
    printf("AP Found\r\n");
#ifdef TAG_DEBUG
    if (curImgSlot == 0xFF) {
      if ((scanAttempts >= (INTERVAL_1_ATTEMPTS + INTERVAL_2_ATTEMPTS - 1))) {
        if (!displayCustomImage(CUSTOM_IMAGE_LONGTERMSLEEP)) showLongTermSleep();
      } else {
        if (!displayCustomImage(CUSTOM_IMAGE_APFOUND)) showAPFound();
      }
    }
#endif
    scanAttempts = 0;
    wakeUpReason = WAKEUP_REASON_NETWORK_SCAN;
    initPowerSaving(INTERVAL_BASE);
    currentTagMode = TAG_MODE_ASSOCIATED;
    return getNextSleep();
  } else {
    printf("No AP found\r\n");
#ifdef TAG_DEBUG
    if (curImgSlot == 0xFF) {
      if (!displayCustomImage(CUSTOM_IMAGE_NOAPFOUND)) showNoAP();
    }
#endif
    // still not associated
    return getNextScanSleep(true);
  }
}

uint16_t tagWorker(void) {
  switch (currentTagMode) {
    case TAG_MODE_ASSOCIATED:
      return tagAssociated();
      break;
    case TAG_MODE_CHANSEARCH:
      return tagChanSearch();
      break;
    default:
      return 40;
  }
}

int main(void) {
  periphInit();

  printf("==============================\n");
  printf("Tag Firmware - CC26xx\n");
  printf("==============================\n");
  printf(">> Starting up...\n");

  printf("epd init\n");
  epdInit();
  epdDeinit();

  printf("radio init\n");
  if (!radioInit(mSelfMac)) {
    printf("radio init failed\n");
    while (1);
  }

  battery_update();

  initializeProto();

#ifdef TAG_DEBUG
  if (!displayCustomImage(CUSTOM_IMAGE_SPLASHSCREEN)) showSplashScreen();
#endif

  currentChannel = channelSelect();
  if (currentChannel) {
    printf("AP Found\r\n");
#ifdef TAG_DEBUG
    if (!displayCustomImage(CUSTOM_IMAGE_APFOUND)) showAPFound();
#endif
    initPowerSaving(INTERVAL_BASE);
    currentTagMode = TAG_MODE_ASSOCIATED;
  } else {
    printf("No AP found\r\n");
#ifdef TAG_DEBUG
    if (!displayCustomImage(CUSTOM_IMAGE_NOAPFOUND)) showNoAP();
#endif
    initPowerSaving(INTERVAL_AT_MAX_ATTEMPTS);
    currentTagMode = TAG_MODE_CHANSEARCH;
  }

  // display last image if available
  if (curImgSlot != 0xFF) {
    uint8_t lut = getEepromImageDataArgument(curImgSlot) & 0x03;
    drawImageFromEeprom(curImgSlot, lut);
  } else {
    clearScreen();
  }

  int count = 0;
  while (1) {
    printf(">> Working.. %d\r\n", count++);
    uint32_t sleepTime = tagWorker();
    printf("Sleeping %lu seconds\r\n", sleepTime);
    sleepStandby(sleepTime);
  }
}
