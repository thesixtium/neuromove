import asyncio
import bci_essentials
import bci_essentials.data_tank.data_tank
import bci_essentials.io.lsl_messenger
import numpy as np

import bci_essentials.classification.erp_rg_classifier
import bci_essentials.io.xdf_sources
import bci_essentials.paradigm.p300_paradigm
import bci_essentials.bci_controller

import bci_essentials.data_tank

from bci_essentials_wrappers.bci_essentials_wrapper import Bessy
import custom_messenger

def test_bessy():
    #create classifier
    classifier = bci_essentials.classification.erp_rg_classifier.ErpRgClassifier()
    classifier.set_p300_clf_settings()

    eeg_source = bci_essentials.io.xdf_sources.XdfEegSource("p300_example.xdf")
    marker_source = bci_essentials.io.xdf_sources.XdfMarkerSource("p300_example.xdf")
    
    paradigm = bci_essentials.paradigm.p300_paradigm.P300Paradigm()

    data_tank = bci_essentials.data_tank.data_tank.DataTank()

    # messenger = bci_essentials.io.lsl_messenger.LslMessenger()
    messenger = custom_messenger.TextFileMessenger("output.txt")

    # create controller
    controller = bci_essentials.bci_controller.BciController(
        eeg_source=eeg_source,
        marker_source=marker_source,
        paradigm=paradigm,
        classifier=classifier,
        data_tank=data_tank,
        messenger=messenger
    )

    # setup controller
    controller.setup(online=False)

    input("Press enter to start the controller")

    # run controller
    controller.run(max_loops=100)

async def main():
    eeg_source = bci_essentials.io.xdf_sources.XdfEegSource("C:/Users/danij/OneDrive/Documents/CurrentStudy/sub-DANI/ses-S001/eeg/sub-DANI_ses-S001_task-Default_run-001_eeg.xdf")
    marker_source = bci_essentials.io.xdf_sources.XdfMarkerSource("C:/Users/danij/OneDrive/Documents/CurrentStudy/sub-DANI/ses-S001/eeg/sub-DANI_ses-S001_task-Default_run-001_eeg.xdf")

    bessy = Bessy(9)

    input("press enter to continue")

    bessy.setup_offline_processing(marker_source, eeg_source)

if __name__ == "__main__":
    # main()
    asyncio.run(main())
