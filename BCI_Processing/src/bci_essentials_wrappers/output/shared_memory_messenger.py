from lib.bci_essentials.bci_essentials.io.messenger import Messenger

class SharedMemoryMessenger(Messenger):
    def __init__(self):
        super().__init__()

    def ping(self):
        return super().ping()
    
    def marker_received(self, marker):
        return super().marker_received(marker)
    
    def prediction(self, prediction):
        return super().prediction(prediction)