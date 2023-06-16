import socket
import threading

def receive_messages(client_socket):
    while True:
        try:
            message = client_socket.recv(1024).decode()
            print(message)
        except OSError:
            break

def send_message(client_socket):
    while True:
        message = input()
        client_socket.send(message.encode())

def main():
    server_ip = '0.0.0.0'
    server_port = 7777

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((server_ip, server_port))

    receive_thread = threading.Thread(target=receive_messages, args=(client_socket,))
    receive_thread.start()

    send_thread = threading.Thread(target=send_message, args=(client_socket,))
    send_thread.start()

if __name__ == '__main__':
    main()
