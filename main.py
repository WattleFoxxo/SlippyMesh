import slippylib

PORT = "/dev/ttyACM1"
BAUDRATE = 115200
PROTOCOL = "5"
CHUCNK_SIZE = 64

LOCAL_ADDRESS = "4"
NAME = "fff"


def on_recive(sender_address, target_address, protocol, message):
    if protocol != PROTOCOL:
        return
    
    name = message.split(";")[0]
    message =  message[len(name)+1:]

    if target_address == LOCAL_ADDRESS:
        print(f"[from {sender_address}] [{name}@{sender_address}] {message}")
    else:
        print(f"[{name}@{sender_address}] {message}")

print("simple chat setup")
LOCAL_ADDRESS = int(input("(1/2) unique address, 1 - 254: "))
if int(LOCAL_ADDRESS) < 1 or int(LOCAL_ADDRESS) > 254:
    input("the range of address is 1 - 254, press enter to exit...")
    exit()

NAME = input("(2/2) name: ")
if len(NAME) > 16:
    input("max name length is 16 characters, press enter to exit...")
    exit()

if not NAME.isalnum():
    input("your name has to be alpha numeric, press enter to exit...")
    exit()

print("all set up")
slippy = slippylib.SlippyDevice(PORT, BAUDRATE, LOCAL_ADDRESS, on_recive)

while True:
    message = input()
    target_address = slippylib.EVERYBODY_ADDRESS

    if len(message) <= 32:
        slippy.emit(target_address, PROTOCOL, f"{NAME};{message}")
    else:
        print("error: max message length is 32 characters")
