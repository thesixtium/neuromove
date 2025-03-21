import subprocess
import threading


class RunLiDAR:
    def __init__(self):
        self.serial_read_thread = threading.Thread(target=self.start)
        self.serial_read_thread.start()

    def start(self):
        subprocess.run(["dir"], shell=True)
        subprocess.run(["cd src && dir"], shell=True)
        subprocess.run(["cd src/LiDAR && dir"], shell=True)
        subprocess.run(["cd src/LiDAR/build && dir"], shell=True)
        subprocess.run(["cmake src/LiDAR/build"], shell=True)
        subprocess.run(["make src/LiDAR/bin -j2"], shell=True)
        subprocess.run(["src/LiDAR/bin/aleks_lidar"], shell=True)
