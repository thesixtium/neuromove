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