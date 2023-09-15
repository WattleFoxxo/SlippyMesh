#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <EEPROM.h>

#include <RHRouter.h>
#include <RHMesh.h>
#include <RH_RF95.h>

#include <base64.hpp>

#define SETTING_PREFERED_ADDRESS 0 // Set this to your prefed address. Set to zero to disable. Addresses range from 1 to 254.
#define SETTING_AUTO_RESTART 0 // Automaticly restart your device. Set to zero to disable. Value is in minutes.

#define SLIPPY_NETWORK_VERSION 1
#define SLIPPY_PACKET_PING 0
#define SLIPPY_PACKET_DIRECT 1
#define SLIPPY_PACKET_BROADCAST 2
#define SLIPPY_MESSAGE_SIZE 200
#define SLIPPY_MAX_NODES 254

struct Packet {
    uint8_t network_version;
    uint8_t packet_type;
    uint8_t length;
    uint8_t data[SLIPPY_MESSAGE_SIZE];
};
const uint8_t SLIPPY_HEADER_SIZE = sizeof(Packet) - SLIPPY_MESSAGE_SIZE;

bool sendMessage(uint8_t address, uint8_t* message);
void serializePacket(const Packet& packet, uint8_t* byteArray);
void deserializePacket(const uint8_t* byteArray, Packet& packet);
void printBytes(uint8_t* byteArray, uint8_t len);
uint8_t findAvalabeAddress(uint8_t addr);

void cmdUnrecognized(String cmd);
void cmdHelp();
void cmdInfo();
void cmdSend();
void cmdSend64();

void alexD(uint8_t from);

void(* resetFunc)(void) = 0;

#endif