#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>
#include <CommandParser.h>

// NOTE: You may need to change these parameters
#define SLIPPY_REMOTE_CMD_MAX_COMMANDS 10
#define SLIPPY_REMOTE_CMD_MAX_ARGUMENTS 5
#define SLIPPY_REMOTE_CMD_MAX_COMMAND_NAME_LENGTH 10
#define SLIPPY_REMOTE_CMD_MAX_ARGUMENT_LENGTH 64
#define SLIPPY_REMOTE_CMD_MAX_RESPONSE_LENGTH 64

typedef CommandParser<SLIPPY_REMOTE_CMD_MAX_COMMANDS, SLIPPY_REMOTE_CMD_MAX_ARGUMENTS, SLIPPY_REMOTE_CMD_MAX_COMMAND_NAME_LENGTH, SLIPPY_REMOTE_CMD_MAX_ARGUMENT_LENGTH, SLIPPY_REMOTE_CMD_MAX_RESPONSE_LENGTH> MyRemoteCommandParser;

void parseCommand(char* input, char* response);
void registerRemoteCommands();

void rcmd_ping(MyRemoteCommandParser::Argument *args, char *res);
void rcmd_uptime(MyRemoteCommandParser::Argument *args, char *res);

#endif
