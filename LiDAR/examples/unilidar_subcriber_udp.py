from __future__ import print_function
import socket
import re
from jps import get_full_path, jps
from cloud_to_grid import *

print("Done imports")

# IP and Port
UDP_IP = "127.0.0.1"
UDP_PORT = 12345

# Create UDP socket
print("Make socket")
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

count = 0
points_scanned = 0
start_time = time.time()
points_for_full_room = 20000

FEET_TO_CM = 30.48
LiDAR_RADIUS_FT = 30

LiDAR_DIAMETER_CM = int(LiDAR_RADIUS_FT * 2 * FEET_TO_CM) + 1


print("Start")
while True:
    grid = [ [0 for _ in range(LiDAR_DIAMETER_CM)] for _ in range(LiDAR_DIAMETER_CM) ]

    points_scanned += 1
    
    if count % 1000 == 0:
        print(count)
        
    # Recv data
    data, addr = sock.recvfrom(10000)
    data = data.decode()
    search = re.search('[(]([^,]+),([^,]+),([^)]+)[)]', data)
    
    # in centimeters
    x = int(float(search.group(1)) * 100)
    y = int(float(search.group(2)) * 100)
    z = float(search.group(3)) * 100
    if z < 50:
        grid[x][y] = 1
        count += 1
        
    if count > points_for_full_room:
        count = 0
        print(f"Seconds per 3000 points: {((time.time() - start_time) / points_scanned) * 3000}")

        display_grid(grid)

        start_time = time.time()
        grid = add_edge_buffer(grid)
        print(f"Grid:\t{time.time() - start_time}")

        start_time = time.time()
        path = get_full_path(jps(grid, 1, 1, 35, 36))
        print(f"Path:\t{time.time() - start_time}")

        display_path(grid, path)
        sock.close()
        exit()


