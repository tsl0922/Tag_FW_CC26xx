
#include "syncedproto.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eeprom.h"
#include "epd.h"
#include "periph.h"
#include "powermgt.h"
#include "radio.h"

extern void executeCommand(uint8_t cmd);

// clang-format off
uint8_t curImgSlot = 0xFF;
uint32_t curHighSlotId = 0;
uint8_t nextImgSlot = 0;
uint8_t imgSlots = 0;

// download-stuff
struct blockRequest curBlock = {0};     // used by the block-requester, contains the next request that we'll send
struct AvailDataInfo curDataInfo = {0}; // last 'AvailDataInfo' we received from the AP
bool requestPartialBlock = false;       // if we should ask the AP to get this block from the host or not
#define BLOCK_TRANSFER_ATTEMPTS 5

// stuff we need to keep track of related to the network/AP
uint8_t APmac[8] = {0};
uint16_t APsrcPan = 0;
uint8_t mSelfMac[8] = {0};

uint8_t seq = 0;
uint8_t currentChannel = 0;

// buffer we use to prepare/read packets
static uint8_t inBuffer[128] = {0};
static uint8_t outBuffer[128] = {0};
// clang-format on

// tools
static uint8_t getPacketType(const void* buffer) {
  const struct MacFcs* fcs = buffer;
  if ((fcs->frameType == 1) && (fcs->destAddrType == 2) && (fcs->srcAddrType == 3) && (fcs->panIdCompressed == 0)) {
    // broadcast frame
    uint8_t type = ((uint8_t*)buffer)[sizeof(struct MacFrameBcast)];
    return type;
  } else if ((fcs->frameType == 1) && (fcs->destAddrType == 3) && (fcs->srcAddrType == 3) &&
             (fcs->panIdCompressed == 1)) {
    // normal frame
    uint8_t type = ((uint8_t*)buffer)[sizeof(struct MacFrameNormal)];
    return type;
  }
  return 0;
}
static bool pktIsUnicast(const void* buffer) {
  const struct MacFcs* fcs = buffer;
  if ((fcs->frameType == 1) && (fcs->destAddrType == 2) && (fcs->srcAddrType == 3) && (fcs->panIdCompressed == 0)) {
    return false;
  } else if ((fcs->frameType == 1) && (fcs->destAddrType == 3) && (fcs->srcAddrType == 3) &&
             (fcs->panIdCompressed == 1)) {
    // normal frame
    return true;
  }
  // unknown type...
  return false;
}

static bool checkCRC(const void* p, const uint8_t len) {
  uint8_t total = 0;
  for (uint8_t c = 1; c < len; c++) {
    total += ((uint8_t*)p)[c];
  }
  // printf("CRC: rx %d, calc %d\r\n", ((uint8_t *)p)[0], total);
  return ((uint8_t*)p)[0] == total;
}

static void addCRC(void* p, const uint8_t len) {
  uint8_t total = 0;
  for (uint8_t c = 1; c < len; c++) {
    total += ((uint8_t*)p)[c];
  }
  ((uint8_t*)p)[0] = total;
}

// radio stuff
static void sendPing(void) {
  struct MacFrameBcast* txframe = (struct MacFrameBcast*)(outBuffer + 1);
  memset(outBuffer, 0, sizeof(struct MacFrameBcast) + 2 + 4);
  outBuffer[0] = sizeof(struct MacFrameBcast) + 1 + 2;
  outBuffer[sizeof(struct MacFrameBcast) + 1] = PKT_PING;
  memcpy(txframe->src, mSelfMac, 8);
  txframe->fcs.frameType = 1;
  txframe->fcs.ackReqd = 1;
  txframe->fcs.destAddrType = 2;
  txframe->fcs.srcAddrType = 3;
  txframe->seq = seq++;
  txframe->dstPan = PROTO_PAN_ID;
  txframe->dstAddr = 0xFFFF;
  txframe->srcPan = PROTO_PAN_ID;
  radioTx(outBuffer);
}
uint8_t detectAP(const uint8_t channel) {
  for (uint8_t c = 1; c <= MAXIMUM_PING_ATTEMPTS; c++) {
    yield();
    sendPing();
    uint32_t timeout = millis() + PING_REPLY_WINDOW;
    while (millis() < timeout) {
      int8_t ret = radioRx(inBuffer, 128);
      if (ret > 1) {
        if ((inBuffer[sizeof(struct MacFrameNormal) + 1] == channel) && (getPacketType(inBuffer) == PKT_PONG)) {
          if (pktIsUnicast(inBuffer)) {
            struct MacFrameNormal* f = (struct MacFrameNormal*)inBuffer;
            memcpy(APmac, f->src, 8);
            APsrcPan = f->pan;
            return c;
          }
        }
      }
    }
  }
  return 0;
}

