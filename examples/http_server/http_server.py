import socket
import re
import os 

HOST = '0.0.0.0'
PORT = 7776
PUBLIC = os.path.dirname(os.path.realpath(__file__))+'/public'

def serve_file(client_socket, file_path):
    try:
        with open(file_path, 'rb') as file:
            response = b'HTTP/1.1 200 OK\r\n'
            response += b'Content-Length: ' + str(len(file.read())).encode() + b'\r\n'
            response += b'Connection: close\r\n\r\n'
            file.seek(0)
            response += file.read()
    except IOError:
        response = b'HTTP/1.1 404 Not Found\r\n'
        response += b'Content-Length: 0\r\n'
        response += b'Connection: close\r\n\r\n'

    client_socket.sendall(response)

client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect((HOST, PORT))

print(f"slippy http server running on {HOST}:{PORT}")

while True:
    request = b''
    while True:
        chunk = client_socket.recv(4096)
        if not chunk:
            break
        request += chunk
        if b'\r\n\r\n' in request:
            break

    request_str = request.decode()

    method = re.search(r'^([A-Z]+)', request_str).group(1)
    path = re.search(r'^[A-Z]+\s+([^ ]+)', request_str).group(1)
    
    if path == "/":
        path = "/index.html"

    file_path = PUBLIC + path
    serve_file(client_socket, file_path)

client_socket.close()
