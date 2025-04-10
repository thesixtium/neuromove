from enum import Enum


class States(Enum):
    START = 1
    SETUP = 2
    LOCAL = 3
    DESTINATION = 4
    RECOVERY = 5
    OFF = 6

class DestinationDrivingStates(Enum):
    IDLE = 1
    MAP_ROOM = 2
    SELECT_DESTINATION = 3
    TRANSLATE_TO_MOVEMENT = 4
    DRIVE = 5

class MotorDirections(Enum):
    FORWARD = b"w"
    BACKWARD = b"s"
    LEFT = b"a"
    RIGHT = b"d"
    STOP = b"x"

class SetupStates(Enum):
    SELECT_USER = 1
    SELECT_POSITION = 2
    TRAIN = 3