// data xfer stuff
static void sendShortAvailDataReq(void) {
  struct MacFrameBcast* txframe = (struct MacFrameBcast*)(outBuffer + 1);
  outBuffer[0] = sizeof(struct MacFrameBcast) + 1 + 2;
  outBuffer[sizeof(struct MacFrameBcast) + 1] = PKT_AVAIL_DATA_SHORTREQ;
  memcpy(txframe->src, mSelfMac, 8);
  outBuffer[1] = 0x21;
  outBuffer[2] = 0xC8;  // quickly set txframe fcs structure for broadcast packet
  txframe->seq = seq++;
  txframe->dstPan = PROTO_PAN_ID;
  txframe->dstAddr = 0xFFFF;
  txframe->srcPan = PROTO_PAN_ID;
  radioTx(outBuffer);
}

static void sendAvailDataReq(void) {
  struct MacFrameBcast* txframe = (struct MacFrameBcast*)(outBuffer + 1);
  memset(outBuffer, 0, sizeof(struct MacFrameBcast) + sizeof(struct AvailDataReq) + 2 + 4);
  struct AvailDataReq* availreq = (struct AvailDataReq*)(outBuffer + 2 + sizeof(struct MacFrameBcast));
  outBuffer[0] = sizeof(struct MacFrameBcast) + sizeof(struct AvailDataReq) + 2 + 2;
  outBuffer[sizeof(struct MacFrameBcast) + 1] = PKT_AVAIL_DATA_REQ;
  memcpy(txframe->src, mSelfMac, 8);
  txframe->fcs.frameType = 1;
  txframe->fcs.ackReqd = 1;
  txframe->fcs.destAddrType = 2;
  txframe->fcs.srcAddrType = 3;
  txframe->seq = seq++;
  txframe->dstPan = PROTO_PAN_ID;
  txframe->dstAddr = 0xFFFF;
  txframe->srcPan = PROTO_PAN_ID;
  // TODO: send some (more) meaningful data
  availreq->hwType = SOLUM_M2_BW_29;
  availreq->tagSoftwareVersion = 0;
  availreq->wakeupReason = wakeUpReason;
  availreq->capabilities = capabilities;
  availreq->batteryMv = batteryVoltage;
  availreq->temperature = temperature;
  availreq->lastPacketRSSI = mLastRSSI;
  availreq->lastPacketLQI = mLastLqi;
  availreq->currentChannel = currentChannel;
  addCRC(availreq, sizeof(struct AvailDataReq));
  radioTx(outBuffer);
}

struct AvailDataInfo* getAvailDataInfo(void) {
  for (uint8_t c = 0; c < DATA_REQ_MAX_ATTEMPTS; c++) {
    sendAvailDataReq();
    uint32_t timeout = millis() + DATA_REQ_RX_WINDOW_SIZE;
    while (millis() < timeout) {
      int8_t ret = radioRx(inBuffer, 128);
      if (ret > 1) {
        if (getPacketType(inBuffer) == PKT_AVAIL_DATA_INFO) {
          if (checkCRC(inBuffer + sizeof(struct MacFrameNormal) + 1, sizeof(struct AvailDataInfo))) {
            struct MacFrameNormal* f = (struct MacFrameNormal*)inBuffer;
            memcpy(APmac, f->src, 8);
            APsrcPan = f->pan;
            return (struct AvailDataInfo*)(inBuffer + sizeof(struct MacFrameNormal) + 1);
          }
        }
      }
    }
  }
  return NULL;
}

struct AvailDataInfo* getShortAvailDataInfo(void) {
  for (uint8_t c = 0; c < DATA_REQ_MAX_ATTEMPTS; c++) {
    sendShortAvailDataReq();
    uint32_t timeout = millis() + DATA_REQ_RX_WINDOW_SIZE;
    while (millis() < timeout) {
      int8_t ret = radioRx(inBuffer, 128);
      if (ret > 1) {
        if (getPacketType(inBuffer) == PKT_AVAIL_DATA_INFO) {
          if (checkCRC(inBuffer + sizeof(struct MacFrameNormal) + 1, sizeof(struct AvailDataInfo))) {
            struct MacFrameNormal* f = (struct MacFrameNormal*)inBuffer;
            memcpy(APmac, f->src, 8);
            APsrcPan = f->pan;
            return (struct AvailDataInfo*)(inBuffer + sizeof(struct MacFrameNormal) + 1);
          }
        }
      }
    }
  }
  return NULL;
}

