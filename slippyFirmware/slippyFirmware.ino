#include <SPI.h>
#include <LoRa.h> /* https://github.com/sandeepmistry/arduino-LoRa */
#include <ArduinoUniqueID.h> /* https://github.com/ricaun/ArduinoUniqueID */

/*
ip of mnt talor 57.15.21.28
*/

#define LORA_CS 10
#define LORA_RST 9
#define LORA_IRQ 2
#define LORA_FRQ 915E6

#define MAX_BUFFER 128
#define MSG_ID_BUCKET_SIZE 16
#define NET_VERSION 2

const uint8_t BROADCAST_ADDRESS[4] = {255, 255, 255, 255};

struct Packet {
  uint8_t dest[4];
  uint8_t src[4];
  uint8_t msg_id[4];
  uint8_t buffer_len;
  uint8_t* buffer;
};

uint8_t localAddress[4];

uint8_t msgIdBucket[MSG_ID_BUCKET_SIZE][4];
uint8_t msgIdBucketPos = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("INIT"); // "Starting slippy.."

  randomSeed(analogRead(0));

  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);

  if (!LoRa.begin(LORA_FRQ)) {
    Serial.println("LORA_FAIL"); // "LoRa module failure!"
    while(true);
  }

  // LoRa.setSpreadingFactor(12);
  // LoRa.setSignalBandwidth(125E3);
  // LoRa.setCodingRate4(5);
  // LoRa.setTxPower(20);

  LoRa.setTxPower(20);                    // Supported values are 2 to 20
  LoRa.setSpreadingFactor(12);            // Supported values are between 6 and 12
  LoRa.setSignalBandwidth(125E3);         // Supported values are 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3.
  LoRa.setCodingRate4(8);                 // Supported values are between 5 and 8, these correspond to coding rates of 4/5 and 4/8
  // LoRa.setGain(6);                        // Supported values are between 0 and 6
  LoRa.enableCrc();

  getAddress(localAddress);
  
  Serial.print("READY "); // "Rady, Your address is: <address>""
  printArray(localAddress, ".", 4);
  Serial.print("\n");
}

void loop() {
  if (Serial.available() > 0) {
    char serialBuffer[512];
    char *token;

    byte numBytes = Serial.readBytesUntil('\n', serialBuffer, 512);
    token = strtok(serialBuffer, " ");

    Packet newPacket;
    
    for (int i = 0; i < 4; i++) {
      newPacket.dest[i] = atoi(strtok(NULL, " "));
    }

    for (int i = 0; i < 4; i++) {
      newPacket.src[i] = localAddress[i];
    }

    for (int i = 0; i < 4; i++) {
      newPacket.msg_id[i] = random(256);
    }
    
    newPacket.buffer_len = atoi(strtok(NULL, " "));

    char tempBuff[newPacket.buffer_len];
    for (int i = 0; i < newPacket.buffer_len; i++) {
      tempBuff[i] = (char)atoi(strtok(NULL, " "));
    }
    newPacket.buffer = tempBuff;

    sendPacket(newPacket);
  }

  readPacket(LoRa.parsePacket());
}

void sendPacket(Packet packet) {
  Serial.println("SENDING"); // "Sending..."
  LoRa.beginPacket();

  LoRa.write(NET_VERSION);

  for (size_t i = 0; i < 4; i++) {
    LoRa.write(packet.dest[i]);
  }

  for (size_t i = 0; i < 4; i++) {
    LoRa.write(packet.src[i]);
  }

  for (size_t i = 0; i < 4; i++) {
    LoRa.write(packet.msg_id[i]);
  }

  LoRa.write(packet.buffer_len);

  for (size_t i = 0; i < packet.buffer_len; i++) {
    LoRa.write(packet.buffer[i]);
  }

  LoRa.endPacket();
  Serial.println("SENT"); // "Sent."
}

