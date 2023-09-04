# Slippy Mesh V4
A lightweight flodded mesh-network

## Goals
- [x] Basic data transmission
- [x] Flooded mesh networking
- [ ] Encryption
- [ ] More to come

## Instalation
1. Open `SlippyFirmware/SlippyFirmware.ino` in the arduino ide.

2. Make sure you install the required libraries.
 - [Arduino LoRa](https://github.com/WattleFoxxo/arduino-LoRa)*
 - [Arduino Unique ID](https://github.com/ricaun/ArduinoUniqueID)
 - [AT Commands](https://github.com/yourapiexpert/ATCommands)

> libraries marked with "*" are custom forks.

3. Edit the pin configuration for your board.
The defualt configuration is for the "Dragino LoRa Shield" with the SX1276 ic.
```c++
// LoRa Pins
#define LORA_CS_PIN 10
#define LORA_RESET_PIN 9
#define LORA_IRQ_PIN 2
```

4. Next make sure the modem config is the same for all users.
```c++
// LoRa Modem Settings
#define LORA_SF 10
#define LORA_BW 256E3
#define LORA_CR 8
#define LORA_PREAMBLE_LENGTH 8
```

5. Edit these settings to fit your needs. 
```c++
// Slippy Settings
#define SLIPPY_AUTO_RESTART false
#define SLIPPY_AUTO_RESTART_WARNING false
#define SLIPPY_AUTO_RESTART_TIME 30
#define SLIPPY_AUTO_RESTART_WARNING_TIME 25
#define SLIPPY_AUTO_RESTART_WARNING_MESSAGE "This node will be restarting in 5 minutes!"
#define SLIPPY_DEBUG_MESSAGES false
#define SLIPPY_TIMEOUT 2000
```

6. Now upload the code to your arduino.

## Usage
Open your preferred serial monitor and set the baud rate to `115200` and line eding to `NL+CR`

To send a message this syntax is used `AT+SEND=<address>,<message>`
```
AT+SEND=192,168,0,1,I hAvE yoUr Ip
```
```
AT+SEND=255,255,255,255,Hello Everyone
```
> note the address is separated by a comma.

To get help infomation type
```
AT+HELP
```

## Remote commands
The at-command listerner execution daemon or alexd is a simple way to get infomation from other nodes.

The defualt alexd commands are

```
AT+PING
```
> will return "PONG".


```
AT+UPTIME
```
> will return the nodes uptime.


```
AT+TEMP
```
> will return the temprature of the nodes SX1276 ic.

To send these commands use this syntax `AT+SEND=<address>,<alexd command>`

### Adding your own alexd commands

In `SlippyFirmware/SlippyFirmware.ino` at the bottom of the 
`void alexD(SlippyDataStreamPacket *packet)` function add
```c++
if (!strcmp(packet->data, "AT+<COMMAND NAME IN FULL CAPS>")) {
    /* DO NOT EDIT */
    dataStreamOut.sourceAddress = localAddress;
    dataStreamOut.destinationAddress = packet->sourceAddress;
    dataStreamOut.packetId = random(0xFFFF);
    /* DO NOT EDIT END */

    // Do stuff here

    strcpy(dataStreamOut.data, "<PUT YOUR RESPONCE HERE>");

    /* DO NOT EDIT */
    addIDtoPacketBucket(dataStreamOut.packetId);
    sendPacket(&dataStreamOut);
    /* DO NOT EDIT END */
  }
```