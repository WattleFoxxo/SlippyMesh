#include <SPI.h>
#include <LoRa.h> /* YOU NEED THE FORKED LIBARY: https://github.com/WattleFoxxo/arduino-LoRa */
#include <ArduinoUniqueID.h> /* https://github.com/ricaun/ArduinoUniqueID */
#include <ATCommands.h> /* https://github.com/yourapiexpert/ATCommands */

// LoRa Pins
#define LORA_CS_PIN 10
#define LORA_RESET_PIN 9
#define LORA_IRQ_PIN 2

// LoRa Modem Settings
#define LORA_SF 10 // 9
#define LORA_BW 256E3 // 125E3
#define LORA_CR 8
#define LORA_PREAMBLE_LENGTH 8

// Slippy Settings
#define SLIPPY_AUTO_RESTART false
#define SLIPPY_AUTO_RESTART_WARNING false
#define SLIPPY_AUTO_RESTART_TIME 30
#define SLIPPY_AUTO_RESTART_WARNING_TIME 25
#define SLIPPY_AUTO_RESTART_WARNING_MESSAGE "This node will be restarting in 5 minutes!"
#define SLIPPY_DEBUG_MESSAGES false
#define SLIPPY_TIMEOUT 2000

#define SLIPPY_NETWORK_VERSION 4
#define SLIPPY_BUFFER_SIZE 128
#define SLIPPY_SYNC_WORD 0x22
#define SLIPPY_PACKET_ID_BUCKET_SIZE 8
#define SLIPPY_PACKET_COORDANATOR 0
#define SLIPPY_PACKET_DATASTREAM 1
#define SLIPPY_COORDANATOR_CHANNEL 0
#define SLIPPY_BROADCAST_ADDRESS 0xFFFFFFFF

#define AT_BUFFER_SIZE 255

const uint32_t CHANNELS[8] = {
  9160E5, // 0 COORDINATOR
  9166E5, // 1 DATASTREAM
  9172E5, // 2 DATASTREAM
  9178E5, // 3 DATASTREAM
  9184E5, // 4 DATASTREAM
  9190E5, // 5 DATASTREAM
  9196E5, // 6 DATASTREAM
  9202E5, // 7 DATASTREAM
};

struct SlippyCoordinatorPacket {
  uint8_t channel;
  uint16_t packetId;
};

struct SlippyDataStreamPacket {
  uint8_t hops;
  uint32_t sourceAddress;
  uint32_t destinationAddress;
  uint16_t packetId;
  uint8_t data[SLIPPY_BUFFER_SIZE];
};

SlippyDataStreamPacket dataStreamIn;
SlippyDataStreamPacket dataStreamOut;
SlippyCoordinatorPacket coordinatorIn;
SlippyCoordinatorPacket coordinatorOut;

uint32_t localAddress = 0;

uint8_t listenerState = 0;

uint16_t packetIdBucket[SLIPPY_PACKET_ID_BUCKET_SIZE];
uint8_t packetIdBucketPos = 0;

void(* resetFunc)(void) = 0; // Software reset function

ATCommands AT;

/* AT COMMANDS */

// Slippy Error
bool ATSlippyError(ATCommands *sender) {
  Serial.println(F("Invalid command usage, refer to \"AT+HELP\""));
  return false;
}

// Help Command
bool ATHelpCmd(ATCommands *sender) {
  Serial.println(F("Go to https://github.com/WattleFoxxo/SlippyMesh/tree/SlippyMeshV4 for more info"));
  return true;
}

// Send Command
bool ATSendCmd(ATCommands *sender) {

  uint8_t address[4] = {sender->next().toInt(), sender->next().toInt(), sender->next().toInt(), sender->next().toInt()};
  String buff = sender->next();
  
  dataStreamOut.sourceAddress = localAddress;
  dataStreamOut.destinationAddress = pack32(address, 0);
  dataStreamOut.packetId = random(0xFFFF);
  strcpy(dataStreamOut.data, buff.c_str());
  
  //addIDtoPacketBucket(dataStreamOut.packetId);
  sendPacket(&dataStreamOut);

  return true;
}

// Send Bytes Command
bool ATSendBytesCmd(ATCommands *sender) {
  Serial.println(sender->next());
  Serial.println(sender->next());
  return true;
}


static at_command_t commands[] = {
  {"+HELP", ATHelpCmd, ATSlippyError, ATSlippyError, ATSlippyError},
  {"+SEND", ATSlippyError, ATSlippyError, ATSlippyError, ATSendCmd},
  {"+SENDBYTES", ATSlippyError, ATSlippyError, ATSlippyError, ATSendBytesCmd},
};

