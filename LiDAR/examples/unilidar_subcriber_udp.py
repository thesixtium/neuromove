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

dataDict = {
            'x': [],
            'y': [],
            'z': []
        }

count = 0
points_scanned = 0
start_time = time.time()
points_for_full_room = 20000

print("Start")
while True:
    points_scanned += 1
    
    if count % 1000 == 0:
        print(count)
        
    # Recv data
    data, addr = sock.recvfrom(10000)
    data = data.decode()
    search = re.search('[(]([^,]+),([^,]+),([^)]+)[)]', data)
    x = float(search.group(1)) * 100
    y = float(search.group(2)) * 100
    z = float(search.group(3)) * 100
    if z < 50:
        dataDict['x'].append(x)
        dataDict['y'].append(y)
        dataDict['z'].append(1)
        count += 1
        
    if count > points_for_full_room:
        count = 0
        print(f"Seconds per 3000 points: {((time.time() - start_time) / points_scanned) * 3000}")

        fig = plt.figure(figsize=(12, 12))
        ax = fig.add_subplot()
        ax.scatter(dataDict['x'], dataDict['y'])
        ax.scatter([0], [0])
        ax.set_xlim([-6, 6])
        ax.set_ylim([-6, 6])
        plt.show()
        df = pd.DataFrame(dataDict)

        start_time = time.time()

        grid_time = time.time()
        grid = oc_to_grid(df, max(dataDict['x'] + dataDict['y']))
        grid = add_edge_buffer(grid)
        print(f"Grid:\t{time.time() - grid_time}")

        path_time = time.time()
        path = get_full_path(jps(grid, 1, 1, 35, 36))
        print(f"Path:\t{time.time() - path_time}")

        print(f"Alg:\t{time.time() - start_time}")

        display_grid(grid)
        display_path(grid, path)
        sock.close()
        exit()


