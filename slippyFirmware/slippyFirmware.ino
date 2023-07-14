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
#define MAX_LOCAL_NODES 255
#define MAX_NETWORK_NODES 255

// helpers
#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

struct Message {
  byte type;
  int sender;
  int recipient;
  String data;
}

struct Node {
  int address;
}

Node localNodes[MAX_LOCAL_NODES];
Node networkNodes[MAX_NETWORK_NODES];

void setup() {
  randomSeed(analogRead(0));

  Serial.begin(9600);
  while (!Serial); //Remove for headless

  Serial.print("[info] Initializing LoRa module...");
  LoRa.setPins(DEVICE_CS, DEVICE_RESET, DEVICE_IRQ);

  if (!LoRa.begin(frequency)) {
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

  Serial.print("[info] Connecting to the network...");
  joinNetwork();

}

void loop() {
  // put your main code here, to run repeatedly:

}

void joinNetwork() {
  
}

void sendMessage(Message message) {
  LoRa_txMode();
  LoRa.beginPacket();

  LoRa.write(message.type);
  LoRa.write(message.sender);
  LoRa.write(message.recipient);

  LoRa.write(message.data.length());
  LoRa.print(message.data);

  LoRa.endPacket(true);
}