static bool processBlockPart(const struct blockPart* bp, uint8_t* blockBuffer) {
  uint16_t start = bp->blockPart * BLOCK_PART_DATA_SIZE;
  uint16_t size = BLOCK_PART_DATA_SIZE;
  // validate if it's okay to copy data
  if (bp->blockId != curBlock.blockId) return false;
  if (start >= (BLOCK_XFER_BUFFER_SIZE - 1)) return false;
  if (bp->blockPart > BLOCK_MAX_PARTS) return false;
  if ((start + size) > BLOCK_XFER_BUFFER_SIZE) {
    size = BLOCK_XFER_BUFFER_SIZE - start;
  }
  if (checkCRC(bp, sizeof(struct blockPart) + BLOCK_PART_DATA_SIZE)) {
    //  copy block data to buffer
    memcpy((void*)(blockBuffer + start), (const void*)bp->data, size);
    // we don't need this block anymore, set bit to 0 so we don't request it
    // again
    curBlock.requestedParts[bp->blockPart / 8] &= ~(1 << (bp->blockPart % 8));
    return true;
  } else {
    printf("CRC Failed \r\n");
    return false;
  }
}

static void blockRxLoop(uint8_t* blockBuffer, const uint32_t timeout) {
  bool blockComplete = false;
  uint32_t t = millis() + (timeout + 20);
  do {
    int8_t ret = radioRx(inBuffer, 128);
    if (ret > 1) {
      if (getPacketType(inBuffer) == PKT_BLOCK_PART) {
        struct blockPart* bp = (struct blockPart*)(inBuffer + sizeof(struct MacFrameNormal) + 1);
        if (processBlockPart(bp, blockBuffer)) {
          // printf("Received block part %d\r\n", bp->blockPart);
        }
      } else {
        blockComplete = true;
        for (uint8_t c = 0; c < BLOCK_MAX_PARTS; c++) {
          if (curBlock.requestedParts[c / 8] & (1 << (c % 8))) {
            blockComplete = false;
            break;
          }
        }
      }
    }
  } while (millis() < t && !blockComplete);
}

static struct blockRequestAck* continueToRX(void) {
  struct blockRequestAck* ack = (struct blockRequestAck*)(inBuffer + sizeof(struct MacFrameNormal) + 1);
  ack->pleaseWaitMs = 0;
  return ack;
}

static void sendBlockRequest(void) {
  memset(outBuffer, 0, sizeof(struct MacFrameNormal) + sizeof(struct blockRequest) + 2 + 2);
  struct MacFrameNormal* f = (struct MacFrameNormal*)(outBuffer + 1);
  struct blockRequest* blockreq = (struct blockRequest*)(outBuffer + 2 + sizeof(struct MacFrameNormal));
  outBuffer[0] = sizeof(struct MacFrameNormal) + sizeof(struct blockRequest) + 2 + 2;
  if (requestPartialBlock) {
    outBuffer[sizeof(struct MacFrameNormal) + 1] = PKT_BLOCK_PARTIAL_REQUEST;
  } else {
    outBuffer[sizeof(struct MacFrameNormal) + 1] = PKT_BLOCK_REQUEST;
  }
  memcpy(f->src, mSelfMac, 8);
  memcpy(f->dst, APmac, 8);
  f->fcs.frameType = 1;
  f->fcs.secure = 0;
  f->fcs.framePending = 0;
  f->fcs.ackReqd = 0;
  f->fcs.panIdCompressed = 1;
  f->fcs.destAddrType = 3;
  f->fcs.frameVer = 0;
  f->fcs.srcAddrType = 3;
  f->seq = seq++;
  f->pan = APsrcPan;
  memcpy(blockreq, &curBlock, sizeof(struct blockRequest));
  addCRC(blockreq, sizeof(struct blockRequest));
  radioTx(outBuffer);
}

