#include "main.h"

#include <avr/power.h>

MyCommandParser serialParser;

void(*RESET_FUNC)(void) = 0x00;

uint8_t incomingBuffer[RH_MESH_MAX_MESSAGE_LEN];

uint64_t heartBeatTimer = 0;
uint64_t messageTimer = 0;

uint8_t uid_index = 0;
uint32_t uids[SLIPPY_MAX_UID];

RH_RF95 radio;
RHMesh *manager;

void automessage() {
    uint64_t uptime_ms = millis();
    uint8_t uptime_hours = uptime_ms / 1000 / 60 / 60;
    uint8_t uptime_mins = (uptime_ms / 1000 / 60) % 60;
    uint8_t uptime_secs = (uptime_ms / 1000) % 60;

    uint64_t countdown_ms = 21600000 - uptime_ms;
    uint8_t countdown_hours = countdown_ms / 1000 / 60 / 60;
    uint8_t countdown_mins = (countdown_ms / 1000 / 60) % 60;
    uint8_t countdown_secs = (countdown_ms / 1000) % 60;
    
    char timestr[200];
    sprintf(timestr, "\n**Automated message from Taylor**\n\nUptime: %02d:%02d:%02d\nNext reboot: %02d:%02d:%02d\nNext message: 00:59:59\n\nFor more info visit https://github.com/WattleFoxxo/SlippyMesh\n", uptime_hours, uptime_mins, uptime_secs, countdown_hours, countdown_mins, countdown_secs);

    SlippyPacket newPacket;

    newPacket.service = SLIPPY_PACKET_SERVICE_TXT;
    newPacket.uid = random32bit();
    newPacket.size = strlen(timestr);

    memcpy(newPacket.data, timestr, newPacket.size);
    addUID(newPacket.uid);

    uint8_t error = sendPacket(newPacket, SLIPPY_BROADCAST_ADDRESS);
    Serial.println(getErrorString(error));
}

void setup() {
    randomSeed(analogRead(0));
	Serial.begin(SLIPPY_SERIAL_BUAD);

    // LOW POWER

    pinMode(13, OUTPUT);
    digitalWrite(13, LOW);

    pinMode(A0, OUTPUT);
    pinMode(A1, OUTPUT);
    pinMode(A2, OUTPUT);
    pinMode(A3, OUTPUT);
    pinMode(A4, OUTPUT);
    pinMode(A5, OUTPUT);
    pinMode(A6, OUTPUT);
    pinMode(A7, OUTPUT);
    pinMode(A8, OUTPUT);
    pinMode(A9, OUTPUT);
    pinMode(A10, OUTPUT);
    pinMode(A11, OUTPUT);
    pinMode(A12, OUTPUT);
    pinMode(A13, OUTPUT);
    pinMode(A14, OUTPUT);
    pinMode(A15, OUTPUT);

    digitalWrite(A0, LOW);
    digitalWrite(A1, LOW);
    digitalWrite(A2, LOW);
    digitalWrite(A3, LOW);
    digitalWrite(A4, LOW);
    digitalWrite(A5, LOW);
    digitalWrite(A6, LOW);
    digitalWrite(A7, LOW);
    digitalWrite(A8, LOW);
    digitalWrite(A9, LOW);
    digitalWrite(A10, LOW);
    digitalWrite(A11, LOW);
    digitalWrite(A12, LOW);
    digitalWrite(A13, LOW);
    digitalWrite(A14, LOW);
    digitalWrite(A15, LOW);

    power_adc_disable();
    // power_spi_disable();
    power_usart0_disable();
    power_usart2_disable();
    power_timer1_disable();
    power_timer2_disable();
    power_timer3_disable();
    power_timer4_disable();
    power_timer5_disable();
    power_twi_disable();

    delay(500);

    Serial.println(F("Starting Slippy Mesh..."));

    // Get a unique id
    uint32_t uuid = 0;
    for (int i = 0; i < 4; i++) {
        uuid |= (uint32_t)UniqueID8[i+4] << (i * 8);
    }

    manager = new RHMesh(radio, uuid);
    if (!manager->init()) {
        Serial.println(F("ERROR: Failed to initialize the radio"));
        while(true);
    }

    // set radio parameters
    radio.setTxPower(SLIPPY_RADIO_POWER, false); // :(
    radio.setFrequency(SLIPPY_RADIO_FREQUENCY);
    radio.setCADTimeout(SLIPPY_RADIO_CAD_TIMEOUT);

    // Register commands
    serialParser.registerCommand("help", "", &cmd_help);
    serialParser.registerCommand("send", "us", &cmd_send);
    serialParser.registerCommand("sendex", "usuuu", &cmd_sendex);
    serialParser.registerCommand("send64", "usuuu", &cmd_send64);
    registerRemoteCommands();

    Serial.println(F("Ready!"));
    Serial.print(F("Your address is: "));
    printAddress(manager->thisAddress());
    Serial.println();
    Serial.println(SLIPPY_SERIAL_WELCOME_MESSAGE);

    heartBeat();
    delay(1000);
    automessage();
    delay(1000);
}

