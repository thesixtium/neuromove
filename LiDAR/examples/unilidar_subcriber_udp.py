import socket
import re
from jps import get_full_path, jps
from cloud_to_grid import *

# IP and Port
UDP_IP = "127.0.0.1"
UDP_PORT = 12345

# Create UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

sock.settimeout(10)

while True:
    data, addr = sock.recvfrom(142044)
    print(2)
    data = data.decode()
    print(3)
    data_split = data.split('|')
    
    print(data_split)
    
    sock.close()
    exit(8)


