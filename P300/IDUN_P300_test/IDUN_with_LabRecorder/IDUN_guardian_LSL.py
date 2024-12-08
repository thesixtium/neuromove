import asyncio
from idun_guardian_sdk import GuardianClient
from termcolor import colored
from pylsl import StreamInfo, StreamOutlet

mac_address = "E5:1E:FD:F5:15:26"
RECORDING_TIMER = (60 * 5)  # = 60 seconds * n minutes

async def init_guardian_client() -> GuardianClient:
    '''
    Initlialize the IDUN Guardian Client. Connect to the headset using the API Key and the MAC Address of the headset.
    '''

    api_token = _get_api_key()

    client = GuardianClient(api_token=api_token, debug=True, address=mac_address)
    client.address = await client.search_device()
    print(colored(f"Connected to {client.address}", 'cyan'))


    return client

def _get_api_key() -> str:
    '''
    Read API Key from api_key.txt.

    Prevents API Key from being hardcoded in the script.
    '''
    
    try:
        with open('api_key.txt') as f:
            return f.readline().strip()
        
    except FileNotFoundError:
        print("api_key.txt not found. Please create a file with your API key in the same directory as this script.")
        exit(1)


def _print_impedance(data):
    data = data/1000
    data_str = f"{data} kOhms"
    if data > 300:
        print(colored(data_str, "red"), end="            \r")
    else:
        print(colored(data_str, "green"), end="            \r")

async def wait_for_input(client: GuardianClient):
    print("Press any key to continue...")
    await asyncio.get_event_loop().run_in_executor(None, input)
    await client.stop_impedance()

async def check_impedance(client: GuardianClient):
    input_task = asyncio.create_task(wait_for_input(client))

    try:
        await asyncio.gather(client.stream_impedance(handler=_print_impedance), input_task)
    except Exception as e:
        print(e)
        print("Error in impedance check")

async def start_recording(client: GuardianClient):
    print(colored(f"client address: {client.address}", 'cyan'))
    stream_info = StreamInfo("IDUN", "EEG", 1, 250, "float32", client.address)
    lsl_outlet = StreamOutlet(stream_info, 20, 360)

    def lsl_stream_handler(event):
        message = event.message
        eeg = message["raw_eeg"]
        most_recent_ts = eeg[-1]["timestamp"]
        data = [sample["ch1"] for sample in eeg]
        lsl_outlet.push_chunk(data, most_recent_ts)

    # get the raw eeg data from the headset
    client.subscribe_live_insights(
        raw_eeg=True,
        handler=lsl_stream_handler,
    )

    await client.start_recording(recording_timer=RECORDING_TIMER) 

async def main():
    client = await init_guardian_client()

    await client.check_battery()
    
    await check_impedance(client)

    # await start_recording(client)

    print("Done Impedance Check")

if __name__ == "__main__":
    asyncio.run(main())