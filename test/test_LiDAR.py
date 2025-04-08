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

        fig = plt.figure(figsize=(6, 6))
        ax = fig.add_subplot()
        plt.imshow(grid)
        
        print(f"{sum([sum(i) for i in grid])} / {sum([len(i) for i in grid])}")


        plt.show()
