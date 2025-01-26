import asyncio

from .input.xdf_input import OldXdfFormatInput
from lib.bci_essentials.bci_essentials.io.sources import EegSource, MarkerSource
from src.bci_essentials_wrappers.output.text_file_messenger import TextFileMessenger

from lib.bci_essentials.bci_essentials.io.messenger import Messenger
from lib.bci_essentials.bci_essentials.paradigm.p300_paradigm import P300Paradigm
from lib.bci_essentials.bci_essentials.io.xdf_sources import XdfEegSource, XdfMarkerSource
from lib.bci_essentials.bci_essentials.data_tank.data_tank import DataTank
from lib.bci_essentials.bci_essentials.bci_controller import BciController
from lib.bci_essentials.bci_essentials.classification.erp_rg_classifier import ErpRgClassifier

# TODO: figure out if we need 2 instances of this for destination vs local driving mode (5 vs 4 classes) or if it can be modified on the fly
class Bessy:
    '''
    Bessy is a wrapper class for bci_essentials. It is based on the class of the same name from [FlickTok](https://github.com/kirtonBCIlab/FlickTok/blob/main/src/apps/server/src/Bessy.py).
    '''
    
    def __init__(self, num_classes: int, messenger: Messenger = None):
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
            
        if messenger is None:
            self.__messenger = TextFileMessenger("data/class_output.txt")
        else:
            self.__messenger = messenger
    
        self.__data_tank = DataTank()

        self.__bci_controller = None

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

        # just run step once for XDF data
        self.__bci_controller.step()

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

    async def __bessy_step_loop(self):
        while not self.__stop_event.is_set() and self.__bci_controller is not None:
            await self.__bessy_step()

    async def __bessy_step(self):
        self.__bci_controller.step()

    def __del__(self):
        self.__stop_event.set()