void loop() {
    reciveMessage();

    if (Serial.available()) {
        char line[512];
        size_t lineLength = Serial.readBytesUntil('\n', line, 512);
        line[lineLength] = '\0';

        char response[MyCommandParser::MAX_RESPONSE_SIZE];
        serialParser.processCommand(line, response);
    }

    if (millis() - heartBeatTimer >= 900000) {
        heartBeatTimer = millis();
        heartBeat(); // Heart beat (15M)
    }

    if (millis() - messageTimer >= 3600000) {
        messageTimer = millis();
        automessage(); // Message (1H)
    }

    if (millis() >= 21600000) RESET_FUNC(); // Restart (6H)
}

// Processes incoming packets
void reciveMessage() {
    uint8_t len = sizeof(incomingBuffer);
    uint32_t from;
    uint32_t dest;

    if (manager->recvfromAckTimeout(incomingBuffer, &len, 500, &from, &dest)) {
        SlippyPacket newPacket;
        memcpy(&newPacket, incomingBuffer, len);        

        // Make sure the packet hasnt been processed
        if (hasUID(newPacket.uid)) return;
        addUID(newPacket.uid);

        // Global broadcasting
        if (dest == SLIPPY_BROADCAST_ADDRESS && !getFlag(newPacket.flags, SLIPPY_PACKET_FLAG_LOCAL_BROADCAST)) {
            uint8_t packetSize = SLIPPY_PACKET_SIZE + newPacket.size;
            uint8_t packetBuff[packetSize];
            memcpy(packetBuff, &newPacket, packetSize);

            manager->sendtoFromSourceWait(packetBuff, sizeof(packetBuff), SLIPPY_BROADCAST_ADDRESS, from, 0); // ooOoOoOOoOoo spoofing 
        }

        // Handle different packet services
        if (newPacket.service == SLIPPY_PACKET_SERVICE_TXT) {
            // PLAIN TEXT MESSAGE NOT FOR USE WITH PROGRAMS
            // Just print the raw message

            Serial.println();

            if (dest == SLIPPY_BROADCAST_ADDRESS) {
                if (getFlag(newPacket.flags, SLIPPY_PACKET_FLAG_LOCAL_BROADCAST)) {
                    Serial.print(F("Local Broadcast "));
                } else {
                    Serial.print(F("Broadcast "));
                }
            } else {
                Serial.print(F("Direct "));
            }

            Serial.print(F("message from ["));
            printAddress(from);
            Serial.print(F("]: "));

            printStringBuffer(newPacket.data, newPacket.size);
            Serial.println();

            Serial.print(F("JSON: "));
            printPacket(&newPacket, from, dest);
            Serial.println("\n");

        } else if (newPacket.service == SLIPPY_PACKET_SERVICE_EXE) {
            // REMOTE COMMANDS
            // dont print anything, processes the commands then send the results

            uint8_t strbuff[newPacket.size+1];
            memcpy(strbuff, &newPacket.data, newPacket.size);
            strbuff[newPacket.size] = '\0';

            char response[MyCommandParser::MAX_RESPONSE_SIZE];
            parseCommand(strbuff, response);

            if (strlen(response)) {
                SlippyPacket responsePacket;
                responsePacket.service = SLIPPY_PACKET_SERVICE_TXT;
                responsePacket.uid = random32bit();
                responsePacket.size = strlen(response);
                memcpy(responsePacket.data, response, responsePacket.size);
                addUID(responsePacket.uid);
                sendPacket(responsePacket, from);
            }
        } else if (newPacket.service == SLIPPY_PACKET_SERVICE_ALIVE && SLIPPY_VIEW_HEART_BEAT_MESSAGES) {
            // HEART BEAT
            // to know what nodes are online

            Serial.print(F("Heart beat from ["));
            printAddress(from);
            Serial.print(F("]: "));

            printStringBuffer(newPacket.data, newPacket.size);
            Serial.println();

            Serial.print(F("JSON: "));
            printPacket(&newPacket, from, dest);
            Serial.println();
        } else if (newPacket.service > 16) {
            // EXTERNAL SERVICES (FOR USE WITH PROGRAMS)
            // Dont bother printing the raw data

            Serial.print(F("Packet from ["));
            printAddress(from);
            Serial.println(F("]"));

            Serial.print(F("JSON: "));
            printPacket(&newPacket, from, dest);
            Serial.println();

        }

    }
}

