import enum
import threading
from time import sleep
import joblib
from sklearn.pipeline import Pipeline
from os.path import join, dirname

from src.RaspberryPi.BCI.input.xdf_input import OldXdfFormatInput
from src.RaspberryPi.BCI.output.shared_memory_messenger import SharedMemoryMessenger
from src.RaspberryPi.InternalException import BciSetupException, BessyFailedException

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
    
    def __init__(self, messenger: Messenger = None, model: Pipeline = None, confidence: float = 0.7):
        '''
        Constructor for Bessy object. Initializes `BciController` and all associated classes.

        Parameters
        ------------
        messenger: Messenger
            How to recieve output of the class. Defaults to SharedMemory with text file debug option
        model: Pipeline
            Model to load into the controller on initialization.

        Returns
        -------
        none

        '''

        # constant for NeuroMove but set for easy editing later
        self.__paradigm_type = Paradigm.P300

        self.__stop_event = threading.Event()
        self.__pre_trained = False
        self.__exception = None
        self.__user_name = None

        match self.__paradigm_type:
            case Paradigm.P300:
                self.__paradigm = P300Paradigm()
                self.__classifier = ErpRgClassifier()
                self.__classifier.set_p300_clf_settings()

                if model is not None:
                    self.__pre_trained = True
                    self.__classifier.clf = model
            case _:
                raise BciSetupException(f"Paradigm  \"{self.__paradigm_type}\" not recognized")

        # default to shared memory without debug text file 
        if messenger is None:
            self.__messenger = SharedMemoryMessenger(debug=False, confidence=confidence)
        else:
            self.__messenger = messenger
    
        self.__data_tank = DataTank()

        self.__eeg_source = LslEegSource()
        self.__marker_source = LslMarkerSource()

        self.__bci_controller = self.__init_bessy()

    def __init_bessy(self):
        bci_controller = BciController(
            eeg_source=self.__eeg_source,
            marker_source=self.__marker_source,
            paradigm=self.__paradigm,
            classifier=self.__classifier,
            data_tank=self.__data_tank,
            messenger=self.__messenger
        )

        bci_controller.setup(online=True, train_complete=self.__pre_trained)
        
        bci_controller.event_timestamp_buffer = []
        bci_controller.event_marker_buffer = []

        return bci_controller

    def set_confidence(self, confidence: float):
        self.__messenger.set_confidence(new_confidence=confidence)

    def set_model(self, model_name: str = None):
        # TODO: make sure this can't be called once processing starts somehow??

        if model_name is not None:
            self.__user_name = model_name
        model_path = join("models", f"save_on_exit_{self.__user_name}.pk1")
        
        try:
            model = load_and_return_model(model_path)
        except FileNotFoundError:
            raise BciSetupException("Model does not exist")

        # re-init bessy object
        self.__pre_trained = True
        self.__classifier.clf = model

        self.__bci_controller = self.__init_bessy()

    def set_username(self, user_name: str):
        self.__user_name = user_name

    def run(self):
        '''
        Start processing EEG and marker events.

        Runs continuously until told to stop
        '''

        self.__thread = threading.Thread(target=self.__bessy_step_loop)
        self.__thread.start()

        while self.__thread.is_alive():
            if self.__exception:
                print("RAISING EXCEPTION")
                raise self.__exception
            
            sleep(0.1)
            


    def __bessy_step_loop(self):
        while not self.__stop_event.is_set():
            try:
                self.__bci_controller.step()
            except Exception as e:
                self.__shutdown()
                self.__exception = BessyFailedException(f"Error in Bessy processing loop: {e}")

            sleep(0.1)


    def set_stop(self):
        '''
        Tell the processing loop to stop execution
        '''
        if self.__thread:
            self.__stop_event.set()
            self.__bci_controller = None
            self.__thread.join()

    def __shutdown(self):
        self.__stop_event.set()
        self.save_model("save_on_exit_" + self.__user_name + ".pk1")

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