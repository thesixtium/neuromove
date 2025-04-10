from lib.bci_essentials.bci_essentials.io.messenger import Messenger

from src.RaspberryPi.SharedMemory import SharedMemory

class SharedMemoryMessenger(Messenger):
    def __init__(self, confidence: float =  0.7, debug: bool=False):
        super().__init__()

        self.__write_to_text = debug
        self.__confidence = confidence
        self.__shared_memory = SharedMemory("bci_selection", size=20, create=False)

        if self.__write_to_text:
            self.__filepath = "BCI_OUTPUT_DEBUG.txt"
            open(self.__filepath, "w")

    def __open_file(self):
        file = open(self.__filepath, "a")

        return file

    def ping(self):
        if self.__write_to_text:
            file = self.__open_file()
            file.write("ping\n")
    
    def marker_received(self, marker):
        if self.__write_to_text:
            file = self.__open_file()
            file.write(f"processed marker: {marker}\n")

    def set_confidence(self, new_confidence: float):
        if new_confidence <= 0:
            return
        
        self.__confidence = new_confidence
    
    def prediction(self, prediction):
        if self.__write_to_text:
            file = self.__open_file()
            file.write(f"Predicted {prediction.labels} with confidence {prediction.probabilities}\n")

        if prediction.probabilities[0] > self.__confidence:
            # write to shared memory
            self.__shared_memory.write_string(f"{prediction.labels}    ")
        else:
            # write empty string to show no prediciton made
            self.__shared_memory.write_string("[]   ")
        # self.__shared_memory.write_string(f"Prediction: {prediction.labels} w/ {prediction.probabilities}")