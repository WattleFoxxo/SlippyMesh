#include <SPI.h>
#include <LoRa.h> /* https://github.com/sandeepmistry/arduino-LoRa */
#include <ArduinoUniqueID.h> /* https://github.com/ricaun/ArduinoUniqueID */

#define LORA_CS 10
#define LORA_RST 9
#define LORA_IRQ 2

#define LORA_FRQ 915E6

#define MAX_BUFFER 64
#define MSG_ID_BUCKET_SIZE 8

#define NET_VERSION 1
#define MESSAGE_INFO true
#define LOG_INFO true
#define LOG_WARNING true
#define LOG_ERROR true

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
  info("Starting slippy..\n");

  randomSeed(analogRead(0));

  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);

  if (!LoRa.begin(LORA_FRQ)) {
    error("LoRa module failure.\n");
    while (true);
  }

  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(20);
  LoRa.enableCrc();

  getAddress(localAddress);
  
  info("Your address is: ");
  printAddress(localAddress, ".");
  Serial.print("\n");
}

void loop() {
  while (Serial.available()) {
    static char serialBuffer[MAX_BUFFER];
    static unsigned int serialBufferPos = 0;

    char inByte = Serial.read();

    if (inByte != '\n') {
      serialBuffer[serialBufferPos] = inByte;
      serialBufferPos += 1;
    } else {
      serialBuffer[serialBufferPos] = '\0';
      serialBufferPos = 0;

      char* cmdSplit[2];
      splitCharArray(serialBuffer, ';', cmdSplit, 2);

      char* message = cmdSplit[1];

      char* ipSplit[4];
      splitCharArray(cmdSplit[0], '.', ipSplit, 4);
      
      Packet newPacket;

      for (int i = 0; i < 4; i++) {
        newPacket.dest[i] = atoi(ipSplit[i]);
      }

      for (int i = 0; i < 4; i++) {
        newPacket.src[i] = localAddress[i];
      }

      for (int i = 0; i < 4; i++) {
        newPacket.msg_id[i] = random(256);
      }

      newPacket.buffer_len = strlen(message);

      char tempBuff[newPacket.buffer_len];
      for (int i = 0; i < newPacket.buffer_len; i++) {
        tempBuff[i] = message[i];
      }

      newPacket.buffer = tempBuff;

      addIDtoMsgBucket(newPacket.msg_id);

      sendPacket(newPacket);

      info("Sent message.\n");
    }
  }

  readPacket(LoRa.parsePacket());
}

void sendPacket(Packet packet) {
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
    info("Alrady recived this message, Ignoring.\n");  
    return;
  }

  addIDtoMsgBucket(newPacket.msg_id);

  if (compAddress(newPacket.dest, localAddress)) {
    printPacket(newPacket);
  } else if (compAddress(newPacket.dest, {BROADCAST_ADDRESS})){
    printPacket(newPacket);
    sendPacket(newPacket);
  } else {
    sendPacket(newPacket);
  }
}

void printPacket(Packet packet) {
  Serial.println("-------[MESSAGE]-------");
  Serial.print("Message: \"");
  for (int i = 0; i < packet.buffer_len; i++) {
    Serial.print((char)packet.buffer[i]);
  }
  Serial.print("\"\n");
  Serial.print("To: ");
  printAddress(packet.dest, ".");
  Serial.print("\nFrom: ");
  printAddress(packet.src, ".");
  Serial.print("\n");

  if (MESSAGE_INFO) {
    Serial.print("\n");
    Serial.println("More Info:");
    Serial.print("RSSI: ");
    Serial.println(LoRa.packetRssi());
    Serial.print("SNR: ");
    Serial.println(LoRa.packetSnr());
    Serial.print("Message ID: ");
    printAddress(packet.msg_id, "");
    Serial.print("\n");
  }

  Serial.println("-----[END MESSAGE]-----");

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

/* Logging */
void info(char* msg) {
  if (LOG_INFO) {
    Serial.print("[INFO] ");
    Serial.print(msg);
  }
}

void warning(char* msg) {
  if (LOG_WARNING) {
    Serial.print("[WARNING] ");
    Serial.print(msg);
  }
}

void error(char* msg) {
  if (LOG_ERROR) {
    Serial.print("[ERROR] ");
    Serial.print(msg);
  }
}