import asyncio
import enum
import joblib
from sklearn.pipeline import Pipeline
from os.path import join

from src.RaspberryPi.BCI.output.shared_memory_messenger import SharedMemoryMessenger
from src.RaspberryPi.InternalException import BciSetupException

from lib.bci_essentials.bci_essentials.io.lsl_sources import LslEegSource, LslMarkerSource
from lib.bci_essentials.bci_essentials.io.messenger import Messenger
from lib.bci_essentials.bci_essentials.paradigm.p300_paradigm import P300Paradigm
from lib.bci_essentials.bci_essentials.io.xdf_sources import XdfEegSource, XdfMarkerSource
from lib.bci_essentials.bci_essentials.data_tank.data_tank import DataTank
from lib.bci_essentials.bci_essentials.bci_controller import BciController
from lib.bci_essentials.bci_essentials.classification.erp_rg_classifier import ErpRgClassifier

class Paradigm(enum.Enum):
    P300 = 1

class Bessy:
    '''
    Bessy is a wrapper class for bci_essentials. It is based on the class of the same name from [FlickTok](https://github.com/kirtonBCIlab/FlickTok/blob/main/src/apps/server/src/Bessy.py).

    Attributes
    ----------
    paradigm: Paradigm
        What BCI paradigm to use. Set to p300 for NeuroMove.
    pre_trained: bool
        Whether the model has been trained or not.
    online: bool
        True if training from headset data live. False if training from recorded data.
    messenger: Messenger
        Output interface for the data from BCI_Controller

    Methods
    --------
    run():
        Start processing EEG and marker events.

        If running offline, this happens once. Otherwise it runs continuously until told to stop
    set_stop():
        Tell the processing loop to stop execution
    save_model():
        Save the model to disk as a `pk1` file
    '''
    
    def __init__(self, online: bool = True, xdf_filepath: str = None, messenger: Messenger = None, model: Pipeline = None):
        '''
        Constructor for Bessy object. Initializes `BciController` and all associated classes.

        Parameters
        ------------
        online: bool
            True if training from headset data live. False if training from recorded data.
        xdf_filepath: str
            The path (relative or absolute) to the XDF file to use for offline processing. 

            Should only be set as a value other than `None` if `online` is `False`.
        messenger: Messenger
            How to recieve output of the class. Defaults to SharedMemory with text file debug option
        model: Pipeline
            Model to load into the controller on initialization.

        Returns
        -------
        none

        Raises
        -------
        BCISetupException
            If `online` is False and `xdf_filepath` is None. Offline processing requires a valid filepath

        '''

        # constant for NeuroMove but set for easy editing later
        self.__paradigm = Paradigm.P300

        self.__stop_event = asyncio.Event()
        self.__pre_trained = False

        match self.__paradigm:
            case Paradigm.P300:
                paradigm = P300Paradigm()
                classifier = ErpRgClassifier()
                classifier.set_p300_clf_settings()

                if model is not None:
                    self.__pre_trained = True
                    classifier.clf = model
            case _:
                raise BciSetupException(f"Paradigm  \"{self.__paradigm}\" not recognized")

        # default to shared memory without debug text file 
        if messenger is None:
            self.__messenger = SharedMemoryMessenger(debug=False)
        else:
            self.__messenger = messenger
    
        data_tank = DataTank()

        self.__online = online
        if online == False and xdf_filepath is None:
            raise BciSetupException(f"Offline processing selected but no XDF filepath provided")

        # if both onine = true and xdf filepath given, assume online processing
        eeg_source = None
        marker_source = None
        if online == True:
            eeg_source = LslEegSource()
            marker_source = LslMarkerSource()
        else:
            eeg_source = XdfEegSource(xdf_filepath)
            # TODO: Update this to XdfMarkerSource when using more recent data
            # marker_source = OldXdfFormatInput(xdf_filepath)
            marker_source = XdfMarkerSource(xdf_filepath)

        self.__bci_controller = BciController(
            eeg_source=eeg_source,
            marker_source=marker_source,
            paradigm=paradigm,
            classifier=classifier,
            data_tank=data_tank,
            messenger=self.__messenger
        )

        self.__bci_controller.setup(online=online, train_complete=self.__pre_trained)
        
        self.__bci_controller.event_timestamp_buffer = []
        self.__bci_controller.event_marker_buffer = []

    async def run(self):
        '''
        Start processing EEG and marker events.

        If running offline, this happens once. Otherwise it runs continuously until told to stop
        '''

        if self.__online:
            self.__task = asyncio.create_task(self.__bessy_step_loop())

        else:
            # just run step once for offline data
            self.__bci_controller.step()

    async def __bessy_step_loop(self):
        while not self.__stop_event.is_set():
            await self.__bessy_step()

            await asyncio.sleep(0.1)

    def set_stop(self):
        '''
        Tell the processing loop to stop execution
        '''
        if self.__task:
            self.__stop_event.set()
            self.__bci_controller = None

    async def __bessy_step(self):
        self.__bci_controller.step()

    def __del__(self):
        self.__stop_event.set()
        self.save_model("save_on_exit.pk1")

    # Aleks do your funky model compression stuff here please
    def save_model(self, save_name: str):
        '''
        Save the model to disk as a `pk1` file. Automatically called when the class is destroyed.
        '''
        
        save_path = join("models", save_name)

        # save the model
        joblib.dump(self.__bci_controller._classifier.clf, save_path)

# Aleks do your funky model decompression stuff here please
def load_and_return_model(filepath: str) -> Pipeline:
    '''
    Load a model from disk.

    Parameters
    ----------
    filepath: str
        The filepath (relative or absolute) of the `.pk1` file to load in
    
    Returns
    --------
    Pipeline
    '''
    
    return joblib.load(filepath)