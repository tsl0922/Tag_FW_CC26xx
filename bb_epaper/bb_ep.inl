//
// bb_epaper
// Written by Larry Bank (bitbank@pobox.com)
// Project started 9/11/2024
//
// SPDX-FileCopyrightText: 2024 BitBank Software, Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// bb_ep.inl
// display interfacing/control code for bb_eink library
//
#ifndef __BB_EP__
#define __BB_EP__
// forward declarations
void InvertBytes(uint8_t *pData, int bLen);
void bbepSetPixelFast4Clr(void *pb, int x, int y, unsigned char ucColor);
void bbepSetPixelFast3Clr(void *pb, int x, int y, unsigned char ucColor);
void bbepSetPixelFast2Clr(void *pb, int x, int y, unsigned char ucColor);
void bbepSetPixelFast16Clr(void *pb, int x, int y, unsigned char ucColor);
void bbepSetPixelFast4Gray(void *pb, int x, int y, unsigned char ucColor);
int bbepSetPixel4Gray(void *pb, int x, int y, unsigned char ucColor);
int bbepSetPixel4Clr(void *pb, int x, int y, unsigned char ucColor);
int bbepSetPixel3Clr(void *pb, int x, int y, unsigned char ucColor);
int bbepSetPixel2Clr(void *pb, int x, int y, unsigned char ucColor);
int bbepSetPixel16Clr(void *pb, int x, int y, unsigned char ucColor);
void bbepSetPixel2ClrDither(void *pb, int x, int y, unsigned char ucColor);

// Color mapping tables for each type of display
// the 7 basic colors (and 9 unsupported) are translated into the correct colors
// for each display type
// #define BLACK 0, WHITE 1, YELLOW 2, RED 3, BLUE 4, GREEN 5, ORANGE 6
const uint8_t u8Colors_2clr[16] = {
    BBEP_BLACK, BBEP_WHITE, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK,
    BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK
};

const uint8_t u8Colors_3clr[16] = { // red and yellow are the same for these displays (one or the other)
    BBEP_BLACK, BBEP_WHITE, BBEP_RED, BBEP_RED, BBEP_BLACK, BBEP_RED, BBEP_BLACK, BBEP_RED,
    BBEP_RED, BBEP_BLACK, BBEP_RED, BBEP_BLACK, BBEP_RED, BBEP_BLACK, BBEP_RED, BBEP_BLACK
};
// 2-bit grayscale mode (bits are inverted)
const uint8_t u8Colors_4gray[16] = {
BBEP_GRAY3, BBEP_GRAY2, BBEP_GRAY1, BBEP_GRAY0, BBEP_GRAY3, BBEP_GRAY2, BBEP_GRAY1, BBEP_GRAY0,
BBEP_GRAY3, BBEP_GRAY2, BBEP_GRAY1, BBEP_GRAY0, BBEP_GRAY3, BBEP_GRAY2, BBEP_GRAY1, BBEP_GRAY0
};

// the 4-color mode
const uint8_t u8Colors_4clr[16] = {  // black white red yellow
    BBEP_BLACK, BBEP_WHITE, BBEP_RED, BBEP_YELLOW, BBEP_BLACK, BBEP_WHITE, BBEP_RED, BBEP_YELLOW,
    BBEP_BLACK, BBEP_WHITE, BBEP_RED, BBEP_YELLOW, BBEP_BLACK, BBEP_WHITE, BBEP_RED, BBEP_YELLOW
};

// the 4-color mode (red and yellow are swapped)
const uint8_t u8Colors_4clr_v2[16] = {  // black white red yellow
    BBEP_BLACK, BBEP_WHITE, BBEP_YELLOW, BBEP_RED, BBEP_BLACK, BBEP_WHITE, BBEP_YELLOW, BBEP_RED,
    BBEP_BLACK, BBEP_WHITE, BBEP_YELLOW, BBEP_RED, BBEP_BLACK, BBEP_WHITE, BBEP_YELLOW, BBEP_RED
}; 

const uint8_t u8Colors_7clr[16] = {  // ACeP 7-color
    BBEP_BLACK, BBEP_WHITE, BBEP_YELLOW, BBEP_RED, BBEP_BLUE, BBEP_GREEN, BBEP_ORANGE, BBEP_BLACK,
    BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK
};

const uint8_t u8Colors_spectra[16] = {  // Spectra 6
    BBEP_BLACK, BBEP_WHITE, BBEP_YELLOW, BBEP_RED, 0x05, 0x06, BBEP_BLACK, BBEP_BLACK,
    BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK, BBEP_BLACK
};


const uint8_t ucMirror[256] PROGMEM =
{0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240,
    8, 136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248,
    4, 132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52, 180, 116, 244,
    12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252,
    2, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178, 114, 242,
    10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250,
    6, 134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246,
    14, 142, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254,
    1, 129, 65, 193, 33, 161, 97, 225, 17, 145, 81, 209, 49, 177, 113, 241,
    9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185, 121, 249,
    5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245,
    13, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253,
    3, 131, 67, 195, 35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243,
    11, 139, 75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251,
    7, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247,
    15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 255};

#ifdef NO_RAM
uint8_t u8Cache[128]; // buffer a single line of up to 1024 pixels
#else // we need a larger cache for 4-bit panels
uint8_t u8Cache[512];
#endif

//
// Set the e-paper panel type
// This must be called before any other bb_epaper functions
//
int bbepSetPanel(BBEPDISP *pBBEP, const EPD_PANEL *panel)
{   
    switch (pBBEP->iOrientation) {
        case 0:
        case 180:
            pBBEP->width = pBBEP->native_width = panel->width;
            pBBEP->height = pBBEP->native_height = panel->height;
            break;
        case 90:
        case 270:
            pBBEP->width = pBBEP->native_height = panel->height;
            pBBEP->height = pBBEP->native_width = panel->width;
            break;
    }

    pBBEP->x_offset = panel->x_offset;
    pBBEP->chip_type = panel->chip_type;
    pBBEP->iFlags = panel->flags;
    pBBEP->iPasses = 32; // default LUT pushes for UC81xx partial update mode
    // If the memory is already allocated and the user is setting a
    // panel type which needs 2 planes, assume they have been allocated
    if (pBBEP->ucScreen && (pBBEP->iFlags & (BBEP_3COLOR | BBEP_4GRAY | BBEP_4COLOR))) {
       pBBEP->iFlags |= BBEP_HAS_SECOND_PLANE;
    }
    pBBEP->pInitFull = panel->pInitFull;
    pBBEP->pInitFast = panel->pInitFast;
    pBBEP->pInitPart = panel->pInitPart;
    pBBEP->pColorLookup = panel->pColorLookup;
    pBBEP->type = panel->type;
    // select the correct pixel drawing functions (2/3/4/7 color)
    if (pBBEP->iFlags & BBEP_4COLOR) {
        pBBEP->pfnSetPixel = bbepSetPixel4Clr;
        pBBEP->pfnSetPixelFast = bbepSetPixelFast4Clr;
    } else if (pBBEP->iFlags & BBEP_4GRAY) {
        pBBEP->pfnSetPixel = bbepSetPixel4Gray;
        pBBEP->pfnSetPixelFast = bbepSetPixelFast4Gray;
    } else if (pBBEP->iFlags & BBEP_3COLOR) {
        pBBEP->pfnSetPixel = bbepSetPixel3Clr;
        pBBEP->pfnSetPixelFast = bbepSetPixelFast3Clr;
    } else if (pBBEP->iFlags & BBEP_7COLOR) {
        pBBEP->pfnSetPixel = bbepSetPixel16Clr;
        pBBEP->pfnSetPixelFast = bbepSetPixelFast16Clr;
    } else { // must be B/W
        pBBEP->pfnSetPixel = bbepSetPixel2Clr;
        pBBEP->pfnSetPixelFast = bbepSetPixelFast2Clr;
    }
    return BBEP_SUCCESS;
} /* bbepSetPanel() */

void bbepSetDitherPattern(BBEPDISP *pBBEP, uint8_t iPattern)
{
    if (iPattern >= DITHER_COUNT) return;

    pBBEP->iDither = iPattern;
    if (iPattern == DITHER_NONE) { // reset to default behavior
        if (pBBEP->iFlags & BBEP_4COLOR) {
            pBBEP->pfnSetPixelFast = bbepSetPixelFast4Clr;
        } else if (pBBEP->iFlags & BBEP_4GRAY) {
            pBBEP->pfnSetPixelFast = bbepSetPixelFast4Gray;
        } else if (pBBEP->iFlags & BBEP_3COLOR) {
            pBBEP->pfnSetPixelFast = bbepSetPixelFast3Clr;
        } else if (pBBEP->iFlags & BBEP_7COLOR) {
            pBBEP->pfnSetPixelFast = bbepSetPixelFast16Clr;
        } else { // must be B/W
            pBBEP->pfnSetPixelFast = bbepSetPixelFast2Clr;
        }
    } else { // use the dithered pixel drawing functions
        pBBEP->pfnSetPixelFast = bbepSetPixel2ClrDither;
    }
} /* bbepSetDitherPattern() */

