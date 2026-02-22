#include "powermgt.h"

#include <driverlib/aon_batmon.h>
#include <stdio.h>

#include "oepl-definitions.h"

// clang-format off
uint16_t dataReqAttemptArr[POWER_SAVING_SMOOTHING] = {0};  // Holds the amount of attempts required per data_req/check-in
uint8_t dataReqAttemptArrayIndex = 0;
uint8_t dataReqLastAttempt = 0;
uint16_t nextCheckInFromAP = 0;
uint8_t wakeUpReason = WAKEUP_REASON_FIRSTBOOT;
uint8_t scanAttempts = 0;

int8_t temperature = 0;
uint16_t batteryVoltage = 0;
uint16_t longDataReqCounter = 0;
uint16_t voltageCheckCounter = 0;

uint8_t capabilities = 0;
// clang-format on

void initPowerSaving(const uint16_t initialValue) {
  for (uint8_t c = 0; c < POWER_SAVING_SMOOTHING; c++) {
    dataReqAttemptArr[c] = initialValue;
  }
}

uint32_t getNextScanSleep(const bool increment) {
  if (increment) {
    if (scanAttempts < 255) scanAttempts++;
  }

  if (scanAttempts < INTERVAL_1_ATTEMPTS) {
    return INTERVAL_1_TIME;
  } else if (scanAttempts < (INTERVAL_1_ATTEMPTS + INTERVAL_2_ATTEMPTS)) {
    return INTERVAL_2_TIME;
  } else {
    return INTERVAL_3_TIME;
  }
}

void addAverageValue(void) {
  uint16_t curval = INTERVAL_AT_MAX_ATTEMPTS - INTERVAL_BASE;
  curval *= dataReqLastAttempt;
  curval /= DATA_REQ_MAX_ATTEMPTS;
  curval += INTERVAL_BASE;
  dataReqAttemptArr[dataReqAttemptArrayIndex % POWER_SAVING_SMOOTHING] = curval;
  dataReqAttemptArrayIndex++;
}

uint16_t getNextSleep(void) {
  uint16_t avg = 0;
  for (uint8_t c = 0; c < POWER_SAVING_SMOOTHING; c++) {
    avg += dataReqAttemptArr[c];
  }
  avg /= POWER_SAVING_SMOOTHING;
  return avg;
}

void battery_update(void) {
  AONBatMonEnable();
  while (!AONBatMonNewTempMeasureReady());
  temperature = (int8_t)AONBatMonTemperatureGetDegC();
  while (!AONBatMonNewBatteryMeasureReady());
  uint32_t batVal = AONBatMonBatteryVoltageGet();
  batteryVoltage = ((batVal >> 8) & 0xFF) * 1000 + (batVal & 0xFF) * 1000 / 256;
  AONBatMonDisable();

  printf(">> battery: %d mV, temperature: %d C\n", batteryVoltage, temperature);
}