// Sends a packet to the target address
uint8_t sendPacket(SlippyPacket packet, uint32_t address) {
    if (address == manager->thisAddress()) return 6;

    // Convert the struct to a byte array
    uint8_t packetSize = SLIPPY_PACKET_SIZE + packet.size;
    uint8_t packetBuff[packetSize];
    memcpy(packetBuff, &packet, packetSize);

    return manager->sendtoWait(packetBuff, packetSize, address);
}

void heartBeat() {
    SlippyPacket newPacket;

    newPacket.service = SLIPPY_PACKET_SERVICE_ALIVE;
    newPacket.uid = random32bit();
    newPacket.size = sizeof(SLIPPY_CUSTOM_HEART_BEAT);

    memcpy(newPacket.data, SLIPPY_CUSTOM_HEART_BEAT, newPacket.size);
    addUID(newPacket.uid);

    sendPacket(newPacket, SLIPPY_BROADCAST_ADDRESS);
}

// Prints a address
void printAddress(uint32_t address) {
    Serial.print("0x");
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (address >> (i * 4)) & 0x0F;
        Serial.print(nibble, HEX);
    }
}

// Prints a buffer as numbers
void printBuffer(uint8_t* buffer, uint8_t lenght, char* dilemma=", ") {
    for (int i = 0; i < lenght; i++) {
        Serial.print(buffer[i], DEC);
        Serial.print(dilemma);
    }
}

// Prints a buffer as chars
void printStringBuffer(uint8_t* buffer, uint8_t lenght) {
    for (int i = 0; i < lenght; i++) {
        Serial.print((char)buffer[i]);
    }
}

// Prints each flag from a flags veriable
void printFlags(uint8_t flags) {
    for (uint8_t i = 0; i < 8; i++) {
        if (i != 0) {
            Serial.print(", ");
        }

        if (getFlag(flags, i)) {
            Serial.print(F("true"));
        } else {
            Serial.print(F("false"));
        }
    }
}

// Print a packet and other info in the json format
void printPacket(SlippyPacket* newPacket, uint32_t from, uint32_t dest) {
    // Calculate size of base64 string
    unsigned char base64[(((4 * newPacket->size / 3) + 3) & ~3)]; 
    encode_base64(newPacket->data, newPacket->size, base64);

    // Very clean code

    Serial.print(F("{\"from\":\""));
    printAddress(from);
    Serial.print(F("\","));

    Serial.print(F("\"to\":\""));
    printAddress(dest);
    Serial.print(F("\","));

    Serial.print(F("\"rssi\":"));
    Serial.print(radio.lastRssi(), DEC);
    Serial.print(F(","));
            
    Serial.print(F("\"snr\":"));
    Serial.print(radio.lastSNR(), DEC);
    Serial.print(F(","));

    Serial.print(F("\"type\":"));
    Serial.print(newPacket->type, DEC);
    Serial.print(F(","));

    Serial.print(F("\"service\":"));
    Serial.print(newPacket->service, DEC);
    Serial.print(F(","));

    Serial.print(F("\"flags\":["));
    printFlags(newPacket->flags);
    Serial.print(F("],"));

    Serial.print(F("\"uid\":\""));
    printAddress(newPacket->uid);
    Serial.print(F("\","));

    Serial.print(F("\"size\":"));
    Serial.print(newPacket->size, DEC);
    Serial.print(F(","));

    Serial.print(F("\"data\":\""));
    Serial.print((char*)base64);
    Serial.print(F("\"}"));
}

// Generate a random 32bit number
uint32_t random32bit() {
    uint32_t result = 0;
    
    result |= (uint32_t)random(0, 0xFFFF) << 0;
    result |= (uint32_t)random(0, 0xFFFF) << 16;

    return result;
}

// Add a UID to the UID pool
void addUID(uint32_t uid) {
    uid_index += 1;

    if (uid_index >= SLIPPY_MAX_UID) {
        uid_index = 0;
    }

    uids[uid_index] = uid;
}

// Check if the UID pool has a UID
bool hasUID(uint32_t uid) {
    for (uint8_t i = 0; i < SLIPPY_MAX_UID; i++) {
        if (uids[i] == uid) {
            return true;
        }
    }
    
    return false;
}

// Gets the seleted flag out of the flags byte
bool getFlag(uint8_t flags, uint8_t bit) {
    return (flags & (1 << bit)) != 0;
}

// Sets the selected flag of a flags byte
void setFlag(uint8_t &flags, uint8_t bit, bool value) {
    if (value) {
        flags |= (1 << bit);
    } else {
        flags &= ~(1 << bit);
    }
}

