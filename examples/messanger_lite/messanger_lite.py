import socket
import threading
import sys

NAME = "anom"

def receive_messages(client_socket):
    while True:
        message = b''
        while True:
            chunk = client_socket.recv(4096)
            if not chunk:
                break
            message += chunk
            if b'\r\n\r\n' in message:
                break

        message = message.decode()
        nickname = message.split(";")[0]
        print(f"[{nickname}] {message[len(nickname)+1:-4]}")

def send_message(client_socket):
    while True:
        message = input()
        message = f"{NAME};{message} \r\n\r\n"
        client_socket.send(message.encode())

def main():
    #if len(sys.argv) != 4:
    #    print("Usage: python chat_client.py <server_ip> <server_port> <nickname>")
    #    return

    server_ip = sys.argv[1]
    server_port = int(sys.argv[2])

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((server_ip, server_port))

    receive_thread = threading.Thread(target=receive_messages, args=(client_socket,))
    receive_thread.start()

    send_thread = threading.Thread(target=send_message, args=(client_socket,))
    send_thread.start()

if __name__ == '__main__':
    main()
