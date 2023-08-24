#include <SPI.h>
#include <LoRa.h> /* https://github.com/sandeepmistry/arduino-LoRa */
#include <ArduinoUniqueID.h> /* https://github.com/ricaun/ArduinoUniqueID */

// LoRa Pins
#define LORA_CS_PIN 10
#define LORA_RESET_PIN 9
#define LORA_IRQ_PIN 2

// LoRa Modem Settings
#define LORA_SF 7
#define LORA_BW 125E3
#define LORA_CR 8
#define LORA_PREAMBLE_LENGTH 12
#define LORA_FREQ 915E6

// Slippy Settings
#define SLIPPY_AUTO_RESTART false
#define SLIPPY_AUTO_RESTART_WARNING true
#define SLIPPY_AUTO_RESTART_TIME 30
#define SLIPPY_AUTO_RESTART_WARNING_TIME 25
#define SLIPPY_AUTO_RESTART_WARNING_MESSAGE "This node will be restarting in 5 minutes!"
#define SLIPPY_DEBUG_MESSAGES false

// Slippy Network Static Definitions (NO TOUCHY!)
#define SLIPPY_NETWORK_VERSION 3
#define SLIPPY_BUFFER_SIZE 128
#define SLIPPY_SYNC_WORD 0x22
#define SLIPPY_MSG_ID_BUCKET_SIZE 8
#define SLIPPY_PACKET_NORMAL 0b00000000
#define SLIPPY_PACKET_NOMESH 0b10000000

// The packet structure
struct SlippyPacket {
  uint8_t netVer; // The network version for compatibility checking
  uint8_t packetType; // The packet type, bit 0: Meshing/Relaying, bit 1-7 N/A 
  uint32_t source; // The address of the sender
  uint32_t destination; // The address of the target
  uint16_t messageId; // The message id to avoid relaying old packets
  uint8_t data[SLIPPY_BUFFER_SIZE]; // The Packet data
};

uint8_t LORA_MESSAGE_LENGTH = sizeof(SlippyPacket);

uint32_t localAddress = 0;

uint16_t msgIdBucket[SLIPPY_MSG_ID_BUCKET_SIZE]; // A bucket to store the recent message ids
uint8_t msgIdBucketPos = 0;

bool autoRestartWarning = true;

void(* resetFunc)(void) = 0; // Software reset function

void setup() {
  Serial.begin(115200);
  Serial.println(F("Slippy Mesh is starting.."));

  randomSeed(analogRead(0));

  // Initialize the lora module
  LoRa.setPins(LORA_CS_PIN, LORA_RESET_PIN, LORA_IRQ_PIN);

  if (!LoRa.begin(LORA_FREQ)){
    Serial.println(F("The LoRa module failed to initialize!"));
    while (true);
  }

  // Set the modem settings
  LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setPreambleLength(LORA_PREAMBLE_LENGTH);
  LoRa.setSyncWord(SLIPPY_SYNC_WORD);
  LoRa.enableCrc();

  // Retrieve the local address using unique ids
  localAddress = pack32(UniqueID8, 4);
  
  Serial.print(F("Ready, Your address is: "));

  for (int i = 0; i < 4; i++) {
    if (i != 0) {
      Serial.print(".");
    }
    Serial.print(UniqueID8[i+4]);
  }
  Serial.print(F("\n"));
  Serial.println(F("Type \"H\" for help."));
}

