#ifndef __PERIPH_H_
#define __PERIPH_H_

#include <driverlib/ioc.h>
#include <stdbool.h>
#include <stdint.h>

/* ---- Pin assignments ---- */

#define EPD_BS IOID_12
#define EPD_BUSY IOID_13
#define EPD_RST IOID_14
#define EPD_DC IOID_15
#define EPD_CS IOID_20
#define EPD_CLK IOID_10
#define EPD_MOSI IOID_9

#define FLASH_MOSI IOID_9
#define FLASH_MISO IOID_8
#define FLASH_CLK IOID_10
#define FLASH_CS IOID_11

#define UART_TX IOID_3
#define UART_RX IOID_2

#define LED1 IOID_16
#define LED2 IOID_17

#define WAKEUP IOID_21

#define NFC_FD IOID_4
#define NFC_PWR IOID_5
#define NFC_SCL IOID_25
#define NFC_SDA IOID_24

/* ---- System ---- */

void periphInit(void);
void periphReInit(void);
void periphDeinit(void);
uint32_t millis(void);
void yield(void);
void delay(uint32_t ms);
void sleepStandby(uint32_t seconds);

/* ---- GPIO helpers ---- */

#define INPUT 0
#define INPUT_PULLUP 1
#define OUTPUT 2
#define DISABLED 3
#define HIGH 1
#define LOW 0

void pinMode(uint32_t ioid, int mode);
void digitalWrite(uint32_t ioid, int value);
int digitalRead(uint32_t ioid);

/* ---- SPI device context ---- */

typedef struct {
  uint32_t pinMosi;
  uint32_t pinMiso;
  uint32_t pinClk;
  uint32_t pinCs;
  uint32_t bitRate;
} SpiDevice;

bool spiOpen(SpiDevice* dev);
bool spiClose(SpiDevice* dev);
void spiSelect(SpiDevice* dev);
void spiDeselect(SpiDevice* dev);
bool spiTransfer(SpiDevice* dev, const uint8_t* wbuf, int wlen, uint8_t* rbuf, int rlen, int skipLen);
bool spiWrite(SpiDevice* dev, const uint8_t* buf, int len);
bool spiRead(SpiDevice* dev, uint8_t* buf, int len);

/* ---- UART ---- */

#ifdef UART_DEBUG
void uartInit(uint32_t baudRate);
void uartDeinit(void);
void uartPutChar(uint8_t c);
void uartPuts(const char* s);
#else
#define uartInit(baud) ((void)0)
#define uartDeinit() ((void)0)
#define uartPutChar(c) ((void)0)
#define uartPuts(s) ((void)0)
#endif

#endif /* __PERIPH_H_ */