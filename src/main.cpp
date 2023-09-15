#include "main.h"

uint8_t localAddress = 0;
uint8_t routes[SLIPPY_MAX_NODES];
int16_t rssi[SLIPPY_MAX_NODES];

RH_RF95 radio;
RHMesh *manager;

uint8_t incomingBuffer[RH_MESH_MAX_MESSAGE_LEN];

int freeMem() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void setup() {
    randomSeed(analogRead(0));
    Serial.begin(115200);
    delay(500);

    Serial.println(F("Starting Slippy Mesh..."));

    manager = new RHMesh(radio, 0);

    if (!manager->init()) {
        Serial.println(F("The radio failed to start."));
        while(true);
    }

    radio.setTxPower(23, false);
    radio.setFrequency(916.0);
    radio.setCADTimeout(500);

    for(uint8_t n=1;n<=SLIPPY_MAX_NODES;n++) {
        routes[n-1] = 0;
        rssi[n-1] = 0;
    }

    localAddress = findAvalabeAddress(SETTING_PREFERED_ADDRESS);
    manager->setThisAddress(localAddress);

    Serial.println(F("Slippy Mesh is ready!"));
    Serial.print(F("Your address is: "));
    Serial.println(localAddress);
    Serial.print(F("Free memory: "));
    Serial.println(freeMem());
}

const __FlashStringHelper* getErrorString(uint8_t error) {
    switch(error) {
        case 1:
            return F("invalid length");
            break;
        case 2:
            return F("no route");
            break;
        case 3:
            return F("timeout");
            break;
        case 4:
            return F("no reply");
            break;
        case 5:
            return F("unable to deliver");
            break;
    }

    return F("unknown");
}

void updateRoutingTable() {
    for(uint8_t n = 1; n <= SLIPPY_MAX_NODES; n++) {
        RHRouter::RoutingTableEntry *route = manager->getRouteTo(n);
        if (n == localAddress) {
            routes[n-1] = 255;
        } else {
            routes[n-1] = route->next_hop;
            if (routes[n-1] == 0) {
                rssi[n-1] = 0;
            }
        }
    }
}

void loop() {
    uint8_t len = sizeof(incomingBuffer);
    uint8_t from;
    if (manager->recvfromAckTimeout((uint8_t *)incomingBuffer, &len, 2000, &from)) {
        incomingBuffer[len] = '\0';

        Packet packet;
        deserializePacket(incomingBuffer, packet);

        if (packet.network_version != SLIPPY_NETWORK_VERSION) {
            return;
        }

        unsigned char base64[(((4 * packet.length / 3) + 3) & ~3)]; // Calculate size of base64 string. 
        encode_base64(packet.data, packet.length, base64);

        if (packet.packet_type == SLIPPY_PACKET_DIRECT) {
            Serial.println(F("-- Message --"));
            Serial.print(F("Address: "));
            Serial.println(from, DEC);
            Serial.print(F("Data: "));
            printBytes(packet.data, packet.length);
            Serial.print(F("\nJSON: "));
            Serial.print(F("\"message\":{\"address\":"));
            Serial.print(from);
            Serial.print(F(",\"data\":\""));
            Serial.print((char*)base64);
            Serial.println(F("\"}"));
            Serial.println(F("-------------"));

            alexD(from, packet);
        } else if (packet.packet_type == SLIPPY_PACKET_BROADCAST) {
            Serial.println(F("-- Broadcast Message --"));
            Serial.print(F("Address: "));
            Serial.println(from, DEC);
            Serial.print(F("Data: "));
            printBytes(packet.data, packet.length);
            Serial.print(F("\nJSON: "));
            Serial.print(F("\"broadcast_message\":{\"address\":"));
            Serial.print(from);
            Serial.print(F(",\"data\":"));
            Serial.print((char*)base64);
            Serial.println(F("}"));
            Serial.println(F("-----------------------"));
        }
    }

    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil(' ');

        if (cmd == "help") cmdHelp();
        else if (cmd == "info") cmdInfo();
        else if (cmd == "send") cmdSend();
        else if (cmd == "send64") cmdSend64();
        else cmdUnrecognized(cmd);
    }

    if (SETTING_AUTO_RESTART) {
        if ((millis() / 60000) > SETTING_AUTO_RESTART) {
            resetFunc();
        }
    }
}

bool sendMessage(uint8_t address, uint8_t* message, uint8_t len) {
    uint8_t packetLen = SLIPPY_HEADER_SIZE + len;
    uint8_t packetBuff[packetLen];

    Packet packet;
    packet.network_version = SLIPPY_NETWORK_VERSION;
    packet.length = len;
    strcpy(packet.data, message);

    updateRoutingTable();

    if (address == 255) {
        packet.packet_type = SLIPPY_PACKET_DIRECT;
        
        serializePacket(packet, packetBuff);
        Serial.println(F("Sending message to every node, this will take a while..."));

        for (uint8_t addr = 1; addr < SLIPPY_MAX_NODES; addr++) {
            uint8_t error = manager->sendtoWait(packetBuff, packetLen, addr);
            if (error != RH_ROUTER_ERROR_NONE) {
                Serial.print(F("ERROR: "));
                Serial.println(getErrorString(error));
            } else {
                Serial.println(F("OK"));
            }
        }

        Serial.println(F("Done!"));
        return true;
    } else {
        packet.packet_type = SLIPPY_PACKET_DIRECT;
        
        serializePacket(packet, packetBuff);

        Serial.print(F("Sending message to "));
        Serial.print(address);
        Serial.print(F("... "));

        uint8_t error = manager->sendtoWait(packetBuff, packetLen, address);
        if (error != RH_ROUTER_ERROR_NONE) {
            Serial.print(F("ERROR: "));
            Serial.println(getErrorString(error));
        } else {
            Serial.println(F("OK"));
            return true;
        }
    }
    return false;
}

