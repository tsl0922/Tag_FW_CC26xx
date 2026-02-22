#ifndef _EPD_H_
#define _EPD_H_

#include <stdint.h>

void epdInit(void);
void epdWrite(uint8_t* pData, int iLen);
void epdRefresh(void);
void epdDeinit(void);

uint32_t getImageSize(void);
void drawImageAtAddress(uint32_t addr, uint8_t lut);

void clearScreen(void);
void showSplashScreen(void);
void showAPFound(void);
void showNoAP(void);
void showLongTermSleep(void);

void ledBlink(int pin, int ms);

#endif
