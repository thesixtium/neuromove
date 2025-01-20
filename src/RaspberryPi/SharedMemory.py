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

    def read_grid(self):
        value = self.read_string()
        if value:
            return [ [int(j) for j in i] for i in value.split("|")[:-1] ]
        else:
            return []

    def close(self):
        try:
            self.memory.close()
            self.memory.unlink()
        except:
            pass

    def __del__(self):
        self.close()