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
points_for_full_room = 20000

FEET_TO_CM = 30.48
LiDAR_RADIUS_FT = 30 * 1.1

LiDAR_DIAMETER_CM = int(LiDAR_RADIUS_FT * 2 * FEET_TO_CM) + 1


print("Start")

start_time = time.time()
grid = [ [0 for _ in range(LiDAR_DIAMETER_CM)] for _ in range(LiDAR_DIAMETER_CM) ]
print(f"Make Grid: {time.time() - start_time}")

start_time = time.time()
while True:
    points_scanned += 1
        
    # Recv data
    data, addr = sock.recvfrom(10000)
    data = data.decode()
    data_split = data[1:-1].split(',')
    
    # search = re.search('[(]([^,]+),([^,]+),([^)]+)[)]', data)
    
    # in centimeters
    x = int(float(data_split[0]) * 100) + int(LiDAR_DIAMETER_CM / 2)
    y = int(float(data_split[1]) * 100) + int(LiDAR_DIAMETER_CM / 2)
    z = float(data_split[2]) * 100
    if z < 100:
        grid[x][y] = 1
        count += 1
        
    if count > points_for_full_room:
        count = 0
        print(f"Seconds per 3000 points: {((time.time() - start_time) / points_scanned) * 3000}")
        
        # start_time = time.time()
        # grid = add_edge_buffer(grid)
        # print(f"Grid:\t{time.time() - start_time}")

        # start_time = time.time()
        # path = get_full_path(jps(grid, 1, 1, 35, 36))
        # print(f"Path:\t{time.time() - start_time}")

        #display_grid(grid)
        # display_path(grid, path)
        sock.close()
        exit()