int bbepCreateVirtual(BBEPDISP *pBBEP, int iWidth, int iHeight, int iFlags)
{
    if (pBBEP) {
        memset(pBBEP, 0, sizeof(BBEPDISP));
        pBBEP->native_width = pBBEP->width = iWidth;
        pBBEP->native_height = pBBEP->height = iHeight;
        pBBEP->iFlags = iFlags;
        pBBEP->chip_type = BBEP_CHIP_NONE;
    // select the correct pixel drawing functions (2/3/4 color)
        if (iFlags & BBEP_4COLOR) {
            pBBEP->pColorLookup = u8Colors_4clr;
            pBBEP->pfnSetPixel = bbepSetPixel4Clr;
            pBBEP->pfnSetPixelFast = bbepSetPixelFast4Clr;
        } else if (iFlags & BBEP_4GRAY) {
            pBBEP->pColorLookup = u8Colors_4clr;
            pBBEP->pfnSetPixel = bbepSetPixel4Gray;
            pBBEP->pfnSetPixelFast = bbepSetPixelFast4Gray;
        } else if (iFlags & BBEP_3COLOR) {
            pBBEP->pColorLookup = u8Colors_3clr;
            pBBEP->pfnSetPixel = bbepSetPixel3Clr;
            pBBEP->pfnSetPixelFast = bbepSetPixelFast3Clr;
        } else if (pBBEP->iFlags & BBEP_7COLOR) {
            pBBEP->pColorLookup = u8Colors_7clr;
            pBBEP->pfnSetPixel = bbepSetPixel16Clr;
            pBBEP->pfnSetPixelFast = bbepSetPixelFast16Clr;
        } else { // must be B/W
            pBBEP->pColorLookup = u8Colors_2clr;
            pBBEP->pfnSetPixel = bbepSetPixel2Clr;
            pBBEP->pfnSetPixelFast = bbepSetPixelFast2Clr;
        }
        return BBEP_SUCCESS;
    } else {
        return BBEP_ERROR_BAD_PARAMETER;
    }
}
// Put the ESP32 into light sleep for N milliseconds
void bbepLightSleep(uint32_t u32Millis, uint8_t bLightSleep)
{
#ifdef ARDUINO_ARCH_ESP32
  if (bLightSleep) {
      esp_sleep_enable_timer_wakeup(u32Millis * 1000L);
      esp_light_sleep_start();
  } else {
      delay(u32Millis);
  }
#else
  (void)bLightSleep;
  delay(u32Millis);
#endif
}
//
// Wait for the busy status line to show idle
// The polarity of the busy signal is reversed on the UC81xx compared
// to the SSD16xx controllers
//
void bbepWaitBusy(BBEPDISP *pBBEP)
{
    int iTimeout = 0;
    int iMaxTime = 5000; // for B/W panels

    if (!pBBEP) return;
    if (pBBEP->iBUSYPin == 0xff) return;
    delay(10); // give time for the busy status to be valid
    uint8_t busy_idle =  (pBBEP->chip_type == BBEP_CHIP_UC81xx) ? HIGH : LOW;
    delay(1); // some panels need a short delay before testing the BUSY line
    if (pBBEP->iFlags & (BBEP_3COLOR | BBEP_4COLOR | BBEP_7COLOR)) {
        iMaxTime = 30000; // multi-color panels can take a long time
    }
    while (iTimeout < iMaxTime) {
        if (digitalRead(pBBEP->iBUSYPin) == busy_idle) break;
        // delay(1);
        bbepLightSleep(20, pBBEP->bLightSleep); // save battery power by checking every 20ms
        iTimeout += 20;
    }
} /* bbepWaitBusy() */
//
// Return if panel is busy
//
bool bbepIsBusy(BBEPDISP *pBBEP)
{
    if (!pBBEP) return false;
    if (pBBEP->iBUSYPin == 0xff) return false;
    delay(10); // give time for the busy status to be valid
    uint8_t busy_idle =  (pBBEP->chip_type == BBEP_CHIP_UC81xx) ? HIGH : LOW;
    delay(1); // some panels need a short delay before testing the BUSY line
    return (digitalRead(pBBEP->iBUSYPin) != busy_idle);
} /* bbepWaitBusy() */
//
// Toggle the reset line to wake up the eink from deep sleep
//
void bbepWakeUp(BBEPDISP *pBBEP)
{
    if (!pBBEP) return;
    if (pBBEP->iRSTPin == 0xff) return;

    digitalWrite(pBBEP->iRSTPin, LOW);
    delay(10);
    digitalWrite(pBBEP->iRSTPin, HIGH);
    delay(20);
    bbepWaitBusy(pBBEP);
} /* bbepWakeUp() */
//
// Set the memory window for future writes into panel memory
//
void bbepSetAddrWindow(BBEPDISP *pBBEP, int x, int y, int cx, int cy)
{
    uint8_t uc[12];
    int i, tx, ty;
    
    if (!pBBEP) return;
    if (pBBEP->iFlags & (BBEP_4COLOR | BBEP_7COLOR)) return;
    
#ifdef FUTURE
    if (pBBEP->chip_type == BBEP_CHIP_IT8951) {
        uint16_t u16Temp[6];
        u16Temp[0] = 0; // DEBUG (_endian_type << 8 | _pix_bpp << 4 | _rotate);
        u16Temp[1] = x;
        u16Temp[2] = y;
        u16Temp[3] = cx;
        u16Temp[4] = cy;
        bbepWriteIT8951CmdArgs(pBBEP, IT8951_TCON_LD_IMG_AREA, u16Temp, 5);
        return;
    }
#endif
    tx = x/8; // round down to next lower byte
    ty = y;
    cx = (cx + 7) & 0xfff8; // make width an even number of bytes
    if (pBBEP->chip_type == BBEP_CHIP_UC81xx) {
        bbepWriteCmd(pBBEP, UC8151_PTIN); // partial in
        bbepWriteCmd(pBBEP, UC8151_PTL); // partial window
        i = 0;
        tx *= 8;
        if (pBBEP->native_width >= 256) { // need 2 bytes per x
            uc[i++] = (uint8_t)(tx>>8); // start x
            uc[i++] = (uint8_t)tx;
            uc[i++] = (uint8_t)((tx+cx-1)>>8); // end x
            uc[i++] = (uint8_t)((tx+cx-1) | 7);
        } else {
            uc[i++] = tx; // start x
            uc[i++] = (tx+cx-1) | 7; // end x
        }
        if (pBBEP->native_height >= 250) {
            uc[i++] = (uint8_t)(ty>>8); // start y
            uc[i++] = (uint8_t)ty;
            uc[i++] = (uint8_t)((ty+cy-1)>>8); // end y
            uc[i++] = (uint8_t)(ty+cy-1);
        } else {
            uc[i++] = (uint8_t)ty;
            uc[i++] = (uint8_t)(ty+cy-1);
        }
        uc[i++] = 1; // refresh whole screen (0=refresh partial window only)
        bbepWriteData(pBBEP, uc, i);
        //       EPDWriteCmd(UC8151_PTOU); // partial out
    } else { // SSD16xx
        //        bbepCMD2(pBBEP, SSD1608_DATA_MODE, 0x3);
        bbepWriteCmd(pBBEP, SSD1608_SET_RAMXPOS);
        tx += pBBEP->x_offset;
        if (pBBEP->type == EP7_960x640 || pBBEP->type == EP426_800x480 || pBBEP->type == EP426_800x480_4GRAY) { // pixels, not bytes version
            if (pBBEP->type == EP7_960x640) {
                tx <<= 3;
            }
            uc[0] = (tx & 0xff);
            uc[1] = ((tx >> 8) & 0xff); // high byte
            uc[2] = (tx+cx-1) & 0xff; // low byte
            uc[3] = (tx+cx-1) >> 8; // high byte
            bbepWriteData(pBBEP, uc, 4);
            // set ram counter to start of this region
            bbepWriteCmd(pBBEP, SSD1608_SET_RAMXCOUNT);
            uc[0] = (tx & 0xff);
            uc[1] = (tx >> 8);
            bbepWriteData(pBBEP, uc, 2);
        } else { // bytes version
            uc[0] = tx; // start x (byte boundary)
            uc[1] = tx+((cx-1)>>3); // end x
            bbepWriteData(pBBEP, uc, 2);
            // set ram counter to start of this region
            bbepCMD2(pBBEP, SSD1608_SET_RAMXCOUNT, tx);
        }
        
        bbepWriteCmd(pBBEP, SSD1608_SET_RAMYPOS);
        if (pBBEP->type == EP426_800x480 || pBBEP->type == EP426_800x480_4GRAY) { // flipped y
            uc[2] = (uint8_t)ty; // start y
            uc[3] = (uint8_t)(ty>>8);
            uc[0] = (uint8_t)(ty+cy-1); // end y
            uc[1] = (uint8_t)((ty+cy-1)>>8);
        } else {
            uc[0] = (uint8_t)ty; // start y
            uc[1] = (uint8_t)(ty>>8);
            uc[2] = (uint8_t)(ty+cy-1); // end y
            uc[3] = (uint8_t)((ty+cy-1)>>8);
        }
        bbepWriteData(pBBEP, uc, 4);
        
        // set ram counter to start of this region
        uc[0] = ty;
        uc[1] = (ty>>8);
        bbepWriteCmd(pBBEP, SSD1608_SET_RAMYCOUNT);
        bbepWriteData(pBBEP, uc, 2);
        //        bbepCMD2(pBBEP, SSD1608_DATA_MODE, 0x3);
    }
    bbepWaitBusy(pBBEP);
} /* bbepSetAddrWindow() */
//
// Put the eink into light or deep sleep
//
void bbepSleep(BBEPDISP *pBBEP, int bDeep)
{
    if (!pBBEP) return;
    if (pBBEP->chip_type == BBEP_CHIP_UC81xx) {
        if (pBBEP->iFlags & BBEP_7COLOR) {
            bbepCMD2(pBBEP, UC8151_POFF, 0x00); // power off
            if (pBBEP->iFlags & BBEP_SPLIT_BUFFER) { // dual cable EPD
               pBBEP->iCSPin = pBBEP->iCS2Pin;
               bbepCMD2(pBBEP, UC8151_POFF, 0x00); // second controller
               pBBEP->iCSPin = pBBEP->iCS1Pin;
            }
        } else if (pBBEP->iFlags & BBEP_4COLOR) {
            bbepCMD2(pBBEP, 0x02, 0x00); // power off
            bbepWaitBusy(pBBEP);
            if (bDeep) {
                bbepCMD2(pBBEP, 0x07, 0xa5); // deep sleep
            }
        } else {
            bbepCMD2(pBBEP, UC8151_CDI, 0x17); // border floating
            bbepWriteCmd(pBBEP, UC8151_POFF); // power off
            bbepWaitBusy(pBBEP);
            if (bDeep) {
                bbepCMD2(pBBEP, UC8151_DSLP, 0xa5); // deep sleep
            }
        }
    } else {
        bbepCMD2(pBBEP, SSD1608_DEEP_SLEEP, (bDeep) ? 0x03 : 0x01); // deep sleep mode 1 keeps RAM,only uses about 1uA
    }
    pBBEP->is_awake = 0;
} /* bbepSleep() */

