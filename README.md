# Slippy Mesh
A lightweight lora mesh network written in c++ 

[![Discord](https://img.shields.io/discord/1172014752604631070?style=flat-square&logo=discord)](https://discord.gg/7EZBvFvg9A)
![GitHub Repo stars](https://img.shields.io/github/stars/WattleFoxxo/SlippyMesh?style=flat-square&logo=github)


## Goals
- [x] Arduino Mega Compatable
- [x] Flooded mesh networking
- [x] Routed mesh networking
- [ ] API for external programs
- [ ] Encryption
- [ ] More to come

<br>

## Usage
To send a message to everyone open your serial and type
```
send 0xFFFFFFFF "hello world"
```

<br>

## Commands
### Help
Gets a list of commands
```
help
```

### Send
Sends a messsage to the target address
```
send <address> <message>
```
**Examples**
```
sendex 0x01234567 "hello"
```
> Sends a message to 0x01234567

```
sendex 0xFFFFFFFF "hello everyone"
```
> Sends a message to everyone

### Send Ex
Sends a message to the target address with some extra options
```
sendex <address> <message> <type> <service> <flags>
```
**Examples**
```
sendex 0x01234567 "hello" 0 0 0b00000000
```
> Sends a message to 0x01234567 (same as 'send 0x01234567 "hello"')

```
sendex 0x01234567 "ping" 0 1 0b00000000
```
> Sends a remote command to 0x01234567

```
sendex 0xFFFFFFFF "hello everyone" 0 0 0b00000000
```
> Sends a message to everyone (same as 'send 0xFFFFFFFF "hello everyone"')

```
sendex 0xFFFFFFFF "hello neighbors" 0 0 0b10000000
```
> Sends a message to neighboring nodes

### Send 64
Sends a message to the target address with some extra options and base64
```
send64 <address> <base64> <type> <service> <flags>
```

**Example**
```
send64 0x01234567 "aGVsbG8=" 0 0 0b00000000
```
> Sends a message to 0x01234567 (this is sendex with base64)

<br>

## Remote commands
To send a remote command you need to use sendx
```
sendex <address> "<command> <args>" 0 1 0b00000000
```

### Default commands
```
ping
```
> Will return "pong"

```
uptime
```
> Will return the nodes uptime in "HH:MM:SS"

<br>

### Adding your own remote commands

In `src/commands.cpp` create a function called rcmd_yourcommandname
```c++
void rcmd_yourcommandname(MyRemoteCommandParser::Argument *args, char *res) {
    char* response = "hello world";
    strlcpy(res, response, MyRemoteCommandParser::MAX_RESPONSE_SIZE);
}
```

Then add this at the bottom of `void registerRemoteCommands() {}`
```c++
remoteParser.registerCommand("yourcommandname", "", &rcmd_yourcommandname);
```

Lastly add this before `#endif` in `src/commands.h`
```c++
void rcmd_yourcommandname(MyRemoteCommandParser::Argument *args, char *res);
```

<br>

## External Programs
 - [SlippyRouter (coming soon)](https://github.com/)