/* MAIN PROGRAM */

void setup() {
  Serial.begin(115200);
  Serial.println(F("Slippy Mesh is starting.."));

  randomSeed(analogRead(0));

  // Initialize the lora module
  LoRa.setPins(LORA_CS_PIN, LORA_RESET_PIN, LORA_IRQ_PIN);

  if (!LoRa.begin(CHANNELS[0])){
    Serial.println(F("The LoRa module failed to initialize!"));
    while (true);
  }

  LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setPreambleLength(LORA_PREAMBLE_LENGTH);
  LoRa.setSyncWord(SLIPPY_SYNC_WORD);
  LoRa.enableCrc();

  localAddress = pack32(UniqueID8, 4);
  
  AT.begin(&Serial, commands, sizeof(commands), AT_BUFFER_SIZE);

  Serial.print(F("Ready, Your address is: "));
  for (int i = 0; i < 4; i++) {
    if (i != 0) {
      Serial.print(".");
    }
    Serial.print(UniqueID8[i+4]);
  }
  Serial.print(F("\n"));

  Serial.println(F("For help type \"AT+HELP\"."));
}

void loop() {
  AT.update();
  autoReboot();
  readPacket(LoRa.parsePacket());
}

/* DAEMONS */

// at-command.listerner.execution.daemon
void alexD(SlippyDataStreamPacket *packet) {
  if (!strcmp(packet->data, "AT+PING")) {
    dataStreamOut.sourceAddress = localAddress;
    dataStreamOut.destinationAddress = packet->sourceAddress;
    dataStreamOut.packetId = random(0xFFFF);
    strcpy(dataStreamOut.data, "PONG");
    addIDtoPacketBucket(dataStreamOut.packetId);

    sendPacket(&dataStreamOut);
  }

  if (!strcmp(packet->data, "AT+UPTIME")) {
    dataStreamOut.sourceAddress = localAddress;
    dataStreamOut.destinationAddress = packet->sourceAddress;
    dataStreamOut.packetId = random(0xFFFF);
    strcpy(dataStreamOut.data, ("UPTIME: "+String(millis(), DEC)+" MS").c_str());
    addIDtoPacketBucket(dataStreamOut.packetId);
    
    sendPacket(&dataStreamOut);
  }

  if (!strcmp(packet->data, "AT+TEMP")) {
    dataStreamOut.sourceAddress = localAddress;
    dataStreamOut.destinationAddress = packet->sourceAddress;
    dataStreamOut.packetId = random(0xFFFF);
    strcpy(dataStreamOut.data, ("TEMP: "+String(LoRa.temperature(), DEC)+"°C").c_str());
    addIDtoPacketBucket(dataStreamOut.packetId);
    
    sendPacket(&dataStreamOut);
  }

  // Add custom alexd commands here

}

void autoReboot() {
  static bool autoRestartWarning = true;

  if (SLIPPY_AUTO_RESTART) {
    if (millis() >= SLIPPY_AUTO_RESTART_WARNING_TIME*60000 && autoRestartWarning) {
      autoRestartWarning = false;

      dataStreamOut.sourceAddress = localAddress;
      dataStreamOut.destinationAddress = SLIPPY_BROADCAST_ADDRESS;
      dataStreamOut.packetId = random(0xFFFF);
      strcpy(dataStreamOut.data, SLIPPY_AUTO_RESTART_WARNING_MESSAGE);
      addIDtoPacketBucket(dataStreamOut.packetId);

      sendPacket(&dataStreamOut);
    }

    if (millis() >= SLIPPY_AUTO_RESTART_TIME*60000) {
      resetFunc();
    }
  }
}

/* SLIPPY */

