#ifndef _EEPROM_H_
#define _EEPROM_H_

#include <stdbool.h>
#include <stdint.h>

/* Flash geometry */
#define FLASH_PAGE_SIZE 256
#define FLASH_SECTOR_SIZE 4096

#define EEPROM_IMG_START (0x0000UL)
#define EEPROM_IMG_EACH (0x3000UL)
#define EEPROM_IMG_LEN (0x40000UL)

#define EEPROM_CUR_SLOT_ADDR (0x3F000UL)

bool eepromInit(void);
bool eepromRead(uint32_t addr, uint8_t* dst, uint32_t len);
bool eepromWrite(uint32_t addr, uint8_t* src, uint32_t len);
bool eepromErase(uint32_t addr, uint32_t len);
uint32_t eepromGetSize(void);

#define EEPROM_IMG_VALID (0x494d4721UL)

struct EepromImageHeader {
  uint64_t version;
  uint32_t validMarker;
  uint32_t size;
  uint8_t dataType;
  uint32_t id;
  uint8_t argument;
} __packed;

#endif