static struct blockRequestAck* performBlockRequest(void) {
  for (uint8_t c = 0; c < 30; c++) {
    sendBlockRequest();
    uint32_t timeout = millis() + DATA_REQ_RX_WINDOW_SIZE;
    do {
      int8_t ret = radioRx(inBuffer, 128);
      if (ret > 1) {
        switch (getPacketType(inBuffer)) {
          case PKT_BLOCK_REQUEST_ACK:
            if (checkCRC((inBuffer + sizeof(struct MacFrameNormal) + 1), sizeof(struct blockRequestAck)))
              return (struct blockRequestAck*)(inBuffer + sizeof(struct MacFrameNormal) + 1);
            break;
          case PKT_BLOCK_PART:
            return continueToRX();
            break;
          case PKT_CANCEL_XFER:
            return NULL;
          default:
            printf("pkt w/type %02X\r\n", getPacketType(inBuffer));
            break;
        }
      }
    } while (millis() < timeout);
  }
  return continueToRX();
}

static void sendXferCompletePacket(void) {
  memset(outBuffer, 0, sizeof(struct MacFrameNormal) + 2 + 4);
  struct MacFrameNormal* f = (struct MacFrameNormal*)(outBuffer + 1);
  outBuffer[0] = sizeof(struct MacFrameNormal) + 2 + 2;
  outBuffer[sizeof(struct MacFrameNormal) + 1] = PKT_XFER_COMPLETE;
  memcpy(f->src, mSelfMac, 8);
  memcpy(f->dst, APmac, 8);
  f->fcs.frameType = 1;
  f->fcs.secure = 0;
  f->fcs.framePending = 0;
  f->fcs.ackReqd = 0;
  f->fcs.panIdCompressed = 1;
  f->fcs.destAddrType = 3;
  f->fcs.frameVer = 0;
  f->fcs.srcAddrType = 3;
  f->pan = APsrcPan;
  f->seq = seq++;
  radioTx(outBuffer);
}

static void sendXferComplete(void) {
  for (uint8_t c = 0; c < 16; c++) {
    sendXferCompletePacket();
    uint32_t timeout = millis() + DATA_REQ_RX_WINDOW_SIZE;
    while (millis() < timeout) {
      int8_t ret = radioRx(inBuffer, 128);
      if (ret > 1) {
        if (getPacketType(inBuffer) == PKT_XFER_COMPLETE_ACK) {
          printf("XFC ACK\r\n");
          return;
        }
      }
    }
  }
  printf("XFC NACK!\r\n");
  return;
}

// EEprom related stuff
void saveCurImageSlot(uint8_t slot) {
  if (!eepromErase(EEPROM_CUR_SLOT_ADDR, FLASH_SECTOR_SIZE)) {
    printf("Failed to erase cur-slot sector\r\n");
    return;
  }
  if (!eepromWrite(EEPROM_CUR_SLOT_ADDR, &slot, 1)) {
    printf("Failed to write current image slot\r\n");
  }
}

uint8_t readCurImageSlot(void) {
  uint8_t slot = 0xFF;
  eepromRead(EEPROM_CUR_SLOT_ADDR, &slot, 1);
  return slot;
}

static uint32_t getAddressForSlot(const uint8_t s) { return EEPROM_IMG_START + (EEPROM_IMG_EACH * s); }

static void getNumSlots(void) {
  uint32_t eeSize = eepromGetSize();
  uint16_t nSlots = eeSize / (EEPROM_IMG_EACH >> 8) >> 8;
  if (eeSize < EEPROM_IMG_EACH || !nSlots) {
    printf("eeprom is too small\r\n");
    while (1);
  } else if (nSlots >> 8) {
    printf("eeprom is too big, some will be unused\r\n");
    imgSlots = 254;
  } else
    imgSlots = nSlots;
  printf("eeprom size=%ld, slots=%d\r\n", eeSize, imgSlots);
}

static uint8_t findSlotVer(const uint64_t ver) {
  uint32_t markerValid = EEPROM_IMG_VALID;
  printf("findSlotVer: looking for ver=%08lX%08lX\n", (uint32_t)(ver >> 32), (uint32_t)ver);
  for (uint8_t c = 0; c < imgSlots; c++) {
    struct EepromImageHeader eih;
    eepromRead(getAddressForSlot(c), (uint8_t*)&eih, sizeof(struct EepromImageHeader));
    if (!memcmp(&eih.validMarker, &markerValid, 4)) {
      if (eih.version == ver) {
        printf("  match in slot %d\n", c);
        return c;
      }
    }
  }
  return 0xFF;
}

