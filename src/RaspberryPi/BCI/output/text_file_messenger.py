from lib.bci_essentials.bci_essentials.io.messenger import Messenger

class TextFileMessenger(Messenger):
    def __init__(self, filepath:str, debug=False):
        self.__filepath =filepath
        self.debug = debug

        # clear file contents if they exist
        f = open(self.__filepath, "w")
        f.close()

    def __open_file(self):
        file = open(self.__filepath, "a")

        return file
    
    def ping(self):
        # disable pinging if in debug
        if not self.debug:
            return
        
        file = self.__open_file()
        file.write("ping\n")

    def marker_received(self, marker):
        # disable marker notifications if in debug
        if not self.debug:
            return
        
        file = self.__open_file()
        file.write(f"Processed marker: {marker}\n")

    def prediction(self, prediction):
        # always send predictions
        file = self.__open_file()
        file.write(f"Predicted {prediction.labels} with confidence {prediction.probabilities}\n")