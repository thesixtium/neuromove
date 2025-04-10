from pylsl import StreamInfo, StreamOutlet

import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR.replace(r"/Frontend", "")))

from src.RaspberryPi.SharedMemory import SharedMemory
from src.RaspberryPi.States import DestinationDrivingStates, SetupStates

from frontend_methods import *
from enums import *

def run_setup():
    with open("src/Frontend/frontend.css") as f:
        st.markdown("<style>{}</style>".format(f.read()), unsafe_allow_html=True)

    if "state" not in st.session_state:
        st.session_state["state"] = States.LOCAL

    if "local_driving_memory" not in st.session_state:
        st.session_state["local_driving_memory"] = SharedMemory(shem_name="local_driving", size=10, create=True)

    if "requested_next_state_memory" not in st.session_state:
        st.session_state["requested_next_state_memory"] = SharedMemory(shem_name="requested_next_state", size=10,
                                                                       create=True)
    if "marker_outlet" not in st.session_state:
        st.session_state["marker_info"] = StreamInfo(
            name='MarkerStream',
            type='LSL_Marker_Strings',
            channel_count=1,
            nominal_srate=250,
            channel_format='string',
            source_id='Marker_Outlet'
        )
        st.session_state["marker_outlet"] = StreamOutlet(st.session_state["marker_info"], 20, 360)

    if "bci_selection_memory" not in st.session_state:
        st.session_state["bci_selection_memory"] = SharedMemory(shem_name="bci_selection", size=10, create=True)

    if "flash_sequence" not in st.session_state:
        st.session_state["flash_sequence"] = []

    if "map_sequence" not in st.session_state:
        st.session_state["map_sequence"] = ["0"]

    if "training_target" not in st.session_state:
        st.session_state["training_target"] = -1

    if "currently_training" not in st.session_state:
        st.session_state["currently_training"] = False

    if "waiting_for_bci_response" not in st.session_state:
        st.session_state["waiting_for_bci_response"] = False

    if "setup_substate" not in st.session_state:
        st.session_state["setup_substate"] = SetupStates.SELECT_USER

    if "screen_position" not in st.session_state:
        st.session_state["screen_position"] = ScreenPosition.CENTRE

    if "eye_tracking_memory" not in st.session_state:
        st.session_state["eye_tracking_memory"] = SharedMemory(shem_name="eye_tracking", size=10, create=True)

    if "point_selection_memory" not in st.session_state:
        st.session_state["point_selection_memory"] = SharedMemory(shem_name="occupancy_grid", size=284622, create=True)

    if "running" not in st.session_state:
        st.session_state["running"] = False

    if "destination_driving_state" not in st.session_state:
        st.session_state["destination_driving_state"] = DestinationDrivingStates.IDLE

    if "occupancy_grid" not in st.session_state:
        st.session_state["occupancy_grid"] = None

    if "neighbourhood_grid" not in st.session_state:
        st.session_state["neighbourhood_grid"] = None
    
    if "origin" not in st.session_state:
        st.session_state["origin"] = None

    if "last_bci_selection" not in st.session_state:
        st.session_state["last_bci_selection"] = None
    
    if "bci_wait_start_time" not in st.session_state:
        st.session_state["bci_wait_start_time"] = None