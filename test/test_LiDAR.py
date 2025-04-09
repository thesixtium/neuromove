import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR.replace(r"/test", r"/src")))

import matplotlib.pyplot as plt
from src.LiDAR.LiDAR import LiDAR
from src.RaspberryPi.jps import get_full_path, jps

lidar = LiDAR()
grid, origin = lidar.get_grid()

# Plot
fig = plt.figure(figsize=(6, 6))
ax = fig.add_subplot()
plt.imshow(grid)
plt.scatter(origin[0], origin[1], color="red")



### A D D   B O R D E R ###
for x in range(len(grid)):
    grid[x][0] = 1
    grid[x][len(grid[0])-1] = 1

for y in range(len(grid[0])):
    grid[0][y] = 1
    grid[len(grid)-1][y] = 1


### P A T H ###
flipped_path = get_full_path(jps(grid, origin[1], origin[0], 10, 10))
path = []
for path_point in flipped_path:
    path.append((path_point[1], path_point[0]))
plt.plot(*zip(*path))

plt.axis('off')
plt.show()
