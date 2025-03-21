import subprocess
import threading


class RunLiDAR:
    def __init__(self):
        self.serial_read_thread = threading.Thread(target=self.start)
        self.serial_read_thread.start()

    def start(self):
        subprocess.run(["dir"])
        subprocess.run(["cd src && dir"])
        subprocess.run(["cd src/LiDAR && dir"])
        subprocess.run(["cd src/LiDAR/build && dir"])
        subprocess.run(["cmake src/LiDAR/build"])
        subprocess.run(["make src/LiDAR/bin -j2"])
        subprocess.run(["src/LiDAR/bin/aleks_lidar"])
