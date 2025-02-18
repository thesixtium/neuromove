import threading
from streamlit.web.bootstrap import run
import os
import asyncio
import webbrowser
import subprocess
from subprocess import run

import threading
import sys
from streamlit.web import cli as stcli

class RunUI:
    def __init__(self):
        self.ui_thread = threading.Thread(target=self.start, daemon=True)
        self.ui_thread.start()

    def start(self):
        process = run(["streamlit", "run", r"Frontend/local.py"])