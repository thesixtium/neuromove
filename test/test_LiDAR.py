import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR.replace(r"/test", r"/src")))

import matplotlib.pyplot as plt
from src.LiDAR.LiDAR import LiDAR

lidar = LiDAR()
grid, origin = lidar.get_grid()

# Plot
fig = plt.figure(figsize=(6, 6))
ax = fig.add_subplot()
plt.imshow(grid)
plt.scatter(origin[0], origin[1], color="red")
plt.show()
