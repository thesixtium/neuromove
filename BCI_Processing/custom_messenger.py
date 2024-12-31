from bci_essentials.io.messenger import Messenger
from bci_essentials.classification.generic_classifier import Prediction

class TextFileMessenger(Messenger):
    def __init__(self, file_path: str):
        self.file_path = file_path

        # open file in write mode
        self.file = open(file_path, "w")

    def __del__(self):
        self.file.close()

    def ping(self):
        self.file.write("ping\n")

    def marker_received(self, marker):
        self.file.write("marker received : {}\n".format(marker))

    def prediction(self, prediction: Prediction):
        self.file.write("{} with probabilities {}\n".format(prediction.labels, prediction.probabilities))