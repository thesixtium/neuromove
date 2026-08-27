# pipreqs src --ignore src/LiDAR

#!/usr/bin/env python3

import time
import logging
import numpy as np
import signal
import sys

from src.RaspberryPi.Driving import Driving
from src.RaspberryPi.SharedMemory import SharedMemory
from src.RaspberryPi.States import MotorDirections

def main():
    # Starting variables
    pookinator_memory = SharedMemory(shem_name="pookinator", size=10, create=False)
    driving = Driving()

    while True:
        value = pookinator_memory.read_string()

        # Nothing new has been written yet
        if not value:
            time.sleep(0.01)
            continue

        # Consume the selection by clearing shared memory
        pookinator_memory.write_string("")

        match value:
            case "1":
                direction = MotorDirections.FORWARD
            case "2":
                direction = MotorDirections.RIGHT
            case "3":
                direction = MotorDirections.LEFT
            case "0":
                direction = MotorDirections.STOP
            case _:
                continue

        print(f"BCI selection: {direction.name}")

        driving.drive_one_unit(direction)


if __name__ == '__main__':
    main()
