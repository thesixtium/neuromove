import threading
from time import sleep
from streamlit.web.bootstrap import run
import os
import asyncio
import webbrowser
import subprocess
from subprocess import Popen, run

import threading
import sys
from streamlit.web import cli as stcli

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(os.path.join("src", SCRIPT_DIR)))

class RunUI:
    def __init__(self):
        self.ui_thread = threading.Thread(target=self.start, daemon=True)
        self.ui_thread.start()

    def start(self):
        frontend_path = os.path.join(os.path.dirname(__file__), "frontend.py")
        frontend_dir = os.path.dirname(frontend_path)
        self.process = Popen(["streamlit", "run", frontend_path], cwd=os.path.join(frontend_dir, ".."))

    def close(self):
        self.process.terminate()
        self.process.wait()
        self.ui_thread.join()

if __name__ == "__main__": 
    RunUI()

    while True:
        sleep(0.1)