void bbepStartWrite(BBEPDISP *pBBEP, int iPlane)
{
    uint16_t u8Cmd; // AVR crashes w/odd number of bytes for stack vars
    
    if (!pBBEP) return;
    if (pBBEP->chip_type == BBEP_CHIP_UC81xx) {
        if (pBBEP->iFlags & BBEP_RED_SWAPPED) {
            if (iPlane == PLANE_0)
                u8Cmd = UC8151_DTM1;
            else
                u8Cmd = UC8151_DTM2;
        } else {
            if (iPlane == PLANE_0)
                u8Cmd = UC8151_DTM2;
            else
                u8Cmd = UC8151_DTM1;
        }
    } else { // SSD16xx
        if (iPlane == PLANE_0)
            u8Cmd = SSD1608_WRITE_RAM;
        else
            u8Cmd = SSD1608_WRITE_ALTRAM;
    }
    bbepWriteCmd(pBBEP, (uint8_t)u8Cmd);
} /* bbepStartWrite() */
//
// Dynamically generate partial update LUTS
// based on the iPasses variable
// (only UC81xx for now)
//
void bbepMakeLUTs(BBEPDISP *pBBEP)
{
    if (pBBEP->chip_type == BBEP_CHIP_UC81xx) {
        memset(u8Cache, 0, 44); // start with all 0's
        bbepWriteCmd(pBBEP, 0x20); // VCOM LUT
        // Setup the parameters common to all LUTs (6 bytes per row, 7 rows)
        // We only use 1 row to do all of the work
        // Byte 0 = 4 2-bit patterns, bytes 1-4 = counts for patterns, byte 5 = repeat count
        u8Cache[1] = (uint8_t)pBBEP->iPasses; // slot 0 = number of push passes
        u8Cache[2] = 1; // slot 1 = one pass to discharge all LUTs
        // slots 2 & 3 are 0's (disabled)
        u8Cache[5] = 1; // Number of repetitions for this row
        bbepWriteData(pBBEP, u8Cache, 44); // VCOM LUT is 44 bytes
        bbepWriteCmd(pBBEP, 0x21); // W->W LUT
        u8Cache[0] = 0x80; // 10 = push white, 00,00,00 = neutral
        bbepWriteData(pBBEP, u8Cache, 42); // W->W LUT is 42 bytes
        bbepWriteCmd(pBBEP, 0x22); // B->W LUT
        bbepWriteData(pBBEP, u8Cache, 42); // same pattern as W->W, we're pushing white
        bbepWriteCmd(pBBEP, 0x23); // W->B LUT
        u8Cache[0] = 0x40; // 01 = push black, 00,00,00 = neutral
        bbepWriteData(pBBEP, u8Cache, 42);
        bbepWriteCmd(pBBEP, 0x24); // B->B LUT
        bbepWriteData(pBBEP, u8Cache, 42); // same pattern as W->B, we're pushing black
        bbepWriteCmd(pBBEP, 0x25); // Border LUT
        u8Cache[0] = 0; // no activity
        bbepWriteData(pBBEP, u8Cache, 42);
    } else { // SSD16xx
    }
} /* bbepMakeLUTs() */
//
// More efficient means of sending commands, data and busy-pauses
//
void bbepSendCMDSequence(BBEPDISP *pBBEP, const uint8_t *pSeq)
{
    int iLen;
    uint8_t *s;
    
    if (pBBEP == NULL || pSeq == NULL) return;
    
    s = (uint8_t *)pSeq;
    while (s[0] != 0) { // A 0 length terminates the list
        iLen = *s++;
        if (iLen == MAKE_LUTS) {
            bbepMakeLUTs(pBBEP);
        } else if (iLen == BUSY_WAIT) {
            bbepWaitBusy(pBBEP);
        } else if (iLen == EPD_RESET) {
            bbepWakeUp(pBBEP);
        } else {
            bbepWriteCmd(pBBEP, s[0]);
            s++;
            if (iLen > 1) {
                bbepWriteData(pBBEP, s, iLen-1);
                s += (iLen-1);
            }
        }
    } // while more commands to send
} /* bbepSendCMDSequence() */

