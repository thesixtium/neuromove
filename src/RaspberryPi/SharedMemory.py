#https://stackoverflow.com/questions/73719101/connecting-a-c-program-to-a-python-script-with-shared-memory

from multiprocessing import shared_memory
from src.RaspberryPi.InternalException import DidNotCreateSharedMemory, NotEnoughSharedMemory

class SharedMemory:
    def __init__(self, shem_name: str, size:int, create=False):
        try:
            self.size = size
            self.memory = shared_memory.SharedMemory(name=shem_name, size=size, create=create)
        except:
            raise DidNotCreateSharedMemory(shem_name)

    def _check_size(self, encoded: bytes):
        if len(encoded) >= self.size:
            raise NotEnoughSharedMemory(self.size, len(encoded) + 1)

    def write_string(self, string: str):
        encoded = string.encode()
        self._check_size(encoded)
        self.memory.buf[:len(encoded)] = encoded

    def read_string(self):
        return bytes(self.memory.buf).strip(b'\x00').decode()

    def write_grid(self, occupancy_grid: list):
        data = ''.join(str(item) for innerlist in occupancy_grid for item in innerlist).encode()

        width = len(occupancy_grid[0])
        height = len(occupancy_grid)
        specs = f"{width}x{height}:".encode()

        encoded = specs + data
        self._check_size(encoded)
        self.memory.buf[:len(encoded)] = encoded

    def read_grid(self):
        x = bytes(self.memory.buf).strip(b'\x00')
        specs, data = x.split(b":")
        width, height = specs.decode().split("x")
        grid = []

        for h in range(int(height)):
            start = h * int(width)
            end = (h + 1) * int(width)
            grid.append([int(i) for i in data[start:end].decode()])

        return grid

    def close(self):
        try:
            self.memory.close()
            self.memory.unlink()
        except:
            pass

    def __del__(self):
        self.close()