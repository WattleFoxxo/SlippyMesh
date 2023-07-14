/*
  Slippy Firmware

Packet structure:
  Each "[]" is 1 byte
  Each "{}" has the max bytes of 64
  <[NETWORK_VERSION] [TYPE] [SENDER_HIGH] [SENDER_LOW] [RECIPIENT_HIGH] [RECIPIENT_LOW] {BUFFER}>

  Join Packet structure:
    BUFFER: { [TYPE] [ADDRESS_LOW] [ADDRESS_HIGH] }


*/


#include <SPI.h>
#include <LoRa.h>

// pin def
#define DEVICE_CS 10
#define DEVICE_RESET 9
#define DEVICE_IRQ 2

// lora config
#define BAND 915E6
#define SPREADING_FACTOR 7
#define SIGNAL_BANDWIDTH 125000
#define CODING_RATE 5
#define PREAMBLE_LENGTH 8
#define TX_POWER 13
#define SYNC_WORD 0xF3

// networking
#define MAX_BUFFER 64
#define NETWORK_VERSION 0

// packets types
#define PACKET_NULL 0
#define PACKET_DATA 1
#define PACKET_JOIN 2

// join types
#define JOIN_REQUEST 0
#define JOIN_ACCEPT 1
#define JOIN_REROLL 2
#define JOIN_DECLINE 3
#define JOIN_CHECK_ADDR_TAKEN 4

// permanent addresses
#define ADDR_EVERYONE 0
#define ADDR_JOINING 1

struct Packet {
  uint8_t type = 0;
  uint16_t sender = 0;
  uint16_t recipient = 0;
  uint16_t data[MAX_BUFFER];
};

struct Node {
  uint16_t address;
};

//Node networkNodes[];

Node localnode;

void setup() {
  Serial.begin(9600);
  while (!Serial); //Remove for headless

  Serial.println("[info] Initializing LoRa module...");
  LoRa.setPins(DEVICE_CS, DEVICE_RESET, DEVICE_IRQ);

  if (!LoRa.begin(BAND)) {
    Serial.println("[error] LoRa init failed. Check your connections.");
    while (true);
  }

  LoRa.setSpreadingFactor(SPREADING_FACTOR);
  LoRa.setSignalBandwidth(SIGNAL_BANDWIDTH);
  LoRa.setCodingRate4(CODING_RATE);
  LoRa.setPreambleLength(PREAMBLE_LENGTH);
  LoRa.setTxPower(TX_POWER);
  LoRa.setSyncWord(SYNC_WORD);
  LoRa.enableCrc();

  randomSeed(analogRead(0));

  Serial.println("[info] Connecting to the network...");
  joinNetwork();
}

void loop() {
  if (Serial.available()) {
    String inputString = Serial.readString();
    Serial.println("Sending: " + inputString);

    Packet dataPacket;
    dataPacket.type = PACKET_DATA;
    dataPacket.sender = localnode.address;
    dataPacket.recipient = ADDR_EVERYONE;

    inputString.toCharArray((char*)dataPacket.data, MAX_BUFFER);

    sendPacket(dataPacket);
  }
  /*if (millis() - lastSendTime > interval) {
    String message = "HeLoRa World!";   // send a message
    sendMessage(message);
    Serial.println("Sending " + message);
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 1000;    // 2-3 seconds
  }*/

  Packet packet = readPacket(LoRa.parsePacket());
  if (packet.type != PACKET_NULL) {
    switch (packet.type) {
      case PACKET_DATA:
        Serial.print("From: "+String(packet.sender, DEC)+", ");
        for (int i = 0; i < MAX_BUFFER; i++) {
          Serial.println((char)packet.data[i]);
        }
        break;
      case PACKET_JOIN:
        Packet joinPacket; // auto accept chang later
        joinPacket.type = PACKET_JOIN;
        joinPacket.sender = localnode.address;
        joinPacket.recipient = ADDR_JOINING;
        joinPacket.data[JOIN_ACCEPT];
        sendPacket(joinPacket);
        uint8_t addrLow = joinPacket.data[1]; //str2byte(packet.data, 1);
        uint8_t addrHigh = joinPacket.data[2]; //str2byte(packet.data, 2);
        Serial.println("[info] Node with id: "+String(byte2int(addrLow, addrHigh), DEC)+" has joind the network");
        break;
    }
  }
}

