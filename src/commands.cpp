#include "commands.h"

// Add your remote commands here
// Make sure you add your function to the header file
// For more help look at: https://github.com/Uberi/Arduino-CommandParser/

MyRemoteCommandParser remoteParser;

// Ignore this (c++ moment #34)
void parseCommand(char* input, char* response) {
    remoteParser.processCommand(input, response);
}

void registerRemoteCommands() {
    // Register the command here
    // eg. remoteParser.registerCommand("command name", "args", &command_function);
    remoteParser.registerCommand("ping", "", &rcmd_ping);
    remoteParser.registerCommand("uptime", "", &rcmd_uptime);
}

// Example ping/pong command
void rcmd_ping(MyRemoteCommandParser::Argument *args, char *res) {
    char* response = "pong";
    strlcpy(res, response, MyRemoteCommandParser::MAX_RESPONSE_SIZE);
}

// Example uptime command
void rcmd_uptime(MyRemoteCommandParser::Argument *args, char *res) {
    unsigned long currentMillis = millis();
    unsigned long totalSeconds = currentMillis / 1000;

    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    char response[17];
    snprintf(response, sizeof(response), "Uptime: %02d:%02d:%02d", hours, minutes, seconds);
    
    strlcpy(res, response, MyRemoteCommandParser::MAX_RESPONSE_SIZE);
}