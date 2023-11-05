#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>

#include <ArduinoUniqueID.h>

#include <RHMesh.h>
#include <RH_RF95.h>

#define SLIPPY_LOCAL_BROADCAST_ADDRESS 0x000000ff
#define SLIPPY_GLOBAL_BROADCAST_ADDRESS 0xffffffff

#define SLIPPY_PACKET_TYPE_NORMAL 0
#define SLIPPY_PACKET_TYPE_CHAIN 1

#define SLIPPY_PACKET_SERVICE_TXT 0

#define SLIPPY_PACKET_DATA_SIZE 200

struct SlippyPacketNormal {
    uint8_t type = SLIPPY_PACKET_TYPE_NORMAL;
    uint8_t service = SLIPPY_PACKET_SERVICE_TXT;
    uint8_t flags = 0x00;
    uint32_t uid;

    uint8_t size;
    uint8_t data[SLIPPY_PACKET_DATA_SIZE];
};

struct SlippyPacketChain {
    uint8_t type = SLIPPY_PACKET_TYPE_CHAIN;
    uint8_t service = SLIPPY_PACKET_SERVICE_TXT;
    uint8_t flags = 0x00;
    uint32_t uid;

    uint8_t index;
    uint8_t count;
    uint8_t size;
    uint8_t data[SLIPPY_PACKET_DATA_SIZE];
};

void(* resetFunc)(void) = 0;

uint8_t sendMessaage(uint32_t address, uint8_t* data, uint8_t length, uint8_t service=SLIPPY_PACKET_SERVICE_TXT);
void printAddress(uint32_t address);

#endif