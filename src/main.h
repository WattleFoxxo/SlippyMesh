#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>

#include <avr/wdt.h>

#include <ArduinoUniqueID.h>
#include <base64.hpp>
#include <CommandParser.h>

#include <RHMesh.h>
#include <RH_RF95.h>

#include "commands.h"

// Settings
#define SLIPPY_RADIO_FREQUENCY 916.0
#define SLIPPY_RADIO_POWER 23
#define SLIPPY_RADIO_CAD_TIMEOUT 5000
#define SLIPPY_SERIAL_BUAD 115200
#define SLIPPY_SERIAL_WELCOME_MESSAGE "Type 'help' for help"
#define SLIPPY_CUSTOM_HEART_BEAT "No birds here :)"
#define SLIPPY_VIEW_HEART_BEAT_MESSAGES true

// DO NOT CHANGE
#define SLIPPY_VERSION "2024.02.15-2538b"
#define SLIPPY_BROADCAST_ADDRESS RH_BROADCAST_ADDRESS
#define SLIPPY_PACKET_TYPE_NORMAL 0
#define SLIPPY_PACKET_TYPE_CHAIN 1
#define SLIPPY_PACKET_SERVICE_TXT 0 // pain text NOTE: services 0-16 are reserved for slippy functions
#define SLIPPY_PACKET_SERVICE_EXE 1 // remote commands
#define SLIPPY_PACKET_SERVICE_ALIVE 2 // online nodes
#define SLIPPY_PACKET_SERVICE_OTA 3
#define SLIPPY_PACKET_DATA_SIZE 200
#define SLIPPY_PACKET_CHAIN_DATA_SIZE 128
#define SLIPPY_PACKET_FLAG_LOCAL_BROADCAST 0
#define SLIPPY_MAX_UID 32

struct SlippyPacket {
    uint8_t type = SLIPPY_PACKET_TYPE_NORMAL;
    uint8_t service = SLIPPY_PACKET_SERVICE_TXT; // NOTE: services 0-16 are reserved
    uint8_t flags = 0x00;
    uint32_t uid;

    uint8_t size;
    uint8_t data[SLIPPY_PACKET_DATA_SIZE];
};

struct SlippyChunk {
    uint32_t current;
    uint32_t total;
    uint32_t total_bytes;

    uint8_t size;
    uint8_t data[128];
};

const uint8_t SLIPPY_PACKET_SIZE = sizeof(SlippyPacket) - SLIPPY_PACKET_DATA_SIZE;

typedef CommandParser<10, 5, 10, 256, 64> MyCommandParser;

void reciveMessage();
uint8_t sendPacket(SlippyPacket packet, uint32_t address);
uint8_t sendPacket_(uint8_t* packet, uint8_t size, uint32_t address);

void heartBeat();

void printAddress(uint32_t address);
void printBuffer(uint8_t* buffer, uint8_t lenght, char* dilemma=", ");
void printStringBuffer(uint8_t* buffer, uint8_t lenght);
void printFlags(uint8_t flags);
void printPacket(SlippyPacket* newPacket, uint32_t from, uint32_t dest);

uint32_t random32bit();
void addUID(uint32_t uid);
bool hasUID(uint32_t uid);

bool getFlag(uint8_t flags, uint8_t bit);
void setFlag(uint8_t &flags, uint8_t bit, bool value);

char* getErrorString(uint8_t error);

void cmd_help(MyCommandParser::Argument *args, char *response);
void cmd_send(MyCommandParser::Argument *args, char *response);
void cmd_sendex(MyCommandParser::Argument *args, char *response);
void cmd_send64(MyCommandParser::Argument *args, char *response);

#endif