uint8_t findSlotDataTypeArg(uint8_t arg) {
  arg &= (0xF8);  // unmatch with the 'preload' bit and LUT bits
  for (uint8_t c = 0; c < imgSlots; c++) {
    struct EepromImageHeader eih;
    eepromRead(getAddressForSlot(c), (uint8_t*)&eih, sizeof(struct EepromImageHeader));
    if (eih.validMarker == EEPROM_IMG_VALID) {
      if ((eih.argument & 0xF8) == arg) {
        return c;
      }
    }
  }
  return 0xFF;
}

uint8_t getEepromImageDataArgument(const uint8_t slot) {
  struct EepromImageHeader eih;
  eepromRead(getAddressForSlot(slot), (uint8_t*)&eih, sizeof(struct EepromImageHeader));
  return eih.argument;
}

static uint32_t getHighSlotId(void) {
  uint32_t temp = 0;
  uint32_t markerValid = EEPROM_IMG_VALID;
  for (uint8_t c = 0; c < imgSlots; c++) {
    struct EepromImageHeader eih;
    eepromRead(getAddressForSlot(c), (uint8_t*)&eih, sizeof(struct EepromImageHeader));
    if (!memcmp(&eih.validMarker, &markerValid, 4)) {
      if (temp < eih.id) {
        temp = eih.id;
        nextImgSlot = c;
      }
    }
  }
  printf("found high id=%ld in slot %d\r\n", temp, nextImgSlot);
  return temp;
}

void eraseImageBlocks(void) {
  for (uint8_t c = 0; c < imgSlots; c++) {
    yield();
    eepromErase(getAddressForSlot(c), EEPROM_IMG_EACH);
  }

  // also erase the current slot marker
  eepromErase(EEPROM_CUR_SLOT_ADDR, FLASH_SECTOR_SIZE);
  curImgSlot = 0xFF;
}

static void saveImgBlockData(const uint8_t imgSlot, const uint8_t blockId, uint8_t* blockBuffer) {
  uint32_t length = EEPROM_IMG_EACH - (sizeof(struct EepromImageHeader) + (blockId * BLOCK_DATA_SIZE));
  if (length > FLASH_SECTOR_SIZE) length = FLASH_SECTOR_SIZE;

  if (!eepromWrite(getAddressForSlot(imgSlot) + sizeof(struct EepromImageHeader) + (blockId * BLOCK_DATA_SIZE),
                   blockBuffer + sizeof(struct blockData), length))
    printf("EEPROM write failed\r\n");
}

static bool validateBlockData(uint8_t* blockBuffer) {
  struct blockData* bd = (struct blockData*)blockBuffer;
  printf("expected len = %04X, checksum=%04X\r\n", bd->size, bd->checksum);
  if (bd->size > BLOCK_XFER_BUFFER_SIZE - sizeof(struct blockData)) {
    printf("Impossible data size: %d, we abort here\r\n", bd->size);
    return false;
  }
  uint16_t t = 0;
  for (uint16_t c = 0; c < bd->size; c++) {
    t += bd->data[c];
  }
  printf("Checked len = %04X, checksum=%04X\r\n", bd->size, t);
  return bd->checksum == t;
}

