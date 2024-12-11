import socket
import struct
import matplotlib.pyplot as plt
import re
import time

# IP and Port
UDP_IP = "127.0.0.1"
UDP_PORT = 12345

# Create UDP socket
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
points_for_full_room = 10000 

while True:
    points_scanned += 1
        
    # Recv data
    data, addr = sock.recvfrom(10000)
    data = data.decode()
    search = re.search('[(]([^,]+),([^,]+),([^)]+)[)]', data)
    if count % (points_for_full_room/100) == 0:
        print(f"{round(count/points_for_full_room * 100, 2)}")
    x = float(search.group(1))
    y = float(search.group(2))
    z = float(search.group(3))
    if z < 0.5:
        dataDict['x'].append(x)
        dataDict['y'].append(y)
        dataDict['z'].append(1)
        count += 1
        
    if count > points_for_full_room:
        count = 0
        print(f"Runtime: {(time.time() - start_time)} seconds")
        print(f"Points:  {points_scanned}")
        print(f"Seconds per point: {(time.time() - start_time) / points_scanned}")
        print(f"Seconds per 3000 points: {((time.time() - start_time) / points_scanned) * 3000}")
        start_time = time.time()
        fig = plt.figure(figsize=(12,12))
        #ax = fig.add_subplot(projection='3d')
        #ax.scatter(dataDict['x'], dataDict['y'], dataDict['z'], alpha=0.1)
        ax = fig.add_subplot()
        ax.scatter(dataDict['x'], dataDict['y'])
        ax.scatter([0], [0])
        ax.set_xlim([-6, 6])
        ax.set_ylim([-6, 6])
        plt.show()
        exit()

sock.close()
