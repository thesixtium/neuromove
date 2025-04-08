import subprocess
import threading


class RunLiDAR:
    def __init__(self):
        self.serial_read_thread = threading.Thread(target=self.start)
        self.serial_read_thread.start()

    def start(self):
        print("L I D A R   T H I N G:  cmake:")
        subprocess.run(["cd ./src/LiDAR/build && cmake .. && make -j2"], shell=True)
        print("L I D A R   T H I N G:  run:")
        subprocess.run(["./src/LiDAR/bin/aleks_lidar"], shell=True)
