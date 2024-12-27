import socket
import re
from jps import get_full_path, jps
from cloud_to_grid import *

from src.RaspberryPi.SharedMemory import SharedMemory

sm = SharedMemory("grid", )

while True:
    print(sm.read_string())


