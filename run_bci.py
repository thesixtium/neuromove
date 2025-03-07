import asyncio
from time import sleep
from os.path import join, dirname

from src.RaspberryPi.InternalException import BciSetupException
from src.RaspberryPi.SharedMemory import SharedMemory
from src.RaspberryPi.BCI.output.shared_memory_messenger import SharedMemoryMessenger

from src.RaspberryPi.BCI.bci_essentials_wrapper import Bessy, load_and_return_model

    
async def main():
    messenger = SharedMemoryMessenger(False)

    print("DONE CONSTRUCTOR")
    bessy = Bessy(messenger=messenger)

    # wait for result from shared memory
    bci_mem = SharedMemory(shem_name="bci_selection", size=20, create=False)
    name = bci_mem.read_string()
    while len(name) == 0:
        print("waiting for name....")
        sleep(0.5)
        name = bci_mem.read_string()

    model = None
    if name != "N/A":
        model_file_name = "save_on_exit_" + name + ".pk1"
        model_path = join(dirname(__file__), "models", model_file_name)

        try:
            model = load_and_return_model(model_path)
        except FileNotFoundError:
            raise BciSetupException("Model does not exist")

    bessy.set_model(model)
    print("DONE SETTING MODEL")
        
    await bessy.run()

async def run_main():
    stop_event = asyncio.Event()

    async def wait_for_exit():
        try:
            while not stop_event.is_set():
                await asyncio.sleep(0.5)
        except asyncio.CancelledError:
            pass

    task = asyncio.create_task(main())
    stop_task = asyncio.create_task(wait_for_exit())

    try:
        await stop_task  # Wait for Ctrl+C
    except asyncio.CancelledError:
        pass

    print("Stopping main task...")
    task.cancel()

    try:
        await task
    except asyncio.CancelledError:
        pass  # Cleanup already handled

if __name__ == "__main__":
    try:
        asyncio.run(run_main())
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