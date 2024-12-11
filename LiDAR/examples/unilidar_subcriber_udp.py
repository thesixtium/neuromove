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

count = 0
points_scanned = 0
points_for_full_room = 40000

FEET_TO_CM = 30.48
LiDAR_RADIUS_FT = 30 * 1.3
LiDAR_DIAMETER_CM = int(LiDAR_RADIUS_FT * 2 * FEET_TO_CM) + 1
NEUROMOVE_SIZE = 100


start_time = time.time()
grid = [ [0 for _ in range(LiDAR_DIAMETER_CM)] for _ in range(LiDAR_DIAMETER_CM) ]
print(f"{round(time.time() - start_time, 4)}  :  Make Grid")
xs = []
ys = []
zs = []

while True:
    start_time = time.time()
    while count <= 5000:
        # Recv data
        data, addr = sock.recvfrom(65000)
        data = data.decode()
    
        data_split = data.split('|')
    
        for datum in data_split[1:-1]:
            points_scanned += 1
            datum_split = datum.split(",");
    
            # in centimeters
            x = float(datum_split[0]) * 100
            y = float(datum_split[1]) * 100
            z = float(datum_split[2]) * 100
            if z < 50 and abs(x) > (NEUROMOVE_SIZE/2) and abs(y) > (NEUROMOVE_SIZE/2) :
                x_scaled = int(x) + int(LiDAR_DIAMETER_CM / 2)
                y_scaled = int(y) + int(LiDAR_DIAMETER_CM / 2)
                z_scaled = z
                grid[x_scaled][y_scaled] = 1
                count += 1
                
                xs.append(x_scaled)
                ys.append(y_scaled)
                #zs.append(z_scaled)
                zs.append(1)

    print(f"{round(((time.time() - start_time) / points_scanned) * 3000, 4)}  :  Seconds per 3000 points")

    start_time = time.time()
    # This doesn't work and needs to be replaced with something to join close points
    #old_grid = [row[:] for row in grid]
    #grid = add_edge_buffer(grid)
    print(f"Grid:\t{time.time() - start_time}")
    display_grid_old(grid, old_grid)
    
    # Add borders
    start_time = time.time()
    for x in [0, len(grid)-1]:
        for y in range(len(grid[0])):
            grid[x][y] = 1
            
    for x in range(len(grid)):
        for y in [0, len(grid[0])-1]:
            grid[x][y] = 1
    print(f"{round(time.time() - start_time, 4)}  :  Add Borders")

    start_time = time.time()
    path = get_full_path(jps(grid, int(LiDAR_DIAMETER_CM / 2), int(LiDAR_DIAMETER_CM / 2), 1600, 1600))
    print(f"{round(time.time() - start_time, 4)}  :  Path finding")

    # display_grid(xs, ys, zs, int(LiDAR_DIAMETER_CM / 2))
    display_path(xs, ys, zs, int(LiDAR_DIAMETER_CM / 2), path)
    sock.close()
    exit(8)


