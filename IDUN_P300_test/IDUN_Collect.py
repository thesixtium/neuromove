import asyncio
from pylsl import StreamInfo, StreamOutlet, local_clock

from idun_guardian_sdk import GuardianClient

RECORDING_TIMER: int = (60 * 13)  # = 60 seconds * n minutes
my_api_token = "XXXXXX"


async def stop_task(task):
    task.cancel()
    try:
        await task
    except asyncio.CancelledError:
        pass


async def read_battery_30(client):
    while True:
        battery = await client.check_battery()
        print("Battery Level: %s%%" % battery)
        await asyncio.sleep(30)


async def main():
    client = GuardianClient(api_token=my_api_token, debug=True)
    client.address = await client.search_device()

    info = StreamInfo("IDUN", "EEG", 1, 250, "float32", client.address)
    lsl_outlet = StreamOutlet(info, 20, 360)


    def lsl_stream_handler(event):
        message = event.message
        eeg = message["raw_eeg"]
        most_recent_ts = eeg[-1]["timestamp"]
        data = [sample["ch1"] for sample in eeg]
        lsl_outlet.push_chunk(data, most_recent_ts)

    client.subscribe_live_insights(
        raw_eeg=True,
        handler=lsl_stream_handler,
    )

    recording_task = asyncio.create_task(client.start_recording(recording_timer=RECORDING_TIMER))
    battery_task = asyncio.create_task(read_battery_30(client))
    await recording_task
    await stop_task(battery_task)


if __name__ == "__main__":
    asyncio.run(main())
