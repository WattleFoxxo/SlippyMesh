import threading
import serial
import time

EVERYBODY_ADDRESS = "0"
NOBODY_ADDRESS = "0"

#   reserved protocols:
#       0 -> info
#       1 -> text
#       2 -> file
#       3 -> message

#   reserved address:
#       0 -> everybody
#       255 -> nobody

def recive_thread(device, callback, local_address):
    while True:
        data = b''
        while True:
            data += device.read()
            if b'\r\n\r\n' in data:
                break

        packet = data.decode("utf-8")
        header = str(packet).split(";")
        header_len = len(header[0])+len(header[1])+len(header[2])
        callback(header[0], header[1], header[2], packet[header_len+3:-4])

def send_thread(device, local_address, target_address, protocol, data):
    buffer = []
    device.write(bytes(f"{local_address};{target_address};{protocol};{data}", "utf-8"))

class SlippyDevice():
    def __init__(self, port, baudrate, local_address, on_recive):
        self.device = serial.Serial(port=port, baudrate=baudrate)
        self.local_address = local_address

        reciveThread = threading.Thread(target=recive_thread, args=(self.device, on_recive, local_address,))
        reciveThread.start()

    def emit(self, target_address, protocol, data):
        target_address = str(target_address)
        protocol = str(protocol)
        sendThread = threading.Thread(target=send_thread, args=(self.device, self.local_address, target_address, protocol, data,))
        sendThread.start()
