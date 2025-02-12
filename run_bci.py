import asyncio

import joblib

from src.RaspberryPi.BCI.output.text_file_messenger import TextFileMessenger
from lib.bci_essentials.bci_essentials.io.lsl_sources import LslEegSource, LslMarkerSource
from lib.bci_essentials.bci_essentials.io.xdf_sources import XdfEegSource, XdfMarkerSource

from src.RaspberryPi.BCI.bci_essentials_wrapper import Bessy
from src.RaspberryPi.BCI.input.xdf_input import OldXdfFormatInput
    
async def main():
    # model = joblib.load("test_save.pk1")
    # model = None

    messenger = TextFileMessenger("BCI_Processing/data/output.txt")
    xdf_filepath ="C:/Users/danij/OneDrive/Documents/CurrentStudy/sub-DEBUG/ses-S001/eeg/sub-DEBUG_ses-S001_task-Default_run-003_eeg.xdf"
    # xdf_filepath = "C:/Users/danij/OneDrive/Documents/CurrentStudy/sub-DANI/ses-S001/eeg/sub-DANI_ses-S001_task-Default_run-001_eeg.xdf"

    bessy = Bessy(online=False, xdf_filepath=xdf_filepath, messenger=messenger)
        
    try:
        await bessy.run()
    except KeyboardInterrupt:
        bessy.set_stop()
    except Exception as e:
        raise e

    # input("press enter to continue")

    # bessy.setup_offline_processing(marker_source, eeg_source)

async def online_main():

    bessy = Bessy()
    input("constructor done. press enter to continue")
    
    try:
        bessy.run()
    except KeyboardInterrupt:
        bessy.set_stop()

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