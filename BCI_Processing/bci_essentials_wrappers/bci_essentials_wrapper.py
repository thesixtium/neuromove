import asyncio

import custom_messenger

from .bci_input import BCIEssentialsInput
from bci_essentials.io.sources import EegSource, MarkerSource
from bci_essentials.paradigm.p300_paradigm import P300Paradigm
from bci_essentials.io.xdf_sources import XdfEegSource, XdfMarkerSource
from bci_essentials.data_tank.data_tank import DataTank
from bci_essentials.bci_controller import BciController
from bci_essentials.classification.erp_rg_classifier import ErpRgClassifier

# TODO: figure out if we need 2 instances of this for destination vs local driving mode (5 vs 4 classes) or if it can be modified on the fly
class Bessy:
    '''
    Bessy is a wrapper class for bci_essentials. It is based on the class of the same name from [FlickTok](https://github.com/kirtonBCIlab/FlickTok/blob/main/src/apps/server/src/Bessy.py).
    '''
    
    def __init__(self, num_classes: int):
        # variables constant for NeuroMove but set for easy editing later
        self.__paradigm = "p300"
        self.__flash_scheme = "s" # s for single item flashing at a time

        self.__num_classes = num_classes # number of options to choose from
        self.__stop_event = asyncio.Event()

        match self.__paradigm:
            case "p300":
                self.__paradigm = P300Paradigm()
                self.__classifier = ErpRgClassifier()
                self.__classifier.set_p300_clf_settings()
            case _:
                raise ValueError(f"Paradigm  \"{self.__paradigm}\" not recognized")
            
        self.__messenger = custom_messenger.TextFileMessenger("class_output.txt")
        self.__data_tank = DataTank()

        self.__bci_controller = None
        print("Constructor done")

    def setup_offline_processing(self, marker_source: XdfMarkerSource, eeg_source: XdfEegSource) -> None:
        self.__marker_source = marker_source
        self.__eeg_source = eeg_source

        if self.__bci_controller is not None:
            # TODO: implement elegant handling
            raise NotImplementedError("No graceful handling of stopping existing controller implemented")
        
        self.__bci_controller = BciController(
            eeg_source=self.__eeg_source,
            marker_source=self.__marker_source,
            paradigm=self.__paradigm,
            classifier=self.__classifier,
            data_tank=self.__data_tank,
            messenger=self.__messenger
        )

        self.__bci_controller.setup(online=False)

        self.__bci_controller.event_timestamp_buffer = []
        self.__bci_controller.event_marker_buffer = []

        self.__task = asyncio.create_task(self.__bessy_step_loop())

    # TODO: implement online processing

    # assuming we can change it on the fly    
    def set_num_classes(self, new_num_classes: int) -> None:
        '''
        Change the number of classes in the BCI system.
        '''
        # check if number of classes is valid
        if new_num_classes < 2:
            raise ValueError("Number of classes must be at least 2.")

        self.__num_classes = new_num_classes

    def start_training_session(self):
        self.__input.queue_marker("Trial Started")

    def end_training_session(self):
        self.__input.queue_marker("Trial Ends")

    def mark_trial(self, target: int, flashed: int):
        '''
        Emit a trial marker for bci_controller

        Args
        ----
        target: int
            the number of the target block for the trial
        flashed: int
            the number of the block that just flashed
        '''
        
        message = f"{self.__paradigm},{self.__flash_scheme},{self.__num_classes},{target},{flashed}"
        self.__input.queue_marker(message)

    def train_classifier(self):
        '''Tells Bessy to update classifier using data set'''
        self.__input.queue_marker("Update Classifier")
        
    def make_prediction(self, flashed: int):
        # label of -1 triggers prediction
        message = f"{self.__paradigm},{self.__flash_scheme},{self.__num_classes},-1,{flashed}"
        self.__input.queue_marker(message)

    async def __bessy_step_loop(self):
        while not self.__stop_event.is_set() and self.__bci_controller is not None:
            await self.__bessy_step()

    async def __bessy_step(self):
        self.__bci_controller.step()

    def __del__(self):
        self.__stop_event.set()