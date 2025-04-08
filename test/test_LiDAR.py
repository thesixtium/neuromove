import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR.replace(r"/test", r"/src")))

from src.LiDAR.build.RunLiDAR import RunLiDAR
from src.RaspberryPi.SharedMemory import SharedMemory
import matplotlib.pyplot as plt
import numpy as np
import time

occupancy_grid_memory = SharedMemory(shem_name="occupancy_grid", size=284622, create=True)
LiDAR_runs_memory = SharedMemory(shem_name="LiDAR_runs", size=284622, create=True)

RunLiDAR()
print("LiDAR thread started")

start_time = time.time()
hertz = []
while True:
    value = occupancy_grid_memory.read_grid()

    if value:
        print(value.split("|")[:-1])
        data = {'x': [], 'y': [], 'z': []}
        grid = [[int(j) for j in i] for i in value.split("|")[:-1]]

        for x in range(len(grid)):
            for y in range(len(grid[0])):
                data['x'].append(x)
                data['y'].append(y)
                data['z'].append(grid[x][y])

        plt.subplot(1, 2, 1)
#        fig = plt.figure(figsize=(6, 6))
#        ax = fig.add_subplot()
        plt.scatter(data['x'], data['y'], data['z'])
        plt.pause(0.05)

        plt.subplot(1, 2, 2)
        runs = LiDAR_runs_memory.read_string()
        if runs:
            runs = float(runs)
            now = time.time() - start_time
            hertz.append(now / runs)
        plt.plot(hertz)
    else:
        print("No value")

    plt.show()
