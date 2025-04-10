from datetime import datetime, timedelta, time
from math import e
import threading
import time
from frontend_methods import *
import sys
import os
import numpy as np

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR.replace(r"/Frontend", "")))

def state_local():
    move_content()
    local_driving_grid()

    display_string = "No previous selection"
    if st.session_state["last_bci_selection"] is not None:
        selection = st.session_state["last_bci_selection"]
        display_string = f"Read {selection} from BCI controller"

    st.text(display_string)
 
    read_string = st.session_state['bci_selection_memory'].read_string()
    if len(read_string) > 0 and "[" in read_string:
        st.session_state["waiting_for_bci_response"] = False
        print(f"RECEIVED {read_string} FROM SHARED MEM")
        st.session_state['bci_selection_memory'].write_string("   ")
        read_string = read_string.strip()
        st.session_state['last_bci_selection'] = read_string

        # match read_string:
        #     case "[0]":
        #         direction_update("f")
        #     case "[1]":
        #         direction_update("l")
        #     case "[2]":
        #         direction_update("s")
        #     case "[3]":
        #         direction_update("r")
        #     case "[4]":
        #         switch()
        #     case _:
        #         print("Not confident enough to make a decision")

    if len(st.session_state["flash_sequence"]) > 0:
        st.session_state["flash_sequence"] = st.session_state["flash_sequence"][1:]
        time.sleep(0.1)
        st.rerun()
    elif st.session_state["waiting_for_bci_response"] == True:
        if  datetime.now() - st.session_state["bci_wait_start_time"] > timedelta(seconds=30):
            print("Waiting for BCI controller to respond timed out")
            st.session_state["waiting_for_bci_response"] = False
        time.sleep(0.5)
        st.rerun()
    elif st.session_state["waiting_for_bci_response"] == False and st.session_state["running"] == True:
        give_local_sequence_list()
        st.rerun()
    