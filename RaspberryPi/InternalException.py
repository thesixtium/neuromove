from enum import Enum

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

class UserError(InternalException):
    def __init__(self, message: str):
        super().__init__(10, ExceptionTypes.TEMPORARY, message)
