#include <RHMesh.h>
#include <RH_RF95.h>

#define NODE_ADDRESS 0

/* CONFIG */

//LoRa
#define LORA_CS 10
#define LORA_RST 9
#define LORA_INT 2
#define LORA_FREQ 915.0

//Serial
#define HEADLESS true
#define HUMAN_READABLE true

/* ------ */


/* 
  Serial codes
    [IN] <address> <message> : Incoming message
    [OUT] <address> <message> : Outgoing message
    [STATUS] <code> (<value>)... : Status message

  Serial commands
    *[SEND] <address> <massage> : Send a message
    *[SET] <veriable> <value>... : Set a value

  (all marked with "*" is unimplemented)
*/

RH_RF95 rf95(LORA_CS, LORA_INT);
RHMesh rfMesh(rf95, NODE_ADDRESS);

char serialBuffer[RH_MESH_MAX_MESSAGE_LEN];
int serialBufferIndex = 0;

void setup() {
  randomSeed(analogRead(0));

  Serial.begin(9600);
  while (!Serial); // Remove for headless

  if (HUMAN_READABLE) {
    Serial.println("SlippyMesh Starting...");
  } else {
    Serial.print("[STATUS] STARTING");
  }

  // reset the LoRa module or it will have alex.d behavior
  pinMode(LORA_RST, OUTPUT);
  digitalWrite(LORA_RST, HIGH);
  delay(200);
  digitalWrite(LORA_RST, LOW);
  delay(200);
  digitalWrite(LORA_RST, HIGH);
  delay(50);

  if (!rfMesh.init()) {
    if (HUMAN_READABLE) {
      Serial.println("Failed to init LoRa.");
    } else {
      Serial.println("[STATUS] ERROR_INIT");
    }
    while(1);
  }

  rf95.setModemConfig(RH_RF95::ModemConfigChoice::Bw125Cr45Sf2048);

  if (!rf95.setFrequency(LORA_FREQ)) {
    if (HUMAN_READABLE) {
      Serial.println("Failed to set LoRa frequency.");
    } else {
      Serial.println("[STATUS] ERROR_LORA_FREQ");
    }
    while(1);
  }

  rfMesh.setTimeout(3000);
  rfMesh.setRetries(5);


  if (HUMAN_READABLE) {
    Serial.println("Looking for available address.");
  } else {
    Serial.print("[STATUS] LOOKING");
  }

  uint8_t address = findAvailableAddress();
  rfMesh.setThisAddress(address);

  if (HUMAN_READABLE) {
    Serial.println("Mesh network initialized.");
  } else {
    Serial.print("[STATUS] READY");
  }
}

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void loop() {
  while (Serial.available() > 0) {
    char data = Serial.read();
    if (data == '\n') {
      serialBuffer[serialBufferIndex] = '\0';
      //remove(serialBuffer, 7); // removes "[SEND] " // TODO: get acual command

      int addr = atoi(serialBuffer); // gets address
      remove(serialBuffer, indexOf(serialBuffer, ';')+1); // removes the address

      if (addr > 0 && addr < 256) {
        if (HUMAN_READABLE) {
          Serial.print("Sending \"");
          Serial.print(serialBuffer);
          Serial.print("\" to node #");
          Serial.println(addr);
        } else {
          Serial.print("[OUT] ");
          Serial.print(addr);
          Serial.print(" ");
          printASCII(serialBuffer);
          Serial.println("");
        }

        sendMeshMessage(serialBuffer, addr);
      } else {
        if (HUMAN_READABLE) {
          Serial.println("Invalid address.");
        }
      }

      serialBufferIndex = 0;
      memset(serialBuffer, 0, sizeof(serialBuffer));
    } else {
      if (serialBufferIndex < RH_MESH_MAX_MESSAGE_LEN - 1) {
        serialBuffer[serialBufferIndex] = data;
        serialBufferIndex++;
      }
    }
  }

  if (millis() % 5000 == 0) {
    sendMeshMessage("5g vaccine chip acivate", 1);
  }

  if (millis() >= 300000) {
    resetFunc();
  }

  if (rfMesh.available()) {
    uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    uint8_t from;
    uint8_t dest;

    if (rfMesh.recvfromAck(buf, &len, &from, &dest)) {
      if (dest == rfMesh.thisAddress() || dest == 255 ) {
        if (HUMAN_READABLE) {
          Serial.print("Message from node #");
          Serial.print(from);
          Serial.print(", \"");
          Serial.print((char*)buf);
          Serial.println("\"");
          sendMeshMessage(buf, from);
        } else {
          Serial.print("[IN] ");
          Serial.print(from);
          Serial.print(" ");
          Serial.println((char*)buf);
        }
      }
    }
  }
}

void sendMeshMessage(char* message, uint8_t dest) {
  if (rfMesh.sendtoWait((uint8_t*)message, strlen(message) + 1, dest) != RH_ROUTER_ERROR_NONE) {
    if (HUMAN_READABLE) {
      Serial.println("Message failed to send.");
    } else {
      Serial.println("[STATUS] ERROR_SEND");
    }
  }
}

uint8_t findAvailableAddress() {
  unt8_t nodeId = 1;

  while (nodeId <= 255) {
    if (rfMesh.sendtoWait((uint8_t*)"PING", 5, nodeId) == RH_ROUTER_ERROR_NO_ROUTE) {
      if (HUMAN_READABLE) {
        Serial.print("Found available address, Assigned #");
        Serial.print(nodeId);
        Serial.println(".");
      } else {
        Serial.print("[STATUS] ID ");
        Serial.println(nodeId);
      }
      return nodeId;
    }

    nodeId++;
  }

  if (HUMAN_READABLE) {
    Serial.println("No available address found.");
  } else {
    Serial.println("[STATUS] ERROR_NO_ADDR");
  }
  while (1);
  //return 192;
}

// helpers

int indexOf(char *str, char searchChar) {
  int len = strlen(str);
  for (int i = 0; i < len; i++) {
    if (str[i] == searchChar) {
      return i;
    }
  }
  return 0;
}

void remove(char* str, int n) {
    int len = strlen(str);
    if (n >= len) {
        str[0] = '\0';
    } else {
        memmove(str, str + n, len - n + 1);
    }
}

void printASCII(char* data) {
  Serial.print("[");
  for (int i = 0; i < strlen(data); i++) {
    if (i > 0) {
      Serial.print(", ");
    }
    Serial.print((int)data[i]);
  }
  Serial.print("]");
}
