#define MAX_IMAGE_FLIPS 32
#include "epd.h"

// clang-format off
#include "bb_epaper.h"
#include "cc26xx_io.inl"
#include "bb_ep.inl"
#include "bb_ep_gfx.inl"
#include "bb_ep_helpers.inl"
#include "bb_panels/EP29_128x296.inl"
#include "bb_panels/EP294_128x296.inl"
// clang-format on
#include "eeprom.h"
#include "periph.h"
#include "powermgt.h"
#include "radio.h"
#include "syncedproto.h"

static BBEPDISP disp = {0};

void epdInit(void) {
  yield();
  pinMode(EPD_BS, OUTPUT);
  digitalWrite(EPD_BS, LOW);

  bbepSetPanel(&disp, &panel_EP29_128x296);
  bbepInitIO(&disp, EPD_DC, EPD_RST, EPD_BUSY, EPD_CS, EPD_MOSI, EPD_CLK, 4000000);
  if (bbepTestPanelType(&disp) != disp.chip_type) bbepSetPanel(&disp, &panel_EP294_128x296);
}

void epdWrite(uint8_t* pData, int iLen) {
  yield();
  for (int i = 0; i < iLen; i++) pData[i] = ~pData[i];
  bbepWriteData(&disp, pData, iLen);
}

void epdRefresh(void) {
  yield();
  bbepRefresh(&disp, REFRESH_FULL);
  bbepWaitBusy(&disp);
}

void epdDeinit(void) {
  bbepSleep(&disp, 1);  // deep sleep
  if (disp.ucScreen) {
    free(disp.ucScreen);
    disp.ucScreen = NULL;
  }
}

uint32_t getImageSize(void) { return disp.width * disp.height / 8; }

void drawImageAtAddress(uint32_t addr, uint8_t lut) {
  uint8_t buf[256];
  uint32_t imageSize = getImageSize();
  struct EepromImageHeader* eih = (struct EepromImageHeader*)buf;
  eepromRead(addr, buf, sizeof(struct EepromImageHeader));
  printf("Drawing image: type=0x%02X, addr=0x%08lX\n", eih->dataType, addr);

  switch (eih->dataType) {
    case DATATYPE_IMG_RAW_1BPP:
      printf("DRAW: 1BPP\n");
      epdInit();
      bbepFill(&disp, BBEP_WHITE, PLANE_DUPLICATE);
      yield();
      bbepStartWrite(&disp, PLANE_0);
      for (uint32_t c = 0; c < imageSize; c += sizeof(buf)) {
        uint32_t toRead = sizeof(buf);
        if (c + toRead > imageSize) toRead = imageSize - c;
        yield();
        eepromRead(addr + sizeof(struct EepromImageHeader) + c, buf, toRead);
        printf("Read %lu bytes at 0x%08lX\n", toRead, addr + sizeof(struct EepromImageHeader) + c);
        epdWrite(buf, toRead);
      }
      printf("Refresh\n");
      epdRefresh();
      epdDeinit();
      break;
    default:
      printf(
          "Image with type 0x%02X was requested, but we don't know what to do "
          "with that currently...\r\n",
          eih->dataType);
      break;
  }
}

static void epdGfxInit(void) {
  if (disp.ucScreen == NULL) {
    if (bbepAllocBuffer(&disp, 0) != BBEP_SUCCESS) printf("Failed to allocate buffer!\r\n");
  }
  bbepSetRotation(&disp, 270);
  bbepSetFont(&disp, FONT_16x16);
  bbepSetTextColor(&disp, BBEP_BLACK, BBEP_WHITE);
}

static void drawBattery(int x, int y) {
  bbepSetCursor(&disp, x, y);
  bbepPrintf(&disp, "Battery: %d.%02dV Temp: %d'C\r\n", batteryVoltage / 1000, batteryVoltage % 1000 / 10, temperature);
  bbepPrintf(&disp, "MAC: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\r\n\r\n", mSelfMac[7], mSelfMac[6], mSelfMac[5],
             mSelfMac[4], mSelfMac[3], mSelfMac[2], mSelfMac[1], mSelfMac[0]);
}

void clearScreen(void) {
  epdInit();
  epdGfxInit();
  bbepFill(&disp, BBEP_WHITE, PLANE_DUPLICATE);
  bbepWritePlane(&disp, PLANE_DUPLICATE, 0);
  epdRefresh();
  epdDeinit();
}

void showSplashScreen(void) {
  epdInit();
  epdGfxInit();
  bbepFill(&disp, BBEP_WHITE, PLANE_DUPLICATE);
  bbepPrintln(&disp, "OpenEPaperLink\n");
  bbepSetFont(&disp, FONT_8x8);
  drawBattery(0, 40);
  bbepSetFont(&disp, FONT_16x16);
  bbepSetCursor(&disp, 0, disp.height - 20);
  bbepPrintln(&disp, "Starting...");
  bbepWritePlane(&disp, PLANE_DUPLICATE, 0);
  epdRefresh();
  epdDeinit();
}

void showAPFound(void) {
  epdInit();
  epdGfxInit();
  bbepFill(&disp, BBEP_WHITE, PLANE_DUPLICATE);
  bbepPrintln(&disp, "AP Found\n");
  bbepSetFont(&disp, FONT_8x8);
  bbepPrintf(&disp, "AP: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\r\n", APmac[7], APmac[6], APmac[5], APmac[4], APmac[3],
             APmac[2], APmac[1], APmac[0]);
  bbepPrintf(&disp, "RSSI: %ddBm    LQI: %d\r\n", mLastRSSI, mLastLqi);
  bbepPrintf(&disp, "Ch %d\r\n", currentChannel);
  drawBattery(0, disp.height - 20);
  bbepWritePlane(&disp, PLANE_DUPLICATE, 0);
  epdRefresh();
  epdDeinit();
}

void showNoAP(void) {
  epdInit();
  epdGfxInit();
  bbepFill(&disp, BBEP_WHITE, PLANE_DUPLICATE);
  bbepPrintln(&disp, "NO AP Found\n");
  bbepSetFont(&disp, FONT_8x8);
  bbepPrintln(&disp, "Couldn't find an AP :(");
  drawBattery(0, disp.height - 20);
  bbepWritePlane(&disp, PLANE_DUPLICATE, 0);
  epdRefresh();
  epdDeinit();
}

void showLongTermSleep(void) {
  epdInit();
  epdGfxInit();
  bbepFill(&disp, BBEP_WHITE, PLANE_DUPLICATE);
  bbepPrintln(&disp, "zZ\n");
  drawBattery(0, disp.height - 20);
  bbepWritePlane(&disp, PLANE_DUPLICATE, 0);
  epdRefresh();
  epdDeinit();
}

void ledBlink(int pin, int ms) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  delay(ms);
  digitalWrite(pin, LOW);
}
