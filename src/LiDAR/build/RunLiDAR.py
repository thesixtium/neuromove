import subprocess
import threading
import os
import sys

class RunLiDAR:
    def __init__(self):
        self.serial_read_thread = threading.Thread(target=self.start)
        self.serial_read_thread.start()

    def start(self):
#        subprocess.run(["cd -P ~/Documents/neuromove/src/LiDAR/build && cmake .. && make -j2 && ../bin/aleks_lidar"], shell=True)
        subprocess.run(["cd -P ~/Documents/neuromove/src/LiDAR/build && ../bin/aleks_lidar"], shell=True)
