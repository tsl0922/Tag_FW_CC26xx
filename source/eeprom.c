#include "eeprom.h"

#include <stdint.h>
#include <stdio.h>

#include "periph.h"

/* ---- SPI NOR flash commands ---- */
#define CMD_WREN 0x06 /* Write Enable */
#define CMD_RDSR 0x05 /* Read Status Register */
#define CMD_READ 0x03 /* Read Data */
#define CMD_PP 0x02   /* Page Program (up to 256 bytes) */
#define CMD_SE 0x20   /* Sector Erase  (4 KB) */
#define CMD_DP 0xB9   /* Deep Power-Down */
#define CMD_RDP 0xAB  /* Release from Deep Power-Down */
#define CMD_RDID 0x9F /* Read JEDEC ID */

/* Status register bits */
#define SR_WIP 0x01 /* Write-In-Progress */

/* SPI device context for the external flash */
static SpiDevice sFlashDev = {
    .pinMosi = FLASH_MOSI,
    .pinMiso = FLASH_MISO,
    .pinClk = FLASH_CLK,
    .pinCs = FLASH_CS,
    .bitRate = 4000000,
};

/* ---- low-level helpers ---- */

static void flashWriteEnable(void) {
  uint8_t cmd = CMD_WREN;
  spiSelect(&sFlashDev);
  spiWrite(&sFlashDev, &cmd, 1);
  spiDeselect(&sFlashDev);
}

static uint8_t flashReadStatus(void) {
  uint8_t cmd = CMD_RDSR;
  uint8_t status;
  spiSelect(&sFlashDev);
  spiWrite(&sFlashDev, &cmd, 1);
  spiRead(&sFlashDev, &status, 1);
  spiDeselect(&sFlashDev);
  return status;
}

static bool flashWaitReady(void) {
  for (uint32_t i = 0; i < 500000; i++) {
    if ((flashReadStatus() & SR_WIP) == 0) return true;
  }
  return false; /* timeout */
}

static void flashPowerDown(void) {
  uint8_t cmd = CMD_DP;
  spiSelect(&sFlashDev);
  spiWrite(&sFlashDev, &cmd, 1);
  spiDeselect(&sFlashDev);
}

static void flashReleasePowerDown(void) {
  uint8_t cmd = CMD_RDP;
  spiSelect(&sFlashDev);
  spiWrite(&sFlashDev, &cmd, 1);
  spiDeselect(&sFlashDev);
  /* tRES1 ≈ 3 µs — short busy-wait */
  for (volatile int i = 0; i < 400; i++);
}

static uint32_t flashReadId(void) {
  uint8_t buf[3] = {0};
  uint8_t cmd = CMD_RDID;
  spiSelect(&sFlashDev);
  spiWrite(&sFlashDev, &cmd, 1);
  spiRead(&sFlashDev, buf, sizeof(buf));
  spiDeselect(&sFlashDev);
  return (buf[0] << 16) | (buf[1] << 8) | buf[2];
}

static bool flashOpen(void) {
  if (!spiOpen(&sFlashDev)) return false;
  flashReleasePowerDown();
  return true;
}

static bool flashClose(void) {
  flashPowerDown();
  return spiClose(&sFlashDev);
}

/* ---- public API ---- */

uint32_t eepromGetSize(void) { return EEPROM_IMG_LEN; }

bool eepromInit(void) {
  if (!flashOpen()) return false;
  uint32_t id = flashReadId();
  printf(">> eeprom: jedec id = 0x%06lX\n", id);
  return flashClose();
}

bool eepromRead(uint32_t addr, uint8_t* dstP, uint32_t len) {
  if (len == 0) return true;
  if (!flashOpen()) return false;

  uint8_t cmd[4];
  cmd[0] = CMD_READ;
  cmd[1] = (uint8_t)(addr >> 16);
  cmd[2] = (uint8_t)(addr >> 8);
  cmd[3] = (uint8_t)(addr);

  spiSelect(&sFlashDev);
  spiWrite(&sFlashDev, cmd, sizeof(cmd));
  spiRead(&sFlashDev, dstP, (int)len);
  spiDeselect(&sFlashDev);

  return flashClose();
}

bool eepromWrite(uint32_t addr, uint8_t* srcP, uint32_t len) {
  if (len == 0) return true;
  if (!flashOpen()) return false;

  while (len > 0) {
    /* Respect page boundaries (256-byte aligned pages) */
    uint32_t pageOff = addr & (FLASH_PAGE_SIZE - 1);
    uint32_t pageLeft = FLASH_PAGE_SIZE - pageOff;
    uint32_t toWrite = (len < pageLeft) ? len : pageLeft;

    flashWriteEnable();

    uint8_t cmd[4];
    cmd[0] = CMD_PP;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);

    spiSelect(&sFlashDev);
    spiWrite(&sFlashDev, cmd, sizeof(cmd));
    spiWrite(&sFlashDev, srcP, (int)toWrite);
    spiDeselect(&sFlashDev);

    if (!flashWaitReady()) {
      flashClose();
      return false;
    }

    addr += toWrite;
    srcP += toWrite;
    len -= toWrite;
  }

  return flashClose();
}

bool eepromErase(uint32_t addr, uint32_t len) {
  if (len == 0) return true;
  if (!flashOpen()) return false;

  /* Align start down, end up to sector boundaries */
  uint32_t endAddr = addr + len;
  addr &= ~(uint32_t)(FLASH_SECTOR_SIZE - 1);

  while (addr < endAddr) {
    flashWriteEnable();

    uint8_t cmd[4];
    cmd[0] = CMD_SE;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);

    spiSelect(&sFlashDev);
    spiWrite(&sFlashDev, cmd, sizeof(cmd));
    spiDeselect(&sFlashDev);

    if (!flashWaitReady()) {
      flashClose();
      return false;
    }

    addr += FLASH_SECTOR_SIZE;
  }

  return flashClose();
}