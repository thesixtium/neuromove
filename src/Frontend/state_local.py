from datetime import datetime, timedelta
from math import e
import threading
import time
from frontend_methods import *
import sys
import os
import numpy as np

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR.replace(r"/Frontend", "")))

from src.RaspberryPi.SharedMemory import SharedMemory

def state_local():
    print("LOCAL DRIVING")
    move_content()
    local_driving_grid()

    read_string = st.session_state['bci_selection_memory'].read_string()
    if len(read_string) > 0 and "[" in read_string:
        st.session_state["waiting_for_bci_response"] = False
        print(f"RECEIVED {read_string} FROM SHARED MEM")
        st.session_state['bci_selection_memory'].write_string("   ")
        read_string = read_string.strip()

        match read_string:
            case "[0]":
                direction_update("f")
            case "[1]":
                direction_update("l")
            case "[2]":
                direction_update("s")
            case "[3]":
                direction_update("r")
            case "[4]":
                switch()
            case _:
                print("Not confident enough to make a decision")

    if len(st.session_state["flash_sequence"]) > 0:
        st.session_state["flash_sequence"] = st.session_state["flash_sequence"][1:]
        time.sleep(0.1)
        st.rerun()
    elif st.session_state["waiting_for_bci_response"] == True:
        time.sleep(0.5)
        st.rerun()
    elif st.session_state["running"] == True and st.session_state["eye_tracking_memory"].read_string() == "[0]":
        time.sleep(0.1)
        st.rerun()
    elif st.session_state["waiting_for_bci_response"] == False and st.session_state["eye_tracking_memory"].read_string() == "[1]" and st.session_state["running"] == True and st.session_state["state"] == States.LOCAL:
        give_local_sequence_list()
        st.rerun()
    else:
        print("???")
    