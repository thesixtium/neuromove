import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR.replace(r"/test", r"/src")))

from src.LiDAR.build.RunLiDAR import RunLiDAR
from src.RaspberryPi.SharedMemory import SharedMemory
import matplotlib.pyplot as plt
import numpy as np
import time

occupancy_grid_memory = SharedMemory(shem_name="occupancy_grid", size=28462200, create=True)

RunLiDAR()
print("LiDAR thread started")

start_time = time.time()
hertz = []
while True:
    grid = occupancy_grid_memory.read_grid()

    if len(grid) > 1:
        convolved_grid = [[0 for _ in grid[0]] for _ in grid]

        for i in range(len(grid)):
            for j in range(len(grid[0])):
                if grid[i][j] == 1:
                    x_range = range(max(0, i-3), min(len(grid), i+4))
                    y_range = range(max(0, j-3), min(len(grid[0]), j+4))

                    for x in x_range:
                        for y in y_range:
                            convolved_grid[x][y] = 1

        fig = plt.figure(figsize=(6, 6))
        ax = fig.add_subplot()
        plt.imshow(grid)
        plt.imshow(convolved_grid, alpha=0.5)

        plt.show()
