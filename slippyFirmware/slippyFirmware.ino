#include <RHMesh.h>
#include <RH_RF95.h>

#define NODE_ADDRESS 0

#define LORA_CS 10
#define LORA_RST 9
#define LORA_INT 2

#define LORA_FREQ 915.0

RH_RF95 rf95(LORA_CS, LORA_INT);
RHMesh rfMesh(rf95, NODE_ADDRESS);

void setup() {
  randomSeed(analogRead(0));

  Serial.begin(9600);
  while (!Serial); // Remove for headless

  Serial.println("[INFO] SlippyMesh Starting...");

  // reset the LoRa module or it will have alex.d behavior
  pinMode(LORA_RST, OUTPUT);
  digitalWrite(LORA_RST, HIGH);
  delay(200);
  digitalWrite(LORA_RST, LOW);
  delay(200);
  digitalWrite(LORA_RST, HIGH);
  delay(50);

  if (!rfMesh.init()) {
    Serial.println("[ERROR] Failed to init LoRa.");
    while(1);
  }

  if (!rf95.setFrequency(LORA_FREQ)) {
    Serial.println("[ERROR] Failed to set LoRa frequency.");
    while(1);
  }

  rfMesh.setTimeout(3000);
  rfMesh.setRetries(5);

  Serial.println("[INFO] Looking for available address.");

  uint8_t address = findAvailableAddress();
  rfMesh.setThisAddress(address);

  Serial.println("[INFO] Mesh network initialized.");
}

void loop() {
  if (Serial.available()) {
    String incomingString = Serial.readStringUntil('\n');
    Serial.print("[INFO] Sending: ");
    Serial.print(incomingString);
    Serial.println(" to: 255");
    sendMeshMessage(incomingString.c_str(), 255); // everyone address
  }

  if (rfMesh.available()) {
    uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    uint8_t from;
    uint8_t dest;

    if (rfMesh.recvfromAck(buf, &len, &from, &dest)) {
      if (dest == rfMesh.thisAddress() || dest == 255 ) {
        Serial.print("[DATA];");
        Serial.print(from);
        Serial.print(";");
        Serial.println((char*)buf);
      }
    }
  }
}

void sendMeshMessage(char* message, uint8_t dest) {
  if (rfMesh.sendtoWait((uint8_t*)message, strlen(message) + 1, dest) != RH_ROUTER_ERROR_NONE) {
    Serial.println("[ERROR] Message failed to send.");
  }
}

uint8_t findAvailableAddress() {
  uint8_t nodeId = 1;

  while (nodeId <= 255) {
    if (rfMesh.sendtoWait((uint8_t*)"PING", 5, nodeId) == RH_ROUTER_ERROR_NO_ROUTE) {
      Serial.print("[INFO] Found available address, Assigned: ");
      Serial.print(nodeId);
      Serial.println(".");
      return nodeId;
    }

    nodeId++;
  }

  Serial.println("[ERROR] No available address found.");
  while (1);
}