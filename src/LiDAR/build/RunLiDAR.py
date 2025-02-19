import subprocess
import threading


class RunLiDAR:
    def __init__(self):
        self.serial_read_thread = threading.Thread(target=self.start)
        self.serial_read_thread.start()

    def start(self):
        subprocess.run(["dir && cmake .. && make -j2 && ../bin/aleks_lidar"])