import asyncio
from time import sleep
from os.path import join, dirname, exists

from src.RaspberryPi.InternalException import BciSetupException
from src.RaspberryPi.SharedMemory import SharedMemory
from src.RaspberryPi.BCI.output.shared_memory_messenger import SharedMemoryMessenger

from src.RaspberryPi.BCI.bci_essentials_wrapper import Bessy, load_and_return_model

    
def run_bci():
    messenger = SharedMemoryMessenger(debug=False)

    bessy = Bessy(messenger=messenger)
    print("DONE CONSTRUCTOR")

    # wait for result from shared memory
    bci_mem = SharedMemory(shem_name="bci_selection", size=20, create=False)
    name = bci_mem.read_string()
    while len(name) == 0:
        print("waiting for name....")
        sleep(0.5)
        name = bci_mem.read_string()

    if name != "N/A":
        model_file_name = "save_on_exit_" + name + ".pk1"
        model_path = join(dirname(__file__), "models", model_file_name)
        if exists(model_path):
            bessy.set_model(name)
        else:
            bessy.set_username(name)
    print("DONE SETTING MODEL")
        
    bessy.run()



if __name__ == "__main__":
    try:
        run_bci()
    except KeyboardInterrupt:
        print("Received Ctrl+C, shutting down gracefully.")

    # asyncio.run(online_main())
    # task = None
    # try:
    #     task = asyncio.create_task(main())
    # except KeyboardInterrupt:
    #     print("WHEEEEE")
    #     task.cancel()
    #     exit()
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