//
// Tests the BUSY line to see if you're connected to a
// SSD16xx or UC81xx panel
//
int bbepTestPanelType(BBEPDISP *pBBEP)
{
    if (!pBBEP) return BBEP_CHIP_NOT_DEFINED;
    if (pBBEP->iRSTPin != 0xff) {
        digitalWrite(pBBEP->iRSTPin, LOW);
        delay(40);
        digitalWrite(pBBEP->iRSTPin, HIGH);
        delay(50);
    }
    if (digitalRead(pBBEP->iBUSYPin))
        return BBEP_CHIP_UC81xx; // high state = UltraChip ready
    else
        return BBEP_CHIP_SSD16xx; // low state = Solomon ready
} /* bbepTestPanelType() */
//
// Fill the display with a color
// e.g. all black (0x00) or all white (0xff)
// if there is no backing buffer, write directly to
// the EPD's framebuffer
//
void bbepFill(BBEPDISP *pBBEP, unsigned char ucColor, int iPlane)
{
    uint8_t uc1=0, uc2=0;
    int y, iSize, iPitch;
    uint8_t ucCMD1, ucCMD2;
    
    if (pBBEP == NULL) return;
    ucColor = pBBEP->pColorLookup[ucColor & 0xf]; // translate the color for this display type
    pBBEP->iCursorX = pBBEP->iCursorY = 0;
    iPitch = ((pBBEP->native_width+7)/8);
    iSize = pBBEP->native_height * iPitch;
    if (pBBEP->iFlags & BBEP_7COLOR) {
        uc1 = ucColor | (ucColor << 4);
        iPitch = pBBEP->native_width / 2;
        iSize = pBBEP->native_height * iPitch;
    } else if (pBBEP->iFlags & BBEP_3COLOR) {
        if (ucColor == BBEP_WHITE) {
            uc1 = 0xff; uc2 = 0x00; // red plane has priority
        } else if (ucColor == BBEP_BLACK) {
            uc1 = 0x00; uc2 = 0x00;
        } else if (ucColor == BBEP_RED) {
            uc1 = 0x00; uc2 = 0xff;
        }
    } else if (pBBEP->iFlags & BBEP_4COLOR) {
        iPitch = ((pBBEP->native_width+3)/4);
        iSize = pBBEP->native_height * iPitch;
        iPlane = PLANE_0; // only 1 2-bit memory plane
        uc1 = ucColor | (ucColor << 2);
        uc1 |= (uc1 << 4); // set color in all 4 pixels of the byte
    } else if (pBBEP->iFlags & BBEP_4GRAY) {
        uc1 = (ucColor & 1) ? 0xff : 0x00;
        uc2 = (ucColor & 2) ? 0xff : 0x00;
    } else { // B/W
        if (ucColor == BBEP_WHITE) {
            uc1 = uc2 = 0xff;
        } else {
            uc1 = uc2 = 0x00;
        }
    }
    if (pBBEP->ucScreen) { // there's a local framebuffer, use it
        if (pBBEP->iFlags & BBEP_7COLOR) {
            memset(pBBEP->ucScreen, uc1, iSize);
            return;
        } else if ((pBBEP->iFlags & (BBEP_4GRAY | BBEP_3COLOR)) || iPlane == PLANE_BOTH) {
            memset(pBBEP->ucScreen, uc1, iSize);
            memset(&pBBEP->ucScreen[iSize], uc2, iSize);
        } else if (iPlane == PLANE_DUPLICATE) {
            memset(pBBEP->ucScreen, uc1, iSize);
            if (pBBEP->iFlags & BBEP_HAS_SECOND_PLANE) {
                memset(&pBBEP->ucScreen[iSize], uc1, iSize);
            }
        } else if (iPlane == PLANE_0) {
            memset(pBBEP->ucScreen, uc1, iSize);
        } else if (iPlane == PLANE_1 && (pBBEP->iFlags & BBEP_HAS_SECOND_PLANE)) {
            memset(&pBBEP->ucScreen[iSize], uc2, iSize);
        }
    } else { // write directly to the EPD's framebuffer
        if (pBBEP->iFlags & BBEP_3COLOR) {
            if (ucColor == BBEP_WHITE) {
                uc1 = 0xff; uc2 = 0x00; // red plane has priority
            } else if (ucColor == BBEP_BLACK) {
                uc1 = 0x00; uc2 = 0x00;
            } else if (ucColor == BBEP_RED) {
                uc1 = 0x00; uc2 = 0xff;
            }
        } else if (!(pBBEP->iFlags & (BBEP_4GRAY | BBEP_4COLOR))) { // for B/W, both planes get the same data
            if (ucColor == BBEP_WHITE) ucColor = 0xff;
            else if (ucColor == BBEP_BLACK) ucColor = 0;
            uc1 = uc2 = ucColor;
            if (pBBEP->iFlags & BBEP_4BPP_DATA) { // special case
                 // EInk 4.1" 640x400 uses 4-bpp data for 1-bpp images
                 uc1 = (ucColor == BBEP_WHITE) ? 0x00 : 0x11;
                 memset(u8Cache, uc1, pBBEP->native_width/2);
                 bbepWriteCmd(pBBEP, 0x10);
                 for (y=0; y<pBBEP->native_height; y++) {
                     bbepWriteData(pBBEP, u8Cache, pBBEP->native_width/2);
                 }
                 return;
            }
        }
        if (pBBEP->chip_type == BBEP_CHIP_UC81xx) {
            if (pBBEP->iFlags & (BBEP_RED_SWAPPED | BBEP_4COLOR)) {
                ucCMD1 = UC8151_DTM1;
                ucCMD2 = UC8151_DTM2;
            } else {
                ucCMD1 = UC8151_DTM2;
                ucCMD2 = UC8151_DTM1;
            }
        } else {
            ucCMD1 = SSD1608_WRITE_RAM;
            ucCMD2 = SSD1608_WRITE_ALTRAM;
        }
        // Write one or both memory planes to the EPD
        if (iPlane == PLANE_0 || iPlane == PLANE_DUPLICATE) { // write to first plane
            bbepSetAddrWindow(pBBEP, 0,0, pBBEP->native_width, pBBEP->native_height);
            bbepWriteCmd(pBBEP, ucCMD1);
            for (y=0; y<pBBEP->native_height; y++) {
                memset(u8Cache, uc1, iPitch); // the data is overwritten after each write
                bbepWriteData(pBBEP, u8Cache, iPitch);
            } // for y
        }
        if (iPlane == PLANE_1 || iPlane == PLANE_DUPLICATE) { // write to first plane
            bbepSetAddrWindow(pBBEP, 0,0, pBBEP->native_width, pBBEP->native_height);
            bbepWriteCmd(pBBEP, ucCMD2);
            for (y=0; y<pBBEP->native_height; y++) {
                memset(u8Cache, uc2, iPitch); // the data is overwritten after each write
                bbepWriteData(pBBEP, u8Cache, iPitch);
            } // for y
        }
    }
} /* bbepFill() */

int bbepRefresh(BBEPDISP *pBBEP, int iMode)
{

    if (iMode != REFRESH_FULL && iMode != REFRESH_FAST && iMode != REFRESH_PARTIAL)
        return BBEP_ERROR_BAD_PARAMETER;
    
    switch (iMode) {
        case REFRESH_FULL:
            if (!(pBBEP->iFlags & BBEP_NEEDS_EXTRA_INIT)) { // already sent?
                bbepSendCMDSequence(pBBEP, pBBEP->pInitFull);
            }
            if (pBBEP->iFlags & BBEP_SPLIT_BUFFER) {
               // Send the same sequence to the second controller
               pBBEP->iCSPin = pBBEP->iCS2Pin;
               bbepSendCMDSequence(pBBEP, pBBEP->pInitFull);
               pBBEP->iCSPin = pBBEP->iCS1Pin;
            }
            break;
        case REFRESH_FAST:
            if (!pBBEP->pInitFast) { // fall back to full
                bbepSendCMDSequence(pBBEP, pBBEP->pInitFull);
            } else {
                bbepSendCMDSequence(pBBEP, pBBEP->pInitFast);
            }
            break;
        case REFRESH_PARTIAL:
            if (!pBBEP->pInitPart) { // Do a fast or full refresh if partial not defined
                if (pBBEP->pInitFast) {
                    bbepSendCMDSequence(pBBEP, pBBEP->pInitFast);
                } else { // fall back to full
                    bbepSendCMDSequence(pBBEP, pBBEP->pInitFull);
                }
            } else {
                bbepSendCMDSequence(pBBEP, pBBEP->pInitPart);
            }
            break;
        default:
            return BBEP_ERROR_BAD_PARAMETER;
    } // switch on mode
    if (pBBEP->chip_type == BBEP_CHIP_UC81xx) {
        if (pBBEP->iFlags & (BBEP_4GRAY | BBEP_4COLOR | BBEP_7COLOR)) {
            bbepCMD2(pBBEP, UC8151_DRF, 0x00);
            if (pBBEP->iFlags & BBEP_SPLIT_BUFFER) {
               // Send the same sequence to the second controller
               pBBEP->iCSPin = pBBEP->iCS2Pin;
               bbepCMD2(pBBEP, UC8151_DRF, 0);
               pBBEP->iCSPin = pBBEP->iCS1Pin;
            }
        } else {
            bbepWriteCmd(pBBEP, UC8151_PTOU); // partial out (update the entire panel, not just the last memory window)
            bbepWriteCmd(pBBEP, UC8151_DRF);
        }
    } else {
        const uint8_t u8CMD[4] = {0xf7, 0xc7, 0xff, 0xc0}; // normal, fast, partial, partial2
        const uint8_t u8CMDz[4] = {0xf4, 0xc7, 0xfc, 0}; // special set for SSD1680
        const uint8_t u8CMDz2[4] = {0xf4, 0xc7, 0xdc, 0}; // special set #2 for SSD1680
 //       const uint8_t u8CMDz3[4] = {0xf7, 0xc7, 0xdc, 0}; // special set #3
        if (pBBEP->iFlags & (BBEP_4GRAY | BBEP_3COLOR | BBEP_4COLOR)) {
            iMode = REFRESH_FAST;
        } // 3/4-color = 0xc7
        if (iMode == REFRESH_PARTIAL && pBBEP->iFlags & BBEP_PARTIAL2) {
            iMode = REFRESH_PARTIAL2; // special case for custom LUT
        }
        if (pBBEP->type == EP29Z_128x296 || pBBEP->type == EP213Z_122x250) {
            bbepCMD2(pBBEP, SSD1608_DISP_CTRL2, u8CMDz[iMode]);
        } else if (pBBEP->type == EP154Z_152x152) {
            bbepCMD2(pBBEP, SSD1608_DISP_CTRL2, u8CMDz2[iMode]);
        } else if (pBBEP->type == EP42B_400x300_4GRAY) {
            bbepCMD2(pBBEP, SSD1608_DISP_CTRL2, 0xcf); // SSD1683 does things differently :(
        } else {
            bbepCMD2(pBBEP, SSD1608_DISP_CTRL2, u8CMD[iMode]);
        }
        bbepWriteCmd(pBBEP, SSD1608_MASTER_ACTIVATE); // refresh
    }
    return BBEP_SUCCESS;
} /* bbepRefresh() */

