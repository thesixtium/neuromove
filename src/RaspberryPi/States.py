from enum import Enum


class States(Enum):
    START = 1
    DESTINATION = 2

class DestinationDrivingStates(Enum):
    IDLE = 1
    MAP_ROOM = 2
    SELECT_DESTINATION = 3
    TRANSLATE_TO_MOVEMENT = 4
    DRIVE = 5
