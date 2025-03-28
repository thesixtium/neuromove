import math

import numpy as np
import matplotlib.pyplot as plt
from src.RaspberryPi.jps import *
from io import BytesIO
from src.Frontend.frontend_methods import jps_wrapped, path_to_directions
import psutil
process = psutil.Process()
print(f"JPS Test: {process.memory_info().rss * 0.000001}")

## I M P O R T   D A T A ###
f = open("cropped_data.txt", "r")
#cropped_data = np.fromstring(f.read(), dtype=int)
cropped_data_str = f.read()
cropped_data = []
cropped_data_split = cropped_data_str.split("\n")
for row in cropped_data_split:
    cropped_data.append([int(i) for i in row.split(",")[:-1]])
cropped_data = np.array(cropped_data[:-1])
f.close()

f = open("origin.txt", "r")
origin = f.read()
origin_split = origin.replace("(", "").replace(")", "").split(", ")
origin = (int(origin_split[0]), int(origin_split[1]))
f.close()

f = open("point.txt", "r")
point = f.read()
point_split = point.replace("[", "").replace("]", "").split(" ")
point = [int(point_split[0]), int(point_split[1])]
f.close()


plt.imshow(cropped_data)
plt.gca().invert_yaxis()
plt.savefig("1_import_data.png")


cropped_data, origin, point, path = jps_wrapped(cropped_data, origin, point, display=True)

print(path)

print(path_to_directions(path))