void bbepSetRotation(BBEPDISP *pBBEP, int iRotation)
{
    pBBEP->iScreenOffset = 0;
    pBBEP->iOrientation = iRotation;
    
    switch (iRotation) {
        default: return;
        case 0:
        case 180:
            pBBEP->width = pBBEP->native_width;
            pBBEP->height = pBBEP->native_height;
            break;
        case 90:
        case 270:
            pBBEP->width = pBBEP->native_height;
            pBBEP->height = pBBEP->native_width;
            break;
    }
} /* bbepSetRotation() */

void bbepWriteImage4bppSpecial(BBEPDISP *pBBEP, uint8_t ucCMD)
{
    int tx, ty, iPitch, iRedOff;
    uint8_t uc, ucSrcMask, *s, *d;
    // Convert the bit direction and write the data to the EPD
    // This particular controller has 4 bits per pixel where 0=black, 3=white, 4=red 
    // this wastes 50% of the time transmitting bloated info (only need 2 bits) 
    iPitch = ((pBBEP->native_width+7)/8);
    iRedOff = pBBEP->native_height * iPitch;

    if (ucCMD) {
        bbepWriteCmd(pBBEP, ucCMD); // start write
    }
    if (pBBEP->iOrientation == 0) {
      for (ty=0; ty<pBBEP->height; ty++) {
         d = u8Cache;
         s = &pBBEP->ucScreen[ty * (pBBEP->width/8)];
         ucSrcMask = 0x80;
         for (tx=0; tx<pBBEP->width; tx+=2) {
             uc = 0x33; // start with white/white
             if (!(s[0] & ucSrcMask)) {// src pixel = black
                uc = 0x03;
             }
             if (s[iRedOff] & ucSrcMask) { // red
                 uc = 0x43;
             }
             ucSrcMask >>= 1;
             if (!(s[0] & ucSrcMask)) {// src pixel = black
                 uc &= 0xf0;
             }
             if (s[iRedOff] & ucSrcMask) { // red
                  uc &= 0xf0; uc |= 0x4;
             }
             ucSrcMask >>= 1;
             if (ucSrcMask == 0) {
                ucSrcMask = 0x80;
                s++;
             }
             *d++ = uc; // store 2 pixels
         } // for tx
        bbepWriteData(pBBEP, u8Cache, pBBEP->width/2);
      } // for ty
    } else if (pBBEP->iOrientation == 180) {
        for (ty=pBBEP->height-1; ty>=0; ty--) {
            d = u8Cache;
            s = &pBBEP->ucScreen[((ty+1) * (pBBEP->width/8)) - 1];
            ucSrcMask = 1;
            for (tx=pBBEP->width-1; tx>=0; tx-=2) {
                uc = 0x33; // start with white/white
                if (!(s[0] & ucSrcMask)) // black
                    uc = 0x03;
                if (s[iRedOff] & ucSrcMask) // red
                    uc = 0x43;
                ucSrcMask <<= 1;
                if (!(s[0] & ucSrcMask)) // src pixel = black
                    uc &= 0xf0;
                if (s[iRedOff] & ucSrcMask) { // red
                    uc &= 0xf0; uc |= 0x4;
                }
                ucSrcMask <<= 1;
                if (ucSrcMask == 0) {
                    s--;
                    ucSrcMask = 1;
                }
                *d++ = uc; // store 2 pixels
            } // for tx
            bbepWriteData(pBBEP, u8Cache, pBBEP->width/2);
        } // for ty
    } else if (pBBEP->iOrientation == 90) {
        iPitch = pBBEP->width / 8;
        for (tx=0; tx<pBBEP->width; tx++) {
            d = u8Cache;
            ucSrcMask = 0x80 >> (tx & 7);
            s = &pBBEP->ucScreen[(tx>>3) + ((pBBEP->height-1) * iPitch)];
            for (ty=pBBEP->height-1; ty > 0; ty-=2) {
                uc = 0x33;
                if (!(s[0] & ucSrcMask))
                    uc = 0x03; // black
                if (s[iRedOff] & ucSrcMask)
                    uc = 0x43; // red
                s -= iPitch;
                if (!(s[0] & ucSrcMask)) {// src pixel = black
                    uc &= 0xf0;
                }
                if (s[iRedOff] & ucSrcMask) { // red
                    uc &= 0xf0; uc |= 0x4;
                }
                s -= iPitch;
                *d++ = uc; // store 2 pixels
            } // for ty
            bbepWriteData(pBBEP, u8Cache, pBBEP->height/2);
        } // for tx
    } else if (pBBEP->iOrientation == 270) {
        iPitch = pBBEP->width / 8;
        for (tx=pBBEP->width-1; tx>=0; tx--) {
            d = u8Cache;
            ucSrcMask = 0x80 >> (tx & 7);
            s = &pBBEP->ucScreen[(tx>>3)];
            for (ty=pBBEP->height-1; ty > 0; ty-=2) {
                uc = 0x33;
                if (!(s[0] & ucSrcMask))
                    uc = 0x03; // black
                if (s[iRedOff] & ucSrcMask)
                    uc = 0x43; // red
                s += iPitch;
                if (!(s[0] & ucSrcMask)) {// src pixel = black
                    uc &= 0xf0;
                }
                if (s[iRedOff] & ucSrcMask) { // red
                    uc &= 0xf0; uc |= 0x4;
                }
                s += iPitch;
                *d++ = uc; // store 2 pixels
            } // for ty
            bbepWriteData(pBBEP, u8Cache, pBBEP->height/2);
        } // for tx
  } // 270
} /* bbepWriteImage4bppSpecial() */

// special case for panels with 2 controllers
void bbepWriteImage4bppDual(BBEPDISP *pBBEP, uint8_t ucCMD)
{
    int tx, ty, iPitch;
    uint8_t uc, *s, *d;
        
    if (ucCMD) {
        pBBEP->iCSPin = pBBEP->iCS1Pin;
        bbepWriteCmd(pBBEP, ucCMD); // start write
        pBBEP->iCSPin = pBBEP->iCS2Pin;
        bbepWriteCmd(pBBEP, ucCMD);
    }
    if (pBBEP->iOrientation == 0) {
        iPitch = pBBEP->native_width / 2;
        s = pBBEP->ucScreen;
        for (ty=0; ty<pBBEP->height; ty++) {
            pBBEP->iCSPin = pBBEP->iCS1Pin;
            bbepWriteData(pBBEP, s, iPitch/2);
            pBBEP->iCSPin = pBBEP->iCS2Pin;
            bbepWriteData(pBBEP, &s[iPitch/2], iPitch/2);
            s += iPitch; // 2 pixels per byte
        } // for ty
    } else if (pBBEP->iOrientation == 180) {
        iPitch = pBBEP->native_width / 2;
        for (ty=pBBEP->height-1; ty>=0; ty--) {
            s = &pBBEP->ucScreen[(ty * iPitch) + iPitch - 1];
            // reverse the pixel direction
            for (tx=0; tx<pBBEP->native_width; tx+=2) {
                uc = *s--;
                uc = (uc >> 4) | (uc << 4); // swap nibbles
                u8Cache[tx/2] = uc;
            }
            pBBEP->iCSPin = pBBEP->iCS1Pin;
            bbepWriteData(pBBEP, u8Cache, iPitch/2);
            pBBEP->iCSPin = pBBEP->iCS2Pin;
            bbepWriteData(pBBEP, &u8Cache[iPitch/2], iPitch/2);
        } // for ty
    } else if (pBBEP->iOrientation == 90) {
        iPitch = pBBEP->native_height / 2;
        for (tx=0; tx<pBBEP->width; tx++) {
            d = u8Cache;
            for (ty=pBBEP->height-1; ty > 0; ty-=2) {
                s = &pBBEP->ucScreen[(tx>>1) + (ty * iPitch)];
                if (tx & 1) {
                    uc = (s[0] << 4) | (s[-iPitch] & 0x0f);
                } else {
                    uc = (s[0] & 0xf0) | (s[-iPitch] >> 4);
                }
                s -= iPitch*2;
                *d++ = uc; // store 4 pixels
            } // for ty
            pBBEP->iCSPin = pBBEP->iCS1Pin;
            bbepWriteData(pBBEP, u8Cache, pBBEP->height/4);
            pBBEP->iCSPin = pBBEP->iCS2Pin;
            bbepWriteData(pBBEP, &u8Cache[pBBEP->height/4], pBBEP->height/4);
        } // for tx
    } else if (pBBEP->iOrientation == 270) {
        iPitch = pBBEP->native_height / 2;
        for (tx=pBBEP->width-1; tx>= 0; tx--) {
            d = u8Cache;
            for (ty=0; ty < pBBEP->height; ty+=2) {
                s = &pBBEP->ucScreen[(tx>>1) + (ty * iPitch)];
                if (tx & 1) {
                    uc = (s[0] << 4) | (s[iPitch] & 0x0f);
                } else {
                    uc = (s[0] & 0xf0) | (s[iPitch] >> 4);
                }
                s += iPitch*2;
                *d++ = uc; // store 4 pixels
            } // for ty
            pBBEP->iCSPin = pBBEP->iCS1Pin;
            bbepWriteData(pBBEP, u8Cache, pBBEP->height/4);
            pBBEP->iCSPin = pBBEP->iCS2Pin;
            bbepWriteData(pBBEP, &u8Cache[pBBEP->height/4], pBBEP->height/4);     
        } // for tx
    }
    pBBEP->iCSPin = pBBEP->iCS1Pin; // reset CS to #1
} /* bbepWriteImage4bppDual() */

