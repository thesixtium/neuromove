#https://stackoverflow.com/questions/73719101/connecting-a-c-program-to-a-python-script-with-shared-memory

from multiprocessing import shared_memory
import numpy as np

from src.RaspberryPi.States import MotorDirections

class SharedMemory:
    def __init__(self, shem_name: str, size:int, create=False):
        try:
            self.size = size
            self.memory = shared_memory.SharedMemory(name=shem_name, size=size, create=create)
        except:
            try:
                self.memory = shared_memory.SharedMemory(name=shem_name, size=size, create=False)
            except:
                print("SM erro 1`")


    def _check_size(self, encoded: bytes):
        if len(encoded) >= self.size:
            raise print("SM erro 12")

    def write_string(self, string: str):
        encoded = string.encode()
        self._check_size(encoded)
        self.memory.buf[:len(encoded)] = encoded

    def write_enum(self, enum):
        encoded = enum.value.encode()
        self._check_size(encoded)
        self.memory.buf[:len(encoded)] = encoded

    def write_np_array(self, array):
        encoded = str(array).encode()
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

    def read_np_array(self):
        value = self.read_string()

        if value:
            value = value.replace("[", "").replace("]", "")
            value_split = value.split("|")
            data = []
            for values in value_split:
                new_line = []
                for number in values:
                    new_line.append(int(number))
                if len(new_line) != 0:
                    data.append(new_line)
            return np.array(data)
        else:
            return []

    def read_local_driving(self):
        value = self.read_string()
        print(value)
        match value:
            case "1":
                return MotorDirections.FORWARD
            case "3":
                return MotorDirections.LEFT
            case "2":
                return MotorDirections.RIGHT
            case _:
                return MotorDirections.STOP

    def close(self):
        try:
            self.memory.close()
            self.memory.unlink()
        except:
            pass

    def __del__(self):
        self.close()