void loop() {
  // Read serial
  if (Serial.available() > 0) {
    char commandBuffer[2] = {0, 0};
    uint8_t cmdLen = Serial.readBytesUntil(' ', commandBuffer, 3);

    // Help command
    if (commandBuffer[0] == 'H') {
      Serial.println(F("--- Help ---"));
      Serial.println(F("H : Shows this menu."));
      Serial.println(F(""));
      Serial.println(F("S<Mode> <Address> <Message> : Sends a message"));
      Serial.println(F("      Mode: (optional) Packet mode, Available modes: N (normal packet), M (packet without meshing/relaying)"));
      Serial.println(F("      Address: Destination address, Format: X.X.X.X, X is a value from 0 to 255. To send a message to everyone use: 255.255.255.255"));
      Serial.println(F("      Message: Message, The Message you want to send"));
      Serial.println(F("      Examples: \"S 192.168.0.1 I have your ip :nerd:\", \"SM 255.255.255.255 Only people near me can hear me\", \"S 192.168.0.1 PING\" (You should receive \"PONG\")."));
      Serial.println(F("------------"));
    }

    // Send command
    if (commandBuffer[0] == 'S') {
      uint8_t addressArray[4];
      for (int i = 0; i < 4; i++) {
        char addressBuffer[4];
        uint8_t len;
        if (i == 3) {
          len = Serial.readBytesUntil(' ', addressBuffer, 4);
        } else {
          len = Serial.readBytesUntil('.', addressBuffer, 4);
        }
        addressBuffer[len] = '\0';
        addressArray[i] = atoi(addressBuffer);
      }

      uint32_t address = pack32(addressArray, 0);

      char dataBuffer[SLIPPY_BUFFER_SIZE];
      uint8_t numBytes = Serial.readBytesUntil('\n', dataBuffer, SLIPPY_BUFFER_SIZE-1);
      dataBuffer[numBytes] = '\0';

      uint8_t packetType = SLIPPY_PACKET_NORMAL;
      
      if (commandBuffer[1] == 'M') {
        packetType = SLIPPY_PACKET_NOMESH;
      }

      SlippyPacket newPacket;
      newPacket.netVer = SLIPPY_NETWORK_VERSION;
      newPacket.packetType = packetType;
      newPacket.source = localAddress;
      newPacket.destination = address;
      newPacket.messageId = random(0xFFFF);
      strcpy(newPacket.data, dataBuffer);
      addIDtoMsgBucket(newPacket.messageId);
      sendPacket(newPacket);
    }
  }

  // Auto restart
  if (SLIPPY_AUTO_RESTART) {
    if (millis() >= SLIPPY_AUTO_RESTART_WARNING_TIME*60000 && autoRestartWarning) {
      autoRestartWarning = false;
      SlippyPacket newPacket;
      newPacket.netVer = SLIPPY_NETWORK_VERSION;
      newPacket.packetType = SLIPPY_PACKET_NORMAL;
      newPacket.source = localAddress;
      newPacket.destination = 0xFFFFFFFF;
      newPacket.messageId = random(0xFFFF);
      strcpy(newPacket.data, SLIPPY_AUTO_RESTART_WARNING_MESSAGE);
      addIDtoMsgBucket(newPacket.messageId);
      sendPacket(newPacket);
    }

    if (millis() >= SLIPPY_AUTO_RESTART_TIME*60000) {
      resetFunc();
    }
  }

  // Read LoRa
  receivePacket(LoRa.parsePacket());
}

// Sends a packet
void sendPacket(SlippyPacket packet) {
  if (SLIPPY_DEBUG_MESSAGES) { Serial.println(F("[Debug] Sending packet..")); }
  LoRa.beginPacket();
  LoRaPut(packet); // Puts the packet into the lora buffer
  LoRa.endPacket();
  if (SLIPPY_DEBUG_MESSAGES) { Serial.println(F("[Debug] Sent packet..")); }
}

// Reads the next packet
void receivePacket(int packetSize) {
  if (packetSize == 0) {
    return;
  }

  SlippyPacket newPacket;
  LoRaGet(newPacket); // Gets the packet from the lora buffer

  if (newPacket.netVer != SLIPPY_NETWORK_VERSION) {
    if (SLIPPY_DEBUG_MESSAGES) { Serial.println(F("[Debug] Received packet with invalid network version, Ignoring.")); }
    return;
  }

  // Check if the message id is alrady in the bucket
  if (checkBucket(newPacket.messageId)) {
    if (SLIPPY_DEBUG_MESSAGES) { Serial.println(F("[Debug] Alrady recived this packet, Ignoring.")); }
    return;
  }

  // Save this message id
  addIDtoMsgBucket(newPacket.messageId);

  bool packetType[8];
  unpack8(newPacket.packetType, packetType);

  // Check the destination address
  if (newPacket.destination == localAddress) {
    // The destination is your address
    printPacket(newPacket);

    if (newPacket.data[0] == 'P' && newPacket.data[1] == 'I' && newPacket.data[2] == 'N' && newPacket.data[3] == 'G') {
      SlippyPacket pongPacket;
      pongPacket.netVer = SLIPPY_NETWORK_VERSION;
      pongPacket.packetType = newPacket.packetType;
      pongPacket.source = localAddress;
      pongPacket.destination = newPacket.source;
      pongPacket.messageId = random(0xFFFF);
      strcpy(pongPacket.data, "PONG");

      if (SLIPPY_DEBUG_MESSAGES) { Serial.println(F("[Debug] Received \"PING\", Responding with \"PONG\"..")); }

      sendPacket(pongPacket);
    }
  } else if (newPacket.destination == 0xFFFFFFFF) {
    // The destination is the global address
    if (SLIPPY_DEBUG_MESSAGES) { Serial.print(F("[Debug] Received global packet")); }

    if (!packetType[0]) {
      if (SLIPPY_DEBUG_MESSAGES) { Serial.println(F(", Relaying..")); }
      sendPacket(newPacket);
    } else {
      if (SLIPPY_DEBUG_MESSAGES) { Serial.println(F(" with meshing/relaying disabled.")); }
    } 

    printPacket(newPacket);
  } else {
    // The packet is for someone else
    if (SLIPPY_DEBUG_MESSAGES) { Serial.print(F("[Debug] This packet is not for me")); }

    if (!packetType[0]) {
      if (SLIPPY_DEBUG_MESSAGES) { Serial.println(F(", Relaying..")); }
      sendPacket(newPacket);
    } else {
      if (SLIPPY_DEBUG_MESSAGES) { Serial.println(F(" and meshing/relaying is disabled.")); }
    }  
  }
}