void bbepWriteImage4bpp(BBEPDISP *pBBEP, uint8_t ucCMD)
{
    int tx, ty, iPitch;
    uint8_t uc, *s, *d;
        
    if (ucCMD) {
        bbepWriteCmd(pBBEP, ucCMD); // start write
    }
    if (pBBEP->iOrientation == 0) {
        iPitch = pBBEP->native_width / 2;
        s = pBBEP->ucScreen;
        for (ty=0; ty<pBBEP->height; ty++) {
            bbepWriteData(pBBEP, s, iPitch);
            s += iPitch; // 2 pixels per byte
        } // for ty
    } else if (pBBEP->iOrientation == 180) {
        iPitch = pBBEP->native_width / 2;
        for (ty=pBBEP->height-1; ty>=0; ty--) {
            s = &pBBEP->ucScreen[(ty * iPitch) + iPitch - 1];
            // reverse the pixel direction
            for (tx=0; tx<pBBEP->native_width; tx+=2) {
                uc = *s--;
                uc = (uc >> 4) | (uc << 4); // swap nibbles
                u8Cache[tx/2] = uc;
            }
            bbepWriteData(pBBEP, u8Cache, iPitch);
        } // for ty
    } else if (pBBEP->iOrientation == 90) {
        iPitch = pBBEP->native_height / 2;
        for (tx=0; tx<pBBEP->width; tx++) {
            d = u8Cache;
            for (ty=pBBEP->height-1; ty > 0; ty-=2) {
                s = &pBBEP->ucScreen[(tx>>1) + (ty * iPitch)];
                if (tx & 1) {
                    uc = (s[0] << 4) | (s[-iPitch] & 0x0f);
                } else {
                    uc = (s[0] & 0xf0) | (s[-iPitch] >> 4);
                }
                s -= iPitch*2;
                *d++ = uc; // store 4 pixels
            } // for ty
            bbepWriteData(pBBEP, u8Cache, pBBEP->height/2);
        } // for tx
    } else if (pBBEP->iOrientation == 270) {
        iPitch = pBBEP->native_height / 2;
        for (tx=pBBEP->width-1; tx>= 0; tx--) {
            d = u8Cache;
            for (ty=0; ty < pBBEP->height; ty+=2) {
                s = &pBBEP->ucScreen[(tx>>1) + (ty * iPitch)];
                if (tx & 1) {
                    uc = (s[0] << 4) | (s[iPitch] & 0x0f);
                } else {
                    uc = (s[0] & 0xf0) | (s[iPitch] >> 4);
                }
                s += iPitch*2;
                *d++ = uc; // store 4 pixels
            } // for ty
            bbepWriteData(pBBEP, u8Cache, pBBEP->height/2);
        } // for tx
    }
} /* bbepWriteImage4bpp() */

void bbepWriteImage2bpp(BBEPDISP *pBBEP, uint8_t ucCMD)
{
int tx, ty, iPitch;
uint8_t *s, *d, uc, uc1, ucMask;
uint8_t *pBuffer;

//    if (pBBEP->pInitFast) { // fast mode on 4-color displays
//     // requires sending the init sequence BEFORE the image data
//        bbepSendCMDSequence(pBBEP, pBBEP->pInitFast);
//    }
    pBuffer = pBBEP->ucScreen;
    if (ucCMD) {
        bbepWriteCmd(pBBEP, ucCMD); // start write
    }
    // Convert the bit direction and write the data to the EPD
    if (pBBEP->iOrientation == 180) {
        for (ty=pBBEP->height-1; ty>=0; ty--) {
            d = u8Cache;
            s = &pBuffer[ty * pBBEP->width/4];
            for (tx=pBBEP->width-4; tx>=0; tx-=4) {
                uc = 0;
                ucMask = 0x03;
                uc1 = s[tx>>2];
                for (int pix=0; pix<8; pix +=2) { // reverse the direction of the 4 pixels
                    uc <<= 2; // shift down 1 pixel
                    uc |= ((uc1 & ucMask) >> pix);
                    ucMask <<= 2;
                }
                *d++ = uc; // store 4 pixels
            } // for tx
            bbepWriteData(pBBEP, u8Cache, pBBEP->width/4);
        } // for ty
    } else if (pBBEP->iOrientation == 0) {
        s = pBBEP->ucScreen;
        iPitch = (pBBEP->width == 122) ? 32: pBBEP->width/4;
        for (ty=0; ty<pBBEP->height; ty++) {
            bbepWriteData(pBBEP, s, iPitch);
            s += (pBBEP->width+3)/4; // 4 pixels per byte
        } // for ty
    } else if (pBBEP->iOrientation == 90) {
        for (tx=0; tx<pBBEP->width; tx++) {
            d = u8Cache;
            for (ty=pBBEP->height-1; ty > 0; ty-=4) {
                s = &pBuffer[(tx>>2) + (ty * (pBBEP->width/4))];
                uc = 0;
                ucMask = 0xc0 >> ((tx & 3) * 2);
                for (int pix=0; pix<4; pix++) {
                    uc <<= 2; // shift down 1 pixel
                    uc |= ((s[0] & ucMask) >> ((3-(tx&3))*2)); // inverted plane 0
                    s -= (pBBEP->width/4);
                }
                *d++ = uc; // store 4 pixels
            } // for ty
            bbepWriteData(pBBEP, u8Cache, pBBEP->height/4);
        } // for tx
    } else if (pBBEP->iOrientation == 270) {
        for (tx=pBBEP->width-1; tx>=0; tx--) {
            d = u8Cache;
            for (ty=3; ty<pBBEP->height; ty+=4) {
                s = &pBuffer[(tx>>2) + (ty * pBBEP->width/4)];
                ucMask = 0xc0 >> ((tx & 3) * 2);
                uc = 0;
                for (int pix=0; pix<4; pix++) {
                    uc >>= 2;
                    uc |= ((s[0] & ucMask) << ((tx&3)*2)); // inverted plane 0
                    s -= (pBBEP->width/4);
                } // for pix
                *d++ = uc; // store 2 pixels
            } // for ty
            bbepWriteData(pBBEP, u8Cache, pBBEP->height/4);
        } // for x
  } // 270
} /* bbepWriteImage2bpp() */

//
// Write 1-bpp graphics to a display which wants to receive it as 4-bpp
//
static void bbepWriteImage1to4bpp(BBEPDISP *pBBEP, uint8_t ucCMD, uint8_t *pBuffer, int bInvert)
{
    int tx, ty;
    uint8_t *s, *d, uc;
    uint8_t ucInvert = 0;
    int iPitch;
// lookup table to convert 2 bits into 8
    const uint8_t u8Lookup[4] = {0x00, 0x01, 0x10, 0x11};

    iPitch = (pBBEP->width + 7) >> 3;
    if (bInvert) {
        ucInvert = 0xff; // red logic is inverted
    }
    if (ucCMD) {
        bbepWriteCmd(pBBEP, ucCMD); // start write
    }
    // Convert the bit direction and write the data to the EPD
    switch (pBBEP->iOrientation) {
        case 0:
            for (ty=0; ty<pBBEP->native_height; ty++) {
                d = u8Cache;
                s = &pBuffer[ty * iPitch];
                for (tx=0; tx<iPitch; tx++) {
                    uc = *s++;
                    *d++ = u8Lookup[uc >> 6];
                    *d++ = u8Lookup[(uc >> 4) & 3];
                    *d++ = u8Lookup[(uc >> 2) & 3];
                    *d++ = u8Lookup[uc & 3];
                }
                if (ucInvert) InvertBytes(u8Cache, iPitch*4);
                bbepWriteData(pBBEP, u8Cache, iPitch*4);
            } // for ty
            break;
    } // switch
} /* bbepWriteImage1to4bpp() */

