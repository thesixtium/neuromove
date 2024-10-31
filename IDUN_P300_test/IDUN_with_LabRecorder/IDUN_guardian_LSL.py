import asyncio
from idun_guardian_sdk import GuardianClient
import tkinter as tk
from termcolor import colored

mac_address = "E5:1E:FD:F5:15:26"

def init_guardian_client() -> GuardianClient:
    '''
    Initlialize the IDUN Guardian Client. Connect to the headset using the API Key and the MAC Address of the headset.
    '''

    api_token = _get_api_key()

    client = GuardianClient(api_token=api_token, debug=True, address=mac_address)
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

async def main():
    client = init_guardian_client()

    await client.check_battery()
    
    input_task = asyncio.create_task(wait_for_input(client))

    try:
        await asyncio.gather(client.stream_impedance(handler=_print_impedance), input_task)
    except Exception as e:
        print(e)
        print("Error in impedance check")

    print("Done Impedance Check")

if __name__ == "__main__":
    asyncio.run(main())