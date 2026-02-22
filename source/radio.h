#ifndef _RADIO_H_
#define _RADIO_H_

#include <stdbool.h>
#include <stdint.h>

extern uint8_t channelList[6];
extern uint8_t mLastLqi;
extern uint8_t mLastRSSI;

bool radioInit(uint8_t* mac);
bool radioEnable(uint_fast8_t channel);
bool radioSetChannel(uint_fast8_t channel);
bool radioDisable(void);
int32_t radioRx(uint8_t* dstBuf, uint32_t maxLen);
bool radioTx(uint8_t* pkt);

#endif