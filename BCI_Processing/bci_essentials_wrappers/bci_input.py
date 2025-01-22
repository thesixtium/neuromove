from bci_essentials.io.sources import MarkerSource
from pylsl import local_clock

#TODO: implement logger
class BCIEssentialsInput(MarkerSource):
    '''
    This class is used to feed markers into bci_essentials. It is a wrapper around MarkerSource to allow for easy
    integration with existing data collected.
    '''

    def __init__(self, logger = None):
        self.__logger = logger
        self.__messages = []
        self.__timestamps = []
    
    name = "BCIEssentialsInput"    # implements MarkerSource.name

    # implements MarkerSource.queue_markers()
    def queue_marker(self, message):
        '''
        Add a message to the queue for Bessy to read.
        '''
        timestamp = local_clock()
        self.__timestamps.append(timestamp)
        self.__messages.append(message)

    # implements MarkerSource.get_markers()
    def get_markers(self) -> tuple[list[list], list]:
        '''
        Get the markers and timestamps since the last call.
        '''
        timestamps = self.__timestamps
        messages = self.__messages

        markers = (messages, timestamps) 
    
        self.__reset_queue()

        return markers

    # implements MarkerSource.time_correction()
    def time_correction(self) -> float:
        '''
        Get the current time correction for timestamps.
        '''
        return 0
    
    def __reset_queue(self):
        self.__messages = []
        self.__timestamps = []
    