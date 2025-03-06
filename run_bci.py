import asyncio
from src.RaspberryPi.BCI.output.shared_memory_messenger import SharedMemoryMessenger

from src.RaspberryPi.BCI.bci_essentials_wrapper import Bessy, load_and_return_model

    
async def main():
    # model = joblib.load("test_save.pk1")
    # model = None
    model = load_and_return_model("models/save_on_exit_DANI4.pk1")

    messenger = SharedMemoryMessenger(False)
    xdf_filepath ="c:/Users/danij/OneDrive/Documents/CurrentStudy/sub-LIAM/ses-S001/eeg/sub-LIAM_ses-S001_task-Default_run-001_eeg.xdf"

    bessy = Bessy(online=True, xdf_filepath=xdf_filepath, messenger=messenger, model=model)
    print("DONE CONSTRUCTOR")
        
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