//
// Write Image data (1-bpp entire plane) from RAM to the e-paper
// Rotate the pixels if necessary
// This version is specifically for the 272x792 5.79" panels
// since they split the memory into two halves
//
static void bbepWriteHalf(BBEPDISP *pBBEP, uint8_t ucCMD, uint8_t *pBuffer, int bInvert)
{
    int tx, ty;
    uint8_t *s, *d, ucSrcMask, ucDstMask, uc;
    uint8_t ucInvert = 0;
    int iPitch;

    iPitch = (pBBEP->native_width + 7) >> 3;
    if (bInvert) {
        ucInvert = 0xff; // red logic is inverted
    }
    if (ucCMD & 0x80) { // second half
        if (pBuffer) {
            pBuffer += (392/8); // start in the middle
        }
        if (ucCMD == 0xa4) {
        bbepWriteCmd(pBBEP, 0x91); // switch to second memory set
        u8Cache[0] = 4;
        bbepWriteData(pBBEP, u8Cache, 1);
        bbepWriteCmd(pBBEP, 0xc4);
        u8Cache[0] = 0x30; u8Cache[1] = 0;
        bbepWriteData(pBBEP, u8Cache, 2);
        bbepWriteCmd(pBBEP, 0xc5);
        u8Cache[0] = 0x0f; u8Cache[1] = 0x1; u8Cache[2] = u8Cache[3] = 0;
        bbepWriteData(pBBEP, u8Cache, 4);
        }
        bbepWriteCmd(pBBEP, 0xce);
        u8Cache[0] = 0x31;
        bbepWriteData(pBBEP, u8Cache, 1);
        bbepWriteCmd(pBBEP, 0xcf);
        u8Cache[0] = 0x0f; u8Cache[1] = 0x01;
        bbepWriteData(pBBEP, u8Cache, 2);
    } else {
        if (ucCMD == 0x24) {
        bbepWriteCmd(pBBEP, 0x11);
        u8Cache[0] = 0x5;
        bbepWriteData(pBBEP, u8Cache, 1);
        bbepWriteCmd(pBBEP, 0x44);
        u8Cache[0] = 0; u8Cache[1] = 0x31;
        bbepWriteData(pBBEP, u8Cache, 2);
        bbepWriteCmd(pBBEP, 0x45);
        u8Cache[0] = 0x0f; u8Cache[1] = 0x1; u8Cache[2] = u8Cache[3] = 0;
        bbepWriteData(pBBEP, u8Cache, 4);
        }
        bbepWriteCmd(pBBEP, 0x4e);
        u8Cache[0] = 0;
        bbepWriteData(pBBEP, u8Cache, 1);
        bbepWriteCmd(pBBEP, 0x4f);
        u8Cache[0] = 0x0f; u8Cache[1] = 0x01;
        bbepWriteData(pBBEP, u8Cache, 2);
    }
    bbepWaitBusy(pBBEP);
    bbepWriteCmd(pBBEP, ucCMD); // start write
    if (!pBuffer) { // just write 00s
        memset(u8Cache, 0, 272);
        for (tx=0; tx<400/8; tx++) {
            bbepWriteData(pBBEP, u8Cache, 272);
        }
        return;
    }
    // Convert the bit direction and write the data to the EPD
    switch (pBBEP->iOrientation) {
        case 0:
            for (tx=0; tx<400/8; tx++) {
                d = u8Cache;
                s = &pBuffer[tx];
                for (ty=0; ty<272; ty++) {
                    *d++ = *s;
                    s += iPitch;
                }
                if (ucInvert) InvertBytes(d, 272);
                bbepWriteData(pBBEP, u8Cache, 272);
            } // for ty
            break;
        case 90:
            for (tx=0; tx<pBBEP->width; tx++) {
                d = u8Cache;
                // need to pick up and reassemble every pixel
                ucDstMask = 0x80;
                uc = 0xff;
                ucSrcMask = 0x80 >> (tx & 7);
                for (ty=pBBEP->native_height-1; ty>=0; ty--) {
                    s = &pBuffer[(tx >> 3) + (ty * iPitch)];
                    if ((s[0] & ucSrcMask) == 0) uc &= ~ucDstMask;
                    ucDstMask >>= 1;
                    if (ucDstMask == 0) {
                        *d++ = (uc ^ ucInvert);
                        ucDstMask = 0x80;
                        uc = 0xff;
                    }
                } // for ty
                *d++ = (uc ^ ucInvert); // store final partial byte
                bbepWriteData(pBBEP, u8Cache, (pBBEP->native_width+7)/8);
            } // for tx
            break;
        case 180:
            for (ty=pBBEP->native_height-1; ty>=0; ty--) {
                d = u8Cache;
                s = &pBuffer[ty * iPitch];
                for (tx=iPitch-1; tx>=0; tx--) {
                    *d++ = (ucMirror[s[tx]] ^ ucInvert);
                } // for tx
                bbepWriteData(pBBEP, u8Cache, iPitch);
            } // for ty
            break;
        case 270:
            for (tx=pBBEP->width-1; tx>=0; tx--) {
                d = u8Cache;
                // reassemble every pixel
                ucDstMask = 0x80;
                uc = 0xff;
                ucSrcMask = 0x80 >> (tx & 7);
                for (ty=0; ty<pBBEP->native_height; ty++) {
                    s = &pBuffer[(tx>>3) + (ty * iPitch)];
                    if ((s[0] & ucSrcMask) == 0) uc &= ~ucDstMask;
                    ucDstMask >>= 1;
                    if (ucDstMask == 0) {
                        *d++ = (uc ^ ucInvert);
                        ucDstMask = 0x80;
                        uc = 0xff;
                    }
                } // for ty
                *d++ = (uc ^ ucInvert); // store final partial byte
                bbepWriteData(pBBEP, u8Cache, (pBBEP->native_width+7)/8);
            } // for x
            break;
    } // switch on orientation
} /* bbepWriteHalf() */

//
// Write Image data (1-bpp entire plane) from RAM to the e-paper
// Rotate the pixels if necessary
//
static void bbepWriteImage(BBEPDISP *pBBEP, uint8_t ucCMD, uint8_t *pBuffer, int bInvert)
{
    int tx, ty;
    uint8_t *s, *d, ucSrcMask, ucDstMask, uc;
    uint8_t ucInvert = 0;
    int iPitch;
    
    iPitch = (pBBEP->width + 7) >> 3;
    if (bInvert) {
        ucInvert = 0xff; // red logic is inverted
    }
    if (ucCMD) {
        bbepWriteCmd(pBBEP, ucCMD); // start write
    }
    // Convert the bit direction and write the data to the EPD
    switch (pBBEP->iOrientation) {
        case 0:
            for (ty=0; ty<pBBEP->native_height; ty++) {
                d = u8Cache;
                s = &pBuffer[ty * iPitch];
                memcpy(d, s, iPitch);
                if (ucInvert) InvertBytes(d, iPitch);
                bbepWriteData(pBBEP, u8Cache, iPitch);
            } // for ty
            break;
        case 90:
            for (tx=0; tx<pBBEP->width; tx++) {
                d = u8Cache;
                // need to pick up and reassemble every pixel
                ucDstMask = 0x80;
                uc = 0xff;
                ucSrcMask = 0x80 >> (tx & 7);
                for (ty=pBBEP->height-1; ty>=0; ty--) {
                    s = &pBuffer[(tx >> 3) + (ty * iPitch)];
                    if ((s[0] & ucSrcMask) == 0) uc &= ~ucDstMask;
                    ucDstMask >>= 1;
                    if (ucDstMask == 0) {
                        *d++ = (uc ^ ucInvert);
                        ucDstMask = 0x80;
                        uc = 0xff;
                    }
                } // for ty
                *d++ = (uc ^ ucInvert); // store final partial byte
                bbepWriteData(pBBEP, u8Cache, (pBBEP->native_width+7)/8);
            } // for tx
            break;
        case 180:
            for (ty=pBBEP->native_height-1; ty>=0; ty--) {
                d = u8Cache;
                s = &pBuffer[ty * iPitch];
                for (tx=iPitch-1; tx>=0; tx--) {
                    *d++ = (ucMirror[s[tx]] ^ ucInvert);
                } // for tx
                bbepWriteData(pBBEP, u8Cache, iPitch);
            } // for ty
            break;
        case 270:
            for (tx=pBBEP->width-1; tx>=0; tx--) {
                d = u8Cache;
                // reassemble every pixel
                ucDstMask = 0x80;
                uc = 0xff;
                ucSrcMask = 0x80 >> (tx & 7);
                for (ty=0; ty<pBBEP->height; ty++) {
                    s = &pBuffer[(tx>>3) + (ty * iPitch)];
                    if ((s[0] & ucSrcMask) == 0) uc &= ~ucDstMask;
                    ucDstMask >>= 1;
                    if (ucDstMask == 0) {
                        *d++ = (uc ^ ucInvert);
                        ucDstMask = 0x80;
                        uc = 0xff;
                    }
                } // for ty
                *d++ = (uc ^ ucInvert); // store final partial byte
                bbepWriteData(pBBEP, u8Cache, (pBBEP->native_width+7)/8);
            } // for x
            break;
    } // switch on orientation
} /* bbepWriteImage() */
//
// Write a region of the local framebuffer to the eink display
// it will be rounded to byte boundaries and must be in the native orientation
// of the display.
//
void bbepWriteRegion(BBEPDISP *pBBEP, int16_t x, int16_t y, int16_t w, int16_t h, int plane)
{
uint8_t *s;
int ty, iPitch;

    if (pBBEP == NULL || x < 0 || y < 0 || x + w >  pBBEP->native_width || y + h > pBBEP->native_height) return;
    if (pBBEP->iFlags) return; // DEBUG - only 1-bpp support for now
    iPitch = (pBBEP->native_width+7)/8;
    bbepSetAddrWindow(pBBEP, x, y, w, h);
    bbepStartWrite(pBBEP, plane);
    s = pBBEP->ucScreen;
    s += (iPitch * y);
    s += x/8;
    for (ty=0; ty<h; ty++) {
        bbepWriteData(pBBEP, s, (w+7)/8);
        s += iPitch; 
    }
} /* bbepWriteRegion() */