void joinNetwork() {
  Packet joinPacket;
  joinPacket.type = PACKET_JOIN;
  joinPacket.sender = ADDR_JOINING;
  joinPacket.recipient = ADDR_EVERYONE;

  // 0 = done, 1 = joining, 2 = reroll, 3 = declined
  uint16_t address;
  uint16_t lastSendTime;
  uint8_t attempts = 0;
  uint8_t status = 1;
  while (status != 0 && status != 3) {
    address = random(16, 65535);

    joinPacket.data[0] = JOIN_REQUEST;
    joinPacket.data[1] = lowByte(address);
    joinPacket.data[2] = highByte(address);

    //Serial.println(joinPacket.data, DEC);
    

    sendPacket(joinPacket);

    status = 1;

    lastSendTime = millis();

    //TODO: finish later
    while (status == 1) {
      Packet responce = readPacket(LoRa.parsePacket());
      if (responce.type == PACKET_JOIN) {
        switch (responce.data[0]) {
          case JOIN_ACCEPT:
            status = 0;
            break;
          case JOIN_REROLL:
            status = 2;
            break;
          case JOIN_DECLINE:
            status = 3;
            break;
        }
      }

      if (millis() - lastSendTime > 2000) {
        Serial.println("[info] Connection timed out, retrying... ("+String(attempts+1, DEC)+"/5)");
        attempts += 1;
        lastSendTime = millis();
      }

      if (attempts >= 1) {
        Serial.println("[error] Failed to connect to network. \n[info] Creating a new network.");
        status = 3;
      }
    }
  }

  if (status == 0) {
    Serial.println("[info] Connected to the network with id: "+String(address, DEC));
  }

  localnode.address = address;
}

void sendPacket(Packet packet) {
  LoRa.beginPacket();  

  LoRa.write(NETWORK_VERSION);
  LoRa.write(packet.type);

  LoRa.write(lowByte(packet.sender)); 
  LoRa.write(highByte(packet.sender));

  LoRa.write(lowByte(packet.recipient)); 
  LoRa.write(highByte(packet.recipient));

  for (int i = 0; i < MAX_BUFFER; i++) {
    LoRa.write(packet.data[i]);
  }

  //LoRa.write((uint8_t*)packet.data, MAX_BUFFER);

  LoRa.endPacket();
}

Packet readPacket(uint8_t packetSize) {
  if (packetSize == 0) {
    Packet emptyPacket;
    emptyPacket.type = PACKET_NULL;
    //emptyPacket.data = "Packet size is invalid";
    return emptyPacket;
  }

  uint8_t networkVer = LoRa.read();

  if (networkVer != NETWORK_VERSION) {
    Packet emptyPacket;
    emptyPacket.type = PACKET_NULL;
    //emptyPacket.data = "Network version is invalid";
    return emptyPacket;
  }

  uint8_t type = LoRa.read();
  uint8_t senderHigh = LoRa.read();
  uint8_t senderLow = LoRa.read();
  uint8_t recipientHigh = LoRa.read();
  uint8_t recipientLow = LoRa.read();
  //uint8_t length = LoRa.read();
  Packet packet;

  packet.type = type;
  packet.sender = byte2int(senderHigh, senderLow);
  packet.recipient = byte2int(recipientHigh, recipientLow);

  for (int i = 0; i < MAX_BUFFER; i++) {
    packet.data[i] = LoRa.read();
  }

  return packet;
}

uint16_t byte2int(uint8_t a, uint8_t b) {
  return ((b << 8) + a);
}

uint8_t str2byte(String str, uint16_t i) {
  return str.substring((i*2), (i*2)+2).toInt();
}