void serializePacket(const Packet& packet, uint8_t* byteArray) {
    byteArray[0] = packet.network_version;
    byteArray[1] = packet.packet_type;
    byteArray[2] = packet.length;

    for (uint8_t i = 0; i < packet.length; i++) {
        byteArray[i + 3] = packet.data[i];
    }
}

void deserializePacket(const uint8_t* byteArray, Packet& packet) {
    packet.network_version = byteArray[0];
    packet.packet_type = byteArray[1];
    packet.length = byteArray[2];

    for (uint8_t i = 0; i < packet.length; i++) {
        packet.data[i] = byteArray[i + 3];
    }
}

void printBytes(uint8_t* byteArray, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        Serial.write(byteArray[i]);
    }
}

uint8_t findAvalabeAddress(uint8_t addr) {
    Serial.println(F("Looking for avalable address..."));

    Packet packet;
    packet.network_version = SLIPPY_NETWORK_VERSION;
    packet.packet_type = SLIPPY_PACKET_PING;
    packet.length = 4;
    strcpy(packet.data, "PING");

    uint8_t packetBuff[7];

    serializePacket(packet, packetBuff);

    if (!SETTING_PREFERED_ADDRESS) {
        addr = random(253) + 1;
    }

    while (true) {
        updateRoutingTable();

        uint8_t error = manager->sendtoWait(packetBuff, 7, addr);
        if (error == RH_ROUTER_ERROR_NO_ROUTE) {
            Serial.print(addr);
            Serial.println(F(" is available!"));
            return addr;
        } else {
            Serial.print(addr);
            Serial.println(F(" is taken."));
        }
       
        addr = random(253) + 1;
    }
}

/* Serial Commands */

void cmdUnrecognized(String cmd) {
	Serial.print("Error: Unrecognized command \"");
	Serial.print(cmd);
	Serial.println("\". Type \"help\" for the list of commands.");
}

void cmdHelp() {
	Serial.println(F("-- Help --"));
    Serial.println(F("Networking"));
    Serial.println(F("    Send message: send <address: byte> <message: string>"));
    Serial.println(F("        example: send 1 hello"));
    Serial.println(F("        example: send 255 hello everyone (warning this is very slow)"));
    Serial.println(F("    Send message with \"base64 encryption\": send64 <address: byte> <message: string>"));
    Serial.println(F("        example: send64 36 YWxleGQ="));
    Serial.println(F("Other"));
    Serial.println(F("    Get basic information: info"));
    Serial.println(F("    Get help: help"));
    Serial.println(F("For more info go to https://github.com/WattleFoxxo/SlippyMesh/"));
    Serial.println(F("----------"));
}

void cmdInfo() {
    Serial.println(F("-- Info --"));
    Serial.print(F("Address: "));
    Serial.println(localAddress, HEX);
    Serial.print(F("Device Version: "));
    Serial.println(radio.getDeviceVersion());
    Serial.print(F("Uptime: "));
    Serial.println(millis());
    Serial.print(F("Json: "));
    Serial.print(F("\"info\":{\"address\":"));
    Serial.print(localAddress);
    Serial.print(F(",\"network_version\":"));
    Serial.print(SLIPPY_NETWORK_VERSION);
    Serial.print(F(",\"uptime\":"));
    Serial.print(millis());
    Serial.println(F("}"));
    Serial.println(F("----------"));
}

void cmdSend() {
    uint8_t address = Serial.readStringUntil(' ').toInt();
    String message = Serial.readStringUntil('\n');
    
    if (message.length() > SLIPPY_MESSAGE_SIZE) {
        Serial.print(F("Error: Maximum message length is "));
        Serial.print(SLIPPY_MESSAGE_SIZE);
        Serial.println(F(" characters."));
        return;
    }

    sendMessage(address, message.c_str(), message.length());
}

void cmdSend64() {
    uint8_t address = Serial.readStringUntil(' ').toInt();
    String base64 = Serial.readStringUntil('\n');
    uint8_t message[63];

    uint8_t len = decode_base64(base64.c_str(), message);
    message[len] = '\0';

    sendMessage(address, message, strlen(message));
}

/* AlexD Commands */

void alexD(uint8_t from, Packet packet) {
    packet.data[packet.length] = '\0';
    
    if (!strcmp(packet.data, "/ping")) {
        String message = "pong";
        sendMessage(from, message.c_str(), message.length());
    }

    if (!strcmp(packet.data, "/uptime")) {
        String message = "uptime: "+String(millis())+" ms";
        sendMessage(from, message.c_str(), message.length());
    }
}