void readPacket(int packetSize) {
  if (packetSize == 0) {
    return;
  }

  if (LoRa.read() != NET_VERSION) {
    return;
  }

  Packet newPacket;

  for (int i = 0; i < 4; i++) {
    newPacket.dest[i] = LoRa.read();
  }

  for (int i = 0; i < 4; i++) {
    newPacket.src[i] = LoRa.read();
  }

  for (int i = 0; i < 4; i++) {
    newPacket.msg_id[i] = LoRa.read();
  }

  newPacket.buffer_len = LoRa.read();

  char tempBuff[newPacket.buffer_len];
  for (int i = 0; i < newPacket.buffer_len; i++) {
    tempBuff[i] = LoRa.read();
  }

  newPacket.buffer = tempBuff;

  if (checkBucket(newPacket.msg_id)) {
    Serial.println("OLD_MSG"); // "Alrady recived this message, Ignoring."  
    return;
  }

  addIDtoMsgBucket(newPacket.msg_id);

  if (compAddress(newPacket.dest, localAddress)) {
    printPacket(newPacket);

    if (compBuffer(newPacket.buffer, "PING", 4)) {
      Packet pongPacket;

      for (int i = 0; i < 4; i++) {
        pongPacket.dest[i] = newPacket.src[i];
      }

      for (int i = 0; i < 4; i++) {
        pongPacket.src[i] = localAddress[i];
      }

      for (int i = 0; i < 4; i++) {
        pongPacket.msg_id[i] = random(256);
      }

      pongPacket.buffer_len = 4;

      pongPacket.buffer = "PONG";

      sendPacket(pongPacket);
    }
  } else if (compAddress(newPacket.dest, {BROADCAST_ADDRESS})){
    //printPacket(newPacket);
    
    if (compBuffer(newPacket.buffer, "PING", 4)) {
      Packet pongPacket;

      for (int i = 0; i < 4; i++) {
        pongPacket.dest[i] = newPacket.src[i];
      }

      for (int i = 0; i < 4; i++) {
        pongPacket.src[i] = localAddress[i];
      }

      for (int i = 0; i < 4; i++) {
        pongPacket.msg_id[i] = random(256);
      }

      pongPacket.buffer_len = 4;

      pongPacket.buffer = "PONG";

      sendPacket(pongPacket);
    }
    
    sendPacket(newPacket);
  } else {
    Serial.println("WRONG_ADDR"); // "This packet is not for me, Rebroadcasting"
    sendPacket(newPacket);
  }
}

void sendPong() {
  
      
}

void printPacket(Packet packet) {
  Serial.print("MESSAGE ");

  printArray(packet.dest, ".", 4);
  Serial.print(" ");
  printArray(packet.src, ".", 4);
  Serial.print(" ");
  printArray(packet.msg_id, "", 4);
  Serial.print(" ");
  Serial.print(packet.buffer_len, DEC);
  Serial.print(" ");
  printArray(packet.buffer, ",", packet.buffer_len);
  Serial.print(" ");
  Serial.print(LoRa.packetRssi(), DEC);
  Serial.print(" ");
  Serial.print(LoRa.packetSnr(), DEC);
  Serial.print("\n");
}

void splitCharArray(char* input, char delimiter, char** outputArray, int maxElements) {
  int inputLength = strlen(input);
  int elementCount = 0;
  char* token = strtok(input, &delimiter);

  while (token != NULL && elementCount < maxElements) {
    outputArray[elementCount++] = token;
    token = strtok(NULL, &delimiter);
  }
}

void printArray(uint8_t* arr, char* del, int size) {
  for (int i = 0; i < size; i++) {
    if (i != 0) {
      Serial.print(del);
    }
    Serial.print(arr[i], DEC);
  }
}

void getAddress(uint8_t* addr) {
  for (size_t i = 0; i < 4; i++) {
    addr[i] = UniqueID8[i + 4];
  }
}

bool compAddress(uint8_t* a, uint8_t* b) {
  for (size_t i = 0; i < 4; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

bool compBuffer(uint8_t* a, uint8_t* b, uint8_t size) {
  for (size_t i = 0; i < size; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

void copyBuffer(uint8_t* a, uint8_t* b, uint8_t size) {
  for (size_t i = 0; i < size; i++) {
    a[i] == b[i];
  }
}

void repeatBuffer(uint8_t* a, uint8_t b, uint8_t size) {
  for (size_t i = 0; i < size; i++) {
    a[i] == b;
  }
}

void randomBuffer(uint8_t* a, uint8_t max, uint8_t size) {
  for (size_t i = 0; i < size; i++) {
    a[i] == random(max);
  }
}

void printAddress(uint8_t* addr, char* delimiter) {
  for (size_t i = 0; i < 4; i++) {
    if (i != 0) {
      Serial.print(delimiter);
    }
    Serial.print(addr[i], DEC);
  }
}

void addIDtoMsgBucket(uint8_t* id) {
  for (int i = 0; i < 4; i++) {
    msgIdBucket[msgIdBucketPos][i] = id[i];
  }
  msgIdBucketPos += 1;

  if (msgIdBucketPos >= MSG_ID_BUCKET_SIZE) {
    msgIdBucketPos = 0;
  }
}

bool checkBucket(uint8_t* id) {
  for (int i = 0; i < MSG_ID_BUCKET_SIZE; i++) {
    if (compAddress(msgIdBucket[i], id)) {
      return true;
    }
  }
  return false;
}