// Returns a string for each error code
char* getErrorString(uint8_t error) {
    switch(error) {
        case 0:
            return "ok";
        case 1:
            return "error: Packet is too large";
            break;
        case 2:
            return "error: No valid route";
            break;
        case 3:
            return "error: Timeout";
            break;
        case 4:
            return "error: No reply";
            break;
        case 5:
            return "error: Unable to deliver";
            break;
        case 6:
            return "error: Invalid address";
            break;
    }
    return "error: unknown error";
}

/* CMDS */

// Help command
void cmd_help(MyCommandParser::Argument *args, char *response) {
    Serial.println(F("Commands:"));
    Serial.println(F("  send <address> <message>"));
    Serial.println(F("    eg. 'sendex 0x01234567 \"hello\"' Sends a message to 0x01234567"));
    Serial.println(F("    eg. 'sendex 0xFFFFFFFF \"hello everyone\"' Sends a message to everyone"));
    Serial.println(F("  sendex <address> <message> <type> <service> <flags>"));
    Serial.println(F("    eg. 'sendex 0x01234567 \"hello\" 0 0 0b00000000' Sends a message to 0x01234567 (same as 'send 0x01234567 \"hello\"')"));
    Serial.println(F("    eg. 'sendex 0x01234567 \"ping\" 0 1 0b00000000' Sends a remote command to 0x01234567"));
    Serial.println(F("    eg. 'sendex 0xFFFFFFFF \"hello everyone\" 0 0 0b00000000' Sends a message to everyone (same as 'send 0xFFFFFFFF \"hello everyone\"')"));
    Serial.println(F("    eg. 'sendex 0xFFFFFFFF \"hello neighbors\" 0 0 0b10000000' Sends a message to neighboring nodes"));
    Serial.println(F("  send64 <address> <base64> <type> <service> <flags>"));
    Serial.println(F("    eg. 'send64 0x01234567 \"aGVsbG8=\" 0 0 0b00000000' Sends a message to 0x01234567 (this is sendex with base64)"));
}

// send <address> <message>
// eg. 'sendex 0x01234567 "hello"' Sends a message to 0x01234567
// eg. 'sendex 0xFFFFFFFF "hello everyone"' Sends a message to everyone
void cmd_send(MyCommandParser::Argument *args, char *response) {
    SlippyPacket newPacket;

    newPacket.service = SLIPPY_PACKET_SERVICE_TXT;
    newPacket.uid = random32bit();
    newPacket.size = strlen(args[1].asString);
    memcpy(newPacket.data, args[1].asString, newPacket.size);

    addUID(newPacket.uid);

    Serial.print("Sending... ");

    uint8_t error = sendPacket(newPacket, args[0].asUInt64);
    Serial.println(getErrorString(error));
}

// sendex <address> <message> <type> <service> <flags>
// eg. 'sendex 0x01234567 "hello" 0 0 0b00000000' Sends a message to 0x01234567 (same as 'send 0x01234567 "hello"')
// eg. 'sendex 0x01234567 "ping" 0 1 0b00000000' Sends a remote command to 0x01234567
// eg. 'sendex 0xFFFFFFFF "hello everyone" 0 0 0b00000000' Sends a message to everyone (same as 'send 0xFFFFFFFF "hello everyone"')
// eg. 'sendex 0xFFFFFFFF "hello neighbors" 0 0 0b10000000' Sends a message to neighboring nodes
void cmd_sendex(MyCommandParser::Argument *args, char *response) {
    SlippyPacket newPacket;

    newPacket.type = args[2].asUInt64;
    newPacket.service = args[3].asUInt64;
    newPacket.flags = args[4].asUInt64;
    newPacket.uid = random32bit();
    newPacket.size = strlen(args[1].asString);
    memcpy(newPacket.data, args[1].asString, newPacket.size);

    addUID(newPacket.uid);

    Serial.print("Sending... ");

    uint8_t error = sendPacket(newPacket, args[0].asUInt64);
    Serial.println(getErrorString(error));
}

// send64 <address> <base64> <type> <service> <flags>
// eg. 'send64 0x01234567 "aGVsbG8=" 0 0 0b00000000' Sends a message to 0x01234567 (this is sendex with base64)
void cmd_send64(MyCommandParser::Argument *args, char *response) {
    uint8_t base64data[MyRemoteCommandParser::MAX_COMMAND_ARG_SIZE];
    uint8_t len = decode_base64(args[1].asString, base64data);

    SlippyPacket newPacket;
    newPacket.type = args[2].asUInt64;
    newPacket.service = args[3].asUInt64;
    newPacket.flags = args[4].asUInt64;
    newPacket.uid = random32bit();
    newPacket.size = len;
    memcpy(newPacket.data, base64data, len);

    addUID(newPacket.uid);

    Serial.print("Sending... ");

    uint8_t error = sendPacket(newPacket, args[0].asUInt64);
    Serial.println(getErrorString(error));
}
