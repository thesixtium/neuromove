import asyncio

import joblib

from src.bci_essentials_wrappers.output.text_file_messenger import TextFileMessenger
from lib.bci_essentials.bci_essentials.io.lsl_sources import LslEegSource, LslMarkerSource
from lib.bci_essentials.bci_essentials.io.xdf_sources import XdfEegSource, XdfMarkerSource

from src.bci_essentials_wrappers.bci_essentials_wrapper import Bessy
from src.bci_essentials_wrappers.input.xdf_input import OldXdfFormatInput


"""def test_bessy():
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

    #initialize these for bessy
    controller.event_marker_buffer = []
    controller.event_timestamp_buffer = []

    while True:
        controller.step()

    # run controller
    # controller.run(max_loops=100)
"""
    
# async def main():
#     eeg_source = XdfEegSource("data/sub-DANI_ses-S001_task-Default_run-001_eeg.xdf")
#     # marker_source = bci_essentials.io.xdf_sources.XdfMarkerSource("sub-DANI_ses-S001_task-Default_run-001_eeg.xdf")
#     marker_source = OldXdfFormatInput("data/sub-DANI_ses-s001_task-Default_run-001_eeg.xdf")

#     # model = joblib.load("test_save.pk1")

#     bessy = Bessy(9, model=None)

#     input("press enter to continue")

#     bessy.setup_offline_processing(marker_source, eeg_source)

async def main():
    eeg_source = XdfEegSource("data/sub-DANI_ses-S001_task-Default_run-001_eeg.xdf")
    # eeg_source = XdfEegSource("C:/Users/danij/OneDrive/Documents/CurrentStudy/sub-HEADSET/ses-S002/eeg/sub-HEADSET_ses-S002_task-Default_run-004_eeg.xdf")
    # marker_source = bci_essentials.io.xdf_sources.XdfMarkerSource("sub-DANI_ses-S001_task-Default_run-001_eeg.xdf")
    marker_source = OldXdfFormatInput("data/sub-DANI_ses-s001_task-Default_run-001_eeg.xdf")
    # marker_source = XdfMarkerSource("C:/Users/danij/OneDrive/Documents/CurrentStudy/sub-HEADSET/ses-S002/eeg/sub-HEADSET_ses-S002_task-Default_run-004_eeg.xdf")

    # model = joblib.load("test_save.pk1")
    # model = None

    messenger = TextFileMessenger("data/output.txt")

    bessy = Bessy(5, online=False, xdf_filepath="data/sub-DANI_ses-s001_task-Default_run-001_eeg.xdf", messenger=messenger)
    bessy.run()

    # input("press enter to continue")

    # bessy.setup_offline_processing(marker_source, eeg_source)

async def online_main():
    eeg_source = LslEegSource()
    print("RESOLVED EEG")
    marker_source = LslMarkerSource()
    print("Streams resolved(?)")

    bessy = Bessy(5)
    input("constructor done. press enter to continue")
    bessy.setup_online_processing(marker_source, eeg_source)

if __name__ == "__main__":
    # asyncio.run(online_main())
    asyncio.run(main())
    # test_bessy()

    # markersource = BessyInput("data/sub-DANI_ses-s001_task-Default_run-001_eeg.xdf")
    # data = markersource.get_markers()

    # import csv
    # list_of_lists, single_list = data

    # # Prepare the CSV data
    # rows = [[single_list[i], ", ".join(map(str, list_of_lists[i]))] for i in range(len(single_list))]

    # # Save to CSV
    # with open("data/output.csv", "w", newline="") as csvfile:
    #     writer = csv.writer(csvfile)
    #     writer.writerow(["Column 1", "Column 2"])  # Header row
    #     writer.writerows(rows)

    # print("Data saved to output.csv")