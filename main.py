# pipreqs src --ignore src/LiDAR

#!/usr/bin/env python3

import time
import logging
import numpy as np
import signal
import sys

from src.RaspberryPi.Driving import Driving
from src.RaspberryPi.SharedMemory import SharedMemory

def main():
    # Starting variables
    pookinator_memory = SharedMemory(shem_name="pookinator", size=10, create=False)
    driving = Driving()

    while True:
        direction = pookinator_memory.read_local_driving()
        driving.drive_one_unit( direction )


if __name__ == '__main__':
    main()
