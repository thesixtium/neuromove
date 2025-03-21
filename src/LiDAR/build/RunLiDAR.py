import subprocess
import threading


class RunLiDAR:
    def __init__(self):
        self.serial_read_thread = threading.Thread(target=self.start)
        self.serial_read_thread.start()

    def start(self):
        print("cmake:")
        subprocess.run(["cmake src/LiDAR/build"], shell=True)
        print("make:")
        subprocess.run(["make src/LiDAR/bin -j2"], shell=True)
        print("run:")
        subprocess.run(["src/LiDAR/bin/aleks_lidar"], shell=True)
