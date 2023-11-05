// #include "main.h"

// uint8_t incomingBuffer[RH_MESH_MAX_MESSAGE_LEN];
// uint8_t outgoingBuffer[RH_MESH_MAX_MESSAGE_LEN];

// RH_RF95 radio;
// RHMesh *manager;

// void setup() {
//     randomSeed(analogRead(0));
// 	Serial.begin(115200);
//     delay(500);

//     uint32_t uuid = ADDR;
//     // for (int i = 0; i < 4; i++) {
//     //     uuid |= (uint32_t)UniqueID8[i+4] << (i * 8);
//     // }

//     Serial.println(F("Starting Slippy Mesh..."));

//     Serial.print(F("Radio: "));
//     manager = new RHMesh(radio, uuid);
//     if (!manager->init()) {
//         Serial.println(F("FAIL"));
//         while(true);
//     }

//     radio.setTxPower(23, false);
//     radio.setFrequency(916.0);
//     radio.setCADTimeout(500);

//     Serial.println(F("READY"));

//     Serial.print(F("Mesh: "));

//     // myAddress = random(5);
//     // manager->setThisAddress(myAddress);

//     Serial.println(F("READY"));

//     Serial.print(F("Your address is: "));
//     printAddress(manager->thisAddress());
//     Serial.println();
//     Serial.println(manager->thisAddress(), DEC);
// }

// void loop() {
//     uint8_t len = sizeof(incomingBuffer);
//     uint32_t from;
//     if (manager->recvfromAckTimeout(incomingBuffer, &len, 500, &from)) {
//         Serial.print("[");
//         printAddress(from);
//         Serial.print("]: ");
//         Serial.println((char*)incomingBuffer);
//     }

//     if (Serial.available() > 0) {

//         uint8_t addressStr[10];
//         uint8_t addressStrLen = Serial.readBytesUntil(' ', addressStr, 10);
//         addressStr[addressStrLen] = '\0';
        
//         uint8_t message[128];
//         uint8_t messageLen = Serial.readBytesUntil('\n', message, 128);
//         message[messageLen] = '\0';
        
//         uint32_t address = strtoul(addressStr, NULL, 16);

//         Serial.print("Sending ... ");

//         uint8_t error = manager->sendtoWait(message, messageLen, address);
//         Serial.println(error);
//     }
// }

// uint8_t sendPacket(uint32_t address, uint8_t* packet, uint8_t length) {
//     uint8_t buffer[length];
//     return manager->sendtoWait(buffer, length, address);
// }

// void printAddress(uint32_t address) {
//     Serial.print("0x");
//     for (int i = 7; i >= 0; i--) {
//         uint8_t nibble = (address >> (i * 4)) & 0x0F;
//         Serial.print(nibble, HEX);
//     }
// }

#include "main.h"

uint8_t incomingBuffer[RH_MESH_MAX_MESSAGE_LEN];
uint8_t outgoingBuffer[RH_MESH_MAX_MESSAGE_LEN];

RH_RF95 radio;
RHMesh *manager;

void setup() {
    randomSeed(analogRead(0));
	Serial.begin(115200);
    delay(500);

    uint32_t uuid = 0;
    for (int i = 0; i < 4; i++) {
        uuid |= (uint32_t)UniqueID8[i+4] << (i * 8);
    }

    Serial.println(F("Starting Slippy Mesh..."));

    Serial.print(F("Radio: "));
    manager = new RHMesh(radio, uuid);
    if (!manager->init()) {
        Serial.println(F("FAIL"));
        while(true);
    }

    radio.setTxPower(23, false);
    radio.setFrequency(916.0);
    radio.setCADTimeout(500);

    Serial.println(F("READY"));

    Serial.print(F("Mesh: "));

    Serial.println(F("READY"));

    Serial.print(F("Your address is: "));
    printAddress(manager->thisAddress());
    Serial.println();
    Serial.println(manager->thisAddress(), DEC);
}

void loop() {
    uint8_t len = sizeof(incomingBuffer);
    uint32_t from;
    if (manager->recvfromAckTimeout(incomingBuffer, &len, 500, &from)) {
        incomingBuffer[len] = '/0';
        Serial.print("[");
        printAddress(from);
        Serial.print("]: ");
        Serial.println((char*)incomingBuffer);
    }

    if (Serial.available() > 0) {

        uint8_t addressStr[10];
        uint8_t addressStrLen = Serial.readBytesUntil(' ', addressStr, 10);
        addressStr[addressStrLen] = '\0';
        
        uint8_t message[SLIPPY_PACKET_DATA_SIZE];
        uint8_t messageLen = Serial.readBytesUntil('\n', message, SLIPPY_PACKET_DATA_SIZE);
        message[messageLen] = '\0';
        
        uint32_t address = strtoul(addressStr, NULL, 16);

        Serial.print(messageLen, DEC);

        uint8_t error = sendMessaage(address, message, messageLen);
        Serial.println(error);
    }
}

uint8_t sendMessaage(uint32_t address, uint8_t* data, uint8_t length, uint8_t service=SLIPPY_PACKET_SERVICE_TXT) {

    SlippyPacketNormal newPacket;

    newPacket.service = service;
    newPacket.uid = random(0xffffffff);
    newPacket.size = length;
    memcpy(newPacket.data, data, length);

    uint8_t size = (sizeof(newPacket)-200)+length;

    Serial.println(size);

    uint8_t buffer[size];
    memcpy(buffer, &newPacket, size);

    Serial.println((char*)buffer);
    // Serial.println(sizeof(newPacket), DEC);
    // Serial.println(length, DEC);

    return manager->sendtoWait(buffer, sizeof(newPacket), address);
}

void printAddress(uint32_t address) {
    Serial.print("0x");
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (address >> (i * 4)) & 0x0F;
        Serial.print(nibble, HEX);
    }
}