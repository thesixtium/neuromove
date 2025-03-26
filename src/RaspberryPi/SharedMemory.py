#https://stackoverflow.com/questions/73719101/connecting-a-c-program-to-a-python-script-with-shared-memory

from multiprocessing import shared_memory
import numpy as np

from src.RaspberryPi.InternalException import DidNotCreateSharedMemory, NotEnoughSharedMemory
from src.RaspberryPi.States import MotorDirections
from src.RaspberryPi.States import States, DestinationDrivingStates

class SharedMemory:
    def __init__(self, shem_name: str, size:int, create=False):
        try:
            self.size = size
            self.memory = shared_memory.SharedMemory(name=shem_name, size=size, create=create)
        except:
            try:
                self.memory = shared_memory.SharedMemory(name=shem_name, size=size, create=False)
            except:
                raise DidNotCreateSharedMemory(shem_name)


    def _check_size(self, encoded: bytes):
        if len(encoded) >= self.size:
            raise NotEnoughSharedMemory(self.size, len(encoded) + 1)

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
            for values in value_split:
                print(values)
            
            value_split = [[int(k) for k in list(filter(lambda j: j != '', i.split(" ")))] for i in value_split]
            return np.array(value_split)
        else:
            return []

    def read_local_driving(self):
        value = self.read_string()
        match value:
            case "f":
                return MotorDirections.FORWARD
            case "b":
                return MotorDirections.BACKWARD
            case "l":
                return MotorDirections.LEFT
            case "r":
                return MotorDirections.RIGHT
            case _:
                return MotorDirections.STOP

    def read_destination_driving_state(self):
        value = self.read_string()
        match value:
            case "i":
                return DestinationDrivingStates.IDLE
            case "m":
                return DestinationDrivingStates.MAP_ROOM
            case "s":
                return DestinationDrivingStates.SELECT_DESTINATION
            case "t":
                return DestinationDrivingStates.TRANSLATE_TO_MOVEMENT
            case "d":
                return DestinationDrivingStates.DRIVE
            case _:
                return DestinationDrivingStates.IDLE

    def read_requested_next_state(self):
        value = self.read_string()
        match value:
            case "1":
                return States.START
            case "2":
                return States.SETUP
            case "3":
                return States.LOCAL
            case "4":
                return States.DESTINATION
            case "5":
                return States.RECOVERY
            case "6":
                return States.OFF
            case _:
                return None

    def close(self):
        try:
            self.memory.close()
            self.memory.unlink()
        except:
            pass

    def __del__(self):
        self.close()