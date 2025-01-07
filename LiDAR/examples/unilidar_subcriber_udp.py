#https://stackoverflow.com/questions/73719101/connecting-a-c-program-to-a-python-script-with-shared-memory

from multiprocessing import shared_memory

from enum import Enum

from matplotlib import pyplot as plt

class ExceptionTypes(Enum):
    PERMANENT = "P"
    TEMPORARY = "T"

class InternalException(Exception):
    def __init__(self, exception_id: int, exception_type: ExceptionTypes, message: str):
        self.exception_id = exception_id
        self.exception_type = exception_type
        self.message = message

    def print(self):
        return f"ID{self.exception_id}{self.exception_type.value}: {self.message}"

    def is_permanent(self):
        return self.exception_type == ExceptionTypes.PERMANENT

    def get_exception_id(self):
        return self.exception_id


class NotImplementedYet(InternalException):
    def __init__(self, function_name: str):
        super().__init__(1, ExceptionTypes.PERMANENT, f"'{function_name}' is not implemented")


class UnknownFSMState(InternalException):
    def __init__(self):
        super().__init__(2, ExceptionTypes.TEMPORARY, f"FSM entered unknown state")


class EnteredRecoveryModeWithoutException(InternalException):
    def __init__(self):
        super().__init__(3, ExceptionTypes.TEMPORARY, f"Entered recovery mode without an exception")


class EnteredOffState(InternalException):
    def __init__(self):
        super().__init__(4, ExceptionTypes.PERMANENT, f"Entered off state")

class SensorDistanceAlert(InternalException):
    def __init__(self, sensor_name):
        super().__init__(5, ExceptionTypes.TEMPORARY, f"Sensor {sensor_name} alerted")

class CouldNotOpenPort(InternalException):
    def __init__(self, port_name):
        super().__init__(6, ExceptionTypes.PERMANENT, f"Could not open port {port_name}")

class InvalidSocketExpectedType(InternalException):
    def __init__(self, expected_type):
        super().__init__(7, ExceptionTypes.PERMANENT, f"Socket class can't handle type {str(expected_type)}")

class CantLoadSocketJSON(InternalException):
    def __init__(self, message):
        super().__init__(8, ExceptionTypes.TEMPORARY, f"Socket class can't decode JSON input: {message}")

class CantConvertSocketData(InternalException):
    def __init__(self, message, expected_type):
        super().__init__(9, ExceptionTypes.TEMPORARY, f"Socket class can't turn {message} into a {str(expected_type)}")

class UserError(InternalException):
    def __init__(self, message: str):
        super().__init__(10, ExceptionTypes.TEMPORARY, message)

class DidNotCreateSharedMemory(InternalException):
    def __init__(self, memory: str):
        super().__init__(11, ExceptionTypes.PERMANENT, f"Didn't create shared memory for {memory}")

class NotEnoughSharedMemory(InternalException):
    def __init__(self, allocated: int, required: int):
        super().__init__(12, ExceptionTypes.PERMANENT, f"Operation requires {required} bytes, only allocated {allocated} bytes")


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
sm = SharedMemory("grid3", 284622, create=False)

while True:
    value = sm.read_string()
    
    if value:
        print(value.split("|")[:-1])
        data = {'x': [], 'y': [], 'z': []}
        grid = [ [int(j) for j in i] for i in value.split("|")[:-1] ]
    
        with open('testData', 'w') as f:
            f.write(str(grid))
        for x in range(len(grid)):
            for y in range(len(grid[0])):
                data['x'].append(x)
                data['y'].append(y)
                data['z'].append(grid[x][y])
        
        fig = plt.figure(figsize=(6,6))
        ax = fig.add_subplot()
        ax.scatter(data['x'], data['y'], data['z'])
        ax.scatter([len(grid)//2], [len(grid)//2], [1])
        plt.show()
        input()