// Prints out relevent infomation in the packet and some lora info
void printPacket(SlippyPacket packet) {
  Serial.println(F("--- Message ---"));
  Serial.print(F("Network Version: "));
  Serial.println(packet.netVer, DEC);
  Serial.print(F("Source Address: "));
  
  uint8_t source[4];
  unpack32(packet.source, source);
  for (int i = 0; i < 4; i++) {
    if (i != 0) {
      Serial.print(".");
    }
    Serial.print(source[i]);
  }
  Serial.print(F("\n"));

  Serial.print(F("Destination Address: "));

  uint8_t destination[4];
  unpack32(packet.destination, destination);
  for (int i = 0; i < 4; i++) {
    if (i != 0) {
      Serial.print(F("."));
    }
    Serial.print(destination[i]);
  }
  Serial.print(F("\n"));

  Serial.print(F("Message ID: "));
  Serial.println(packet.messageId, DEC);
  Serial.print(F("Data: "));
  Serial.println((char*)packet.data);
  Serial.print(F("RSSI: "));
  Serial.println(LoRa.packetRssi(), DEC);
  Serial.print(F("SNR: "));
  Serial.println(LoRa.packetSnr(), DEC);
  Serial.println(F("---------------"));
}

// Packs an array of uint8s into a single uint32
uint32_t pack32(uint8_t* in, uint8_t offset) {
  return ((uint32_t(in[offset]) << 24) | (uint32_t(in[offset+1]) << 16) | (uint32_t(in[offset+2]) << 8) | uint32_t(in[offset+3]));
}

// Unpacks a single uint32 into an array of uint8s
void unpack32(uint32_t in, uint8_t* out) {
  out[0] = (in >> 24) & 0xFF;
  out[1] = (in >> 16) & 0xFF;
  out[2] = (in >> 8) & 0xFF;
  out[3] = in & 0xFF;
}

// Packs an array of bools into a single uint8
uint8_t pack8(bool* in) {
  uint8_t out = 0;
  out |= in[7];
  out |= in[6] << 1;
  out |= in[5] << 2;
  out |= in[4] << 3;
  out |= in[3] << 4;
  out |= in[2] << 5;
  out |= in[1] << 6;
  out |= in[0] << 7;
  return out;
}

// Unpacks a single uint8 into an array of bools
void unpack8(uint8_t in, bool* out) {
  out[7] = in & 0x01;
  out[6] = (in >> 1) & 0x01;
  out[5] = (in >> 2) & 0x01;
  out[4] = (in >> 3) & 0x01;
  out[3] = (in >> 4) & 0x01;
  out[2] = (in >> 5) & 0x01;
  out[1] = (in >> 6) & 0x01;
  out[0] = (in >> 7) & 0x01;
}

// Im even not going to lie i have no idea what this does but it works
// credit: https://github.com/chandrawi/LoRaRF-Arduino/blob/main/src/SX127x.h#L73
template <typename T> void LoRaPut(T data) {
  const uint8_t length = sizeof(T);
  union conv {
    T Data;
    uint8_t Binary[length];
  };
  union conv u;
  u.Data = data;
  LoRa.write(u.Binary, length);
}

// Im even not going to lie i have no idea what this does but it works
// credit: https://github.com/chandrawi/LoRaRF-Arduino/blob/main/src/SX127x.h#L93
template <typename T> uint8_t LoRaGet(T &data) {
  const uint8_t length = sizeof(T);
  union conv {
    T Data;
    uint8_t Binary[length];
  };
  union conv u;
  uint8_t len = LoRaRead(u.Binary, length);
  data = u.Data;
  return len;
}

// Reads the reqested amout of bytes from the lora and puts it into a buffer
uint8_t LoRaRead(uint8_t* data, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
        data[i] = LoRa.read();
    }
    return length;
}

// Add a new message id to the bucket and removes the oldest id
void addIDtoMsgBucket(uint16_t id) {
  msgIdBucket[msgIdBucketPos] = id;
  if (msgIdBucketPos >= SLIPPY_MSG_ID_BUCKET_SIZE) {
    msgIdBucketPos = 0;
  }
}

// Checks if the bucket contians a message id
bool checkBucket(uint8_t* id) {
  for (int i = 0; i < SLIPPY_MSG_ID_BUCKET_SIZE; i++) {
    if (msgIdBucket[i] == id) {
      return true;
    }
  }
  return false;
}