static bool getDataBlock(uint8_t* blockBuffer, const uint16_t blockSize) {
  uint8_t partsThisBlock = 0;
  uint8_t blockAttempts = 0;
  bool blockComplete = false;

  blockAttempts = BLOCK_TRANSFER_ATTEMPTS;
  if (blockSize == BLOCK_DATA_SIZE) {
    partsThisBlock = BLOCK_MAX_PARTS;
    memset(curBlock.requestedParts, 0xFF, BLOCK_REQ_PARTS_BYTES);
  } else {
    partsThisBlock = (sizeof(struct blockData) + blockSize) / BLOCK_PART_DATA_SIZE;
    if ((sizeof(struct blockData) + blockSize) % BLOCK_PART_DATA_SIZE) partsThisBlock++;
    memset(curBlock.requestedParts, 0x00, BLOCK_REQ_PARTS_BYTES);
    for (uint8_t c = 0; c < partsThisBlock; c++) {
      curBlock.requestedParts[c / 8] |= (1 << (c % 8));
    }
  }

  requestPartialBlock = false;  // this forces the AP to request the block data from the host

  while (blockAttempts--) {
    yield();
    printf("REQ block %d, blocks=%d\r\n", curBlock.blockId, partsThisBlock);
    struct blockRequestAck* ack = performBlockRequest();

    if (ack == NULL) {
      printf("Cancelled request\r\n");
      return false;
    }
    if (ack->pleaseWaitMs) {  // SLEEP - until the AP is ready with the data
      printf("Please wait %d ms\r\n", ack->pleaseWaitMs);
      delay(ack->pleaseWaitMs - 20);
    }

    blockRxLoop(blockBuffer, 295);

    // check if we got all the parts we needed, e.g: has the block been
    // completed?
    blockComplete = true;
    for (uint8_t c = 0; c < partsThisBlock; c++) {
      if (curBlock.requestedParts[c / 8] & (1 << (c % 8))) {
        blockComplete = false;
      }
    }

    if (blockComplete) {
      printf("- COMPLETE\r\n");
      if (validateBlockData(blockBuffer)) {
        printf("- Validated\r\n");
        // block download complete, validated
        return true;
      } else {
        for (uint8_t c = 0; c < partsThisBlock; c++) {
          curBlock.requestedParts[c / 8] |= (1 << (c % 8));
        }
        requestPartialBlock = false;
        printf("blk failed validation!\r\n");
      }
    } else {
      printf("- INCOMPLETE\r\n");
      // block incomplete, re-request a partial block
      requestPartialBlock = true;
    }
  }
  printf("failed getting block\r\n");
  return false;
}

static bool downloadImageDataToEEPROM(const struct AvailDataInfo* avail) {
  uint16_t imageSize = 0;
  // check if we already started the transfer of this information & haven't
  // completed it
  if (avail->dataVer == curDataInfo.dataVer && curDataInfo.dataSize) {
    // looks like we did. We'll carry on where we left off.
    printf("restarting image download");
    curImgSlot = nextImgSlot;
  } else {
    // go to the next image slot
    nextImgSlot++;
    if (nextImgSlot >= imgSlots) nextImgSlot = 0;
    curImgSlot = nextImgSlot;

    printf("Saving to image slot %d\r\n", curImgSlot);
    if (!eepromErase(getAddressForSlot(curImgSlot), EEPROM_IMG_EACH)) {
      printf("eeprom erase failed\r\n");
    }
    curBlock.blockId = 0;
    memcpy(&(curBlock.ver), &(avail->dataVer), 8);
    curBlock.type = avail->dataType;
    memcpy(&curDataInfo, (void*)avail, sizeof(struct AvailDataInfo));
    imageSize = curDataInfo.dataSize;
  }

  printf("Downloading image, size: %ld\r\n", curDataInfo.dataSize);

  uint8_t* blockBuffer = (uint8_t*)malloc(BLOCK_XFER_BUFFER_SIZE);
  if (blockBuffer == NULL) {
    printf("Failed to allocate block buffer\r\n");
    return false;
  }

  while (curDataInfo.dataSize) {
    yield();
    uint16_t dataRequestSize = curDataInfo.dataSize > BLOCK_DATA_SIZE ? BLOCK_DATA_SIZE : curDataInfo.dataSize;
    printf("Requesting block %d, size %d\r\n", curBlock.blockId, dataRequestSize);
    if (getDataBlock(blockBuffer, dataRequestSize)) {
      printf("downloaded block %d, size=%d\r\n", curBlock.blockId, dataRequestSize);
      saveImgBlockData(curImgSlot, curBlock.blockId, blockBuffer);
      curBlock.blockId++;
      curDataInfo.dataSize -= dataRequestSize;
    } else {
      free(blockBuffer);
      return false;
    }
  }

  // borrow the blockBuffer temporarily
  struct EepromImageHeader* eih = (struct EepromImageHeader*)blockBuffer;
  memcpy(&eih->version, &curDataInfo.dataVer, 8);
  eih->validMarker = EEPROM_IMG_VALID;
  eih->id = ++curHighSlotId;
  eih->size = imageSize;
  eih->dataType = curDataInfo.dataType;
  eih->argument = curDataInfo.dataTypeArgument;
  bool writeOk = eepromWrite(getAddressForSlot(curImgSlot), (uint8_t*)eih, sizeof(struct EepromImageHeader));
  if (!writeOk) printf("Failed to write image header\r\n");
  free(blockBuffer);
  return writeOk;
}

