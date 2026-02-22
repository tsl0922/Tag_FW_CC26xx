#ifndef __CONTIKI_IO__
#define __CONTIKI_IO__

#include "periph.h"

#define pgm_read_byte(a) *(uint8_t *)(a)
#define pgm_read_word(a) *(uint16_t *)(a)
#define pgm_read_dword(a) *(uint32_t *)(a)
#define memcpy_P memcpy

/* GPIO helpers (pinMode / digitalWrite / digitalRead) are provided by spi.h
 * and implemented in spi.c — do NOT redefine them here. */

/* SPI device context for the EPD display. */
static SpiDevice sEpdDev;

// forward references
void bbepWakeUp(BBEPDISP *pBBEP);

//
// Set the second CS pin for dual-controller displays
//
void bbepSetCS2(BBEPDISP *pBBEP, uint8_t cs) {} /* bbepSetCS2() */

//
// Initialize the GPIO pins and SPI for use by bb_eink
//
void bbepInitIO(BBEPDISP *pBBEP, uint8_t u8DC, uint8_t u8RST, uint8_t u8BUSY,
                uint8_t u8CS, uint8_t u8MOSI, uint8_t u8SCK,
                uint32_t u32Speed) {
  pBBEP->iDCPin = u8DC;
  pBBEP->iCSPin = u8CS;
  pBBEP->iMOSIPin = u8MOSI;
  pBBEP->iCLKPin = u8SCK;
  pBBEP->iRSTPin = u8RST;
  pBBEP->iBUSYPin = u8BUSY;

  pinMode(pBBEP->iDCPin, OUTPUT);
  pinMode(pBBEP->iRSTPin, OUTPUT);
  digitalWrite(pBBEP->iRSTPin, LOW);
  delay(100);
  digitalWrite(pBBEP->iRSTPin, HIGH);
  delay(100);
  if (pBBEP->iBUSYPin != 0xff) {
    pinMode(pBBEP->iBUSYPin, INPUT);
  }
  pBBEP->iSpeed = u32Speed;

  sEpdDev.pinMosi = u8MOSI;
  sEpdDev.pinMiso = IOID_UNUSED;
  sEpdDev.pinClk  = u8SCK;
  sEpdDev.pinCs   = u8CS;
  sEpdDev.bitRate = u32Speed;

  pBBEP->is_awake = 1;
} /* bbepInitIO() */

//
// Write a single byte as a COMMAND (D/C set low)
//
void bbepWriteCmd(BBEPDISP *pBBEP, uint8_t cmd) {
  if (!pBBEP->is_awake) {
    // if it's asleep, it can't receive commands
    bbepWakeUp(pBBEP);
    pBBEP->is_awake = 1;
  }
  digitalWrite(pBBEP->iDCPin, LOW);   /* command mode */
  spiOpen(&sEpdDev);
  spiSelect(&sEpdDev);
  spiWrite(&sEpdDev, &cmd, 1);
  spiDeselect(&sEpdDev);
  spiClose(&sEpdDev);
} /* bbepWriteCmd() */

//
// Write 1 or more bytes as DATA (D/C set high)
//
void bbepWriteData(BBEPDISP *pBBEP, uint8_t *pData, int iLen) {
  digitalWrite(pBBEP->iDCPin, HIGH);  /* data mode */
  spiOpen(&sEpdDev);
  spiSelect(&sEpdDev);
  spiWrite(&sEpdDev, pData, iLen);
  spiDeselect(&sEpdDev);
  spiClose(&sEpdDev);
} /* bbepWriteData() */

//
// Convenience function to write a command byte along with a data
// byte (it's single parameter)
//
void bbepCMD2(BBEPDISP *pBBEP, uint8_t cmd1, uint8_t cmd2) {
  bbepWriteCmd(pBBEP, cmd1);
  bbepWriteData(pBBEP, &cmd2, 1);
} /* bbepCMD2() */

#endif  // __CONTIKI_IO__