//
// Write the local copy of the memory plane(s) to the eink's internal framebuffer
//
int bbepWritePlane(BBEPDISP *pBBEP, int iPlane, int bInvert)
{
    uint8_t ucCMD1, ucCMD2;
    int iOffset;
    
    if (pBBEP == NULL || pBBEP->ucScreen == NULL || iPlane < PLANE_0 || iPlane > PLANE_FALSE_DIFF) {
        return BBEP_ERROR_BAD_PARAMETER;
    }
// Some panels need to be told their resolution before you can write data
// to the internal framebuffer (e.g. 4.26" 800x480), otherwise the writes
// are ignored.
    if (pBBEP->iFlags & BBEP_NEEDS_EXTRA_INIT) {
        bbepSendCMDSequence(pBBEP, pBBEP->pInitFull);
    }

    if (pBBEP->native_width != 792) { // special case of split buffer
        bbepSetAddrWindow(pBBEP, 0,0, pBBEP->native_width, pBBEP->native_height);
    }

    if (pBBEP->iFlags & BBEP_4BPP_DATA) { // special case for some 2 and 3-color panels
        if (pBBEP->iFlags & BBEP_3COLOR) {
            bbepWriteImage4bppSpecial(pBBEP, 0x10);
        } else {
            bbepWriteImage1to4bpp(pBBEP, 0x10, pBBEP->ucScreen, bInvert);
        }
        return BBEP_SUCCESS;
    }
    if (pBBEP->iFlags & BBEP_7COLOR) {
        if (pBBEP->iFlags & BBEP_SPLIT_BUFFER) { // dual cable EPD
           bbepWriteImage4bppDual(pBBEP, 0x10);
        } else {
           bbepWriteImage4bpp(pBBEP, 0x10);
        }
        return BBEP_SUCCESS;
    }
    if (pBBEP->iFlags & BBEP_4COLOR) { // 4-color only has 1 way to go
        bbepWriteImage2bpp(pBBEP, 0x10);
        return BBEP_SUCCESS;
    }
    if (pBBEP->chip_type == BBEP_CHIP_UC81xx) {
        if (pBBEP->iFlags & BBEP_RED_SWAPPED) {
            ucCMD1 = UC8151_DTM1;
            ucCMD2 = UC8151_DTM2;
        } else {
            ucCMD1 = UC8151_DTM2;
            ucCMD2 = UC8151_DTM1;
        }
    } else {
        ucCMD1 = SSD1608_WRITE_RAM;
        ucCMD2 = SSD1608_WRITE_ALTRAM;
    }
    iOffset = ((pBBEP->native_width+7)>>3) * pBBEP->native_height;
    if (pBBEP->iFlags & BBEP_3COLOR && iPlane == PLANE_DUPLICATE) {
        iPlane = PLANE_BOTH;
    } 
    if (pBBEP->native_width == 792) { // special case
        switch (iPlane) {
            case PLANE_0:
                bbepWriteHalf(pBBEP, ucCMD1, pBBEP->ucScreen, bInvert);
                bbepWriteHalf(pBBEP, ucCMD2, NULL, 0);
                bbepWriteHalf(pBBEP, ucCMD1 | 0x80, pBBEP->ucScreen, bInvert);
                bbepWriteHalf(pBBEP, ucCMD2 | 0x80, NULL, 0);
                break;
            case PLANE_1:
                bbepWriteHalf(pBBEP, ucCMD2, &pBBEP->ucScreen[iOffset], bInvert);
                bbepWriteHalf(pBBEP, ucCMD2 | 0x80, &pBBEP->ucScreen[iOffset], bInvert);    
                break;
            case PLANE_0_TO_1:
                bbepWriteHalf(pBBEP, ucCMD2, pBBEP->ucScreen, bInvert);
                bbepWriteHalf(pBBEP, ucCMD2 | 0x80, pBBEP->ucScreen, bInvert);
                break;
            case PLANE_BOTH:
                bbepWriteHalf(pBBEP, ucCMD1, pBBEP->ucScreen, bInvert);
                if (pBBEP->iFlags & BBEP_HAS_SECOND_PLANE) {
                    bbepWriteHalf(pBBEP, ucCMD2, &pBBEP->ucScreen[iOffset], bInvert);
                } else { // write 0's if no plane memory
                    bbepWriteHalf(pBBEP, ucCMD2, NULL, 0); 
                }
                bbepWriteHalf(pBBEP, ucCMD1 | 0x80, pBBEP->ucScreen, bInvert);
                if (pBBEP->iFlags & BBEP_HAS_SECOND_PLANE) {
                    bbepWriteHalf(pBBEP, ucCMD2 | 0x80, &pBBEP->ucScreen[iOffset], bInvert);
                } else { // write 0's if no plane memory
                    bbepWriteHalf(pBBEP, ucCMD2 | 0x80, NULL, 0);
                }
                break;
            case PLANE_DUPLICATE:
                bbepWriteHalf(pBBEP, ucCMD1, pBBEP->ucScreen, bInvert);
                bbepWriteHalf(pBBEP, ucCMD1 | 0x80, pBBEP->ucScreen, bInvert);
                bbepWriteHalf(pBBEP, ucCMD2, pBBEP->ucScreen, bInvert);
                bbepWriteHalf(pBBEP, ucCMD2 | 0x80, pBBEP->ucScreen, bInvert);
                break;

            case PLANE_FALSE_DIFF: // send inverted image to 'old' plane
                bbepWriteHalf(pBBEP, ucCMD1, pBBEP->ucScreen, 0);
                bbepWriteHalf(pBBEP, ucCMD1 | 0x80, pBBEP->ucScreen, 0);
                bbepWriteHalf(pBBEP, ucCMD2, pBBEP->ucScreen, 1);
                bbepWriteHalf(pBBEP, ucCMD2 | 0x80, pBBEP->ucScreen, 1);
                break;
        } // switch on plane option
    } else { // 'normal' panels
        switch (iPlane) {
            case PLANE_0:
                bbepWriteImage(pBBEP, ucCMD1, pBBEP->ucScreen, bInvert);
                break;
            case PLANE_1:
                bbepWriteImage(pBBEP, ucCMD2, &pBBEP->ucScreen[iOffset], bInvert);
                break;
            case PLANE_0_TO_1:
                bbepWriteImage(pBBEP, ucCMD2, pBBEP->ucScreen, bInvert);
                break;
            case PLANE_BOTH:
                bbepWriteImage(pBBEP, ucCMD1, pBBEP->ucScreen, bInvert);
                if (pBBEP->iFlags & BBEP_HAS_SECOND_PLANE) {
                    bbepWriteImage(pBBEP, ucCMD2, &pBBEP->ucScreen[iOffset], bInvert);
                }
                break;
            case PLANE_DUPLICATE:
                bbepWriteImage(pBBEP, ucCMD1, pBBEP->ucScreen, bInvert);
                bbepWriteImage(pBBEP, ucCMD2, pBBEP->ucScreen, bInvert);
                break;

            case PLANE_FALSE_DIFF: // send inverted image to 'old' plane
                bbepWriteImage(pBBEP, ucCMD1, pBBEP->ucScreen, 0);
                bbepWriteImage(pBBEP, ucCMD2, pBBEP->ucScreen, 1);
                break;
        } // switch on plane option
    } // normal panels
    return BBEP_SUCCESS;
} /* bbepWritePlane() */

#endif // __BB_EP__