void drawImageFromEeprom(const uint8_t imgSlot, uint8_t lut) {
  printf(">> Drawing image from slot %d with LUT %d\r\n", imgSlot, lut);
  drawImageAtAddress(getAddressForSlot(imgSlot), lut);
  saveCurImageSlot(imgSlot);
}

bool processImageDataAvail(struct AvailDataInfo* avail) {
  struct imageDataTypeArgStruct arg = *((struct imageDataTypeArgStruct*)&avail->dataTypeArgument);
  printf("Arg: preload=%d, type=0x%02X, lut=%d\r\n", arg.preloadImage, arg.specialType, arg.lut);

  // check if the size sent can be contained in the image slot
  if (avail->dataSize > getImageSize()) {
    printf("Unable to save image, it's too big!\r\n");
    sendXferComplete();
    return false;
  }

  if (arg.preloadImage) {
    printf("Preloading image with type 0x%02X from arg 0x%02X\n", arg.specialType, avail->dataTypeArgument);
    switch (arg.specialType) {
      // regular image preload, there can be multiple of this type in the EEPROM
      case CUSTOM_IMAGE_NOCUSTOM:
        // check if a version of this already exists
        if (findSlotVer(avail->dataVer) != 0xFF) {
          sendXferComplete();
          return true;
        }
        break;
      case CUSTOM_IMAGE_SLIDESHOW:
        break;
      // check if a slot with this argument is already set; if so, erase. Only
      // one of each arg type should exist
      default: {
        uint8_t slot = findSlotDataTypeArg(avail->dataTypeArgument);
        if (slot != 0xFF) eepromErase(getAddressForSlot(slot), EEPROM_IMG_EACH);
      } break;
    }
    printf("downloading preload image...\n");
    if (downloadImageDataToEEPROM(avail)) {
      sendXferComplete();
      return true;
    } else {
      return false;
    }
  } else {
    // check if this download is currently displayed or active
    if (curDataInfo.dataSize == 0 && avail->dataVer == curDataInfo.dataVer) {
      // we've downloaded this already, we're guessing it's already displayed
      printf("currently shown image, send xfc\r\n");
      sendXferComplete();
      return true;
    }
    // check if we've seen this version before
    curImgSlot = findSlotVer(avail->dataVer);
    if (curImgSlot != 0xFF) {
      printf("found image in slot %d, skip download.\n", curImgSlot);
      // found a (complete)valid image slot for this version
      sendXferComplete();
      // mark as completed and draw from EEPROM
      memcpy(&curDataInfo, (void*)avail, sizeof(struct AvailDataInfo));
      curDataInfo.dataSize = 0;  // mark as transfer not pending
      drawImageFromEeprom(curImgSlot, arg.lut);
      return true;
    } else {
      // not found in cache, prepare to download
      printf(">> downloading image...\n");
      if (downloadImageDataToEEPROM(avail)) {
        printf(">> download complete!\r\n");
        sendXferComplete();

        drawImageFromEeprom(curImgSlot, arg.lut);
        return true;
      } else {
        return false;
      }
    }
  }
}

bool processAvailDataInfo(struct AvailDataInfo* avail) {
  printf("dataType: %d, dataTypeArgument: %d\r\n", avail->dataType, avail->dataTypeArgument);
  switch (avail->dataType) {
    case DATATYPE_IMG_BMP:
    case DATATYPE_IMG_DIFF:
    case DATATYPE_IMG_RAW_1BPP:
    case DATATYPE_IMG_RAW_2BPP:
    case DATATYPE_IMG_G5:
      return processImageDataAvail(avail);
    case DATATYPE_COMMAND_DATA:
      memcpy(&curDataInfo, (void*)avail, sizeof(struct AvailDataInfo));
      sendXferComplete();
      executeCommand(avail->dataTypeArgument);
      break;
    default:
      printf("Unknown data type %d\r\n", avail->dataType);
      break;
  }
  return true;
}

void initializeProto(void) {
  wakeUpReason = WAKEUP_REASON_FIRSTBOOT;
  capabilities |= CAPABILITY_HAS_LED;
  if (!eepromInit()) {
    printf("eeprom init failed\r\n");
  }
  getNumSlots();
  curHighSlotId = getHighSlotId();
  curImgSlot = readCurImageSlot();
  printf(">> Current image slot: %d\r\n", curImgSlot);
}