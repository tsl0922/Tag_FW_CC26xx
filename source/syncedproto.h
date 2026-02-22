#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "oepl-definitions.h"
#include "oepl-proto.h"

extern uint8_t mSelfMac[];
extern uint8_t currentChannel;
extern uint8_t APmac[];

extern uint8_t wakeUpReason;
extern uint8_t curImgSlot;

struct AvailDataInfo* getAvailDataInfo(void);
struct AvailDataInfo* getShortAvailDataInfo(void);
uint8_t findSlotDataTypeArg(uint8_t arg);
uint8_t getEepromImageDataArgument(const uint8_t slot);
void eraseImageBlocks(void);
void drawImageFromEeprom(const uint8_t imgSlot, uint8_t lut);
bool processAvailDataInfo(struct AvailDataInfo* avail);
uint8_t detectAP(const uint8_t channel);
void initializeProto(void);