void readPacket(uint8_t packetSize) {
  if (packetSize == 0) {
    return;
  }

  Serial.println(F("[Debug] Incoming packet.."));
  
  if (LoRa.read() != SLIPPY_NETWORK_VERSION) {
    LoRa.flush();
    LoRa.setFrequency(CHANNELS[SLIPPY_COORDANATOR_CHANNEL]);
    return;
  }

  if (LoRa.read() == SLIPPY_PACKET_COORDANATOR) {
    LoRaGet(coordinatorIn);

    if (checkBucket(coordinatorIn.packetId)) {
      Serial.println(F("[Debug] Old packet."));
      LoRa.flush();
      LoRa.setFrequency(CHANNELS[SLIPPY_COORDANATOR_CHANNEL]);
      return;
    }

    LoRa.setFrequency(CHANNELS[coordinatorIn.channel]);

    uint32_t timer = millis() + SLIPPY_TIMEOUT;
    while (LoRa.parsePacket() == 0) {
      if (millis() >= timer) {
        LoRa.flush();
        LoRa.setFrequency(CHANNELS[SLIPPY_COORDANATOR_CHANNEL]);
        return;
      }
    }

    if (LoRa.read() != SLIPPY_NETWORK_VERSION) {
      LoRa.flush();
      LoRa.setFrequency(CHANNELS[SLIPPY_COORDANATOR_CHANNEL]);
      return;
    }

    if (LoRa.read() == SLIPPY_PACKET_DATASTREAM) {
      LoRaGet(dataStreamIn);
      LoRa.setFrequency(CHANNELS[SLIPPY_COORDANATOR_CHANNEL]);

      //addIDtoPacketBucket(dataStreamIn.packetId);

      if (dataStreamIn.destinationAddress == localAddress) {
        printPacket(&dataStreamIn);
        alexD(&dataStreamIn);
      } else if (dataStreamIn.destinationAddress == SLIPPY_BROADCAST_ADDRESS) {
        printPacket(&dataStreamIn);
        alexD(&dataStreamIn);
        Serial.println(F("[Debug] Packet is a broadcast, rebroadcasting.."));
        dataStreamIn.hops += 1;
        sendPacket(&dataStreamIn);
      } else {
        Serial.println(F("[Debug] Packet not for me, rebroadcasting.."));
        dataStreamIn.hops += 1;
        sendPacket(&dataStreamIn);
      }
    }
  }

  LoRa.flush();
  LoRa.setFrequency(CHANNELS[SLIPPY_COORDANATOR_CHANNEL]);
  return;
}

void sendPacket(SlippyDataStreamPacket *packet) {
  delay(random(2)*200); // hacky safty

  uint8_t channel = random(7)+1;

  Serial.println(F("[Debug] Transmition start..."));

  coordinatorOut.channel = channel;
  coordinatorOut.packetId = packet->packetId;
  
  LoRa.beginPacket();
  LoRa.write(SLIPPY_NETWORK_VERSION);
  LoRa.write(SLIPPY_PACKET_COORDANATOR);
  LoRaPut(coordinatorOut);
  LoRa.endPacket();

  Serial.print(F("[Debug] Sending data stream on channel: "));
  Serial.println(channel);

  LoRa.setFrequency(CHANNELS[channel]);

  delay(200);

  LoRa.beginPacket();
  LoRa.write(SLIPPY_NETWORK_VERSION);
  LoRa.write(SLIPPY_PACKET_DATASTREAM);
  LoRaPut(*packet);
  LoRa.endPacket();

  LoRa.setFrequency(CHANNELS[SLIPPY_COORDANATOR_CHANNEL]);

  Serial.println(F("[Debug] Finished transmitting."));
}

void printPacket(SlippyDataStreamPacket *packet) {
  Serial.println(F("--- Message ---"));
  Serial.print(F("Source Address: "));
  
  uint8_t source[4];
  unpack32(packet->sourceAddress, source);
  for (int i = 0; i < 4; i++) {
    if (i != 0) {
      Serial.print(F("."));
    }
    Serial.print(source[i]);
  }
  Serial.print(F("\n"));

  Serial.print(F("Destination Address: "));

  uint8_t destination[4];
  unpack32(packet->destinationAddress, destination);
  for (int i = 0; i < 4; i++) {
    if (i != 0) {
      Serial.print(F("."));
    }
  Serial.print(destination[i]);
  }
  Serial.print(F("\n"));

  Serial.print(F("Message ID: "));
  Serial.println(packet->packetId, DEC);
  Serial.print(F("Packet Hops: "));
  Serial.println(packet->hops, DEC);
  Serial.print(F("Data: "));
  Serial.println((char*)packet->data);
  Serial.print(F("RSSI: "));
  Serial.println(LoRa.packetRssi(), DEC);
  Serial.print(F("SNR: "));
  Serial.println(LoRa.packetSnr(), DEC);
  Serial.println(F("---------------"));
}


/* LORA */

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

/* HELPERS */

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

// Add a new message id to the bucket and removes the oldest id
void addIDtoPacketBucket(uint16_t id) {
  packetIdBucket[packetIdBucketPos] = id;
  if (packetIdBucketPos >= SLIPPY_PACKET_ID_BUCKET_SIZE) {
    packetIdBucketPos = 0;
  }
}

// Checks if the bucket contians a message id
bool checkBucket(uint16_t id) {
  for (int i = 0; i < SLIPPY_PACKET_ID_BUCKET_SIZE; i++) {
    if (packetIdBucket[i] == id) {
      return true;
    }
  }
  return false;
}
