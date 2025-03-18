from pylsl import StreamInfo, StreamOutlet

from src.RaspberryPi.SharedMemory import SharedMemory
from src.Frontend.frontend_methods import *
from src.Frontend.enums import *
from src.RaspberryPi.States import SetupStates

def run_setup():
    with open("Frontend/frontend.css") as f:
        st.markdown("<style>{}</style>".format(f.read()), unsafe_allow_html=True)

    if "state" not in st.session_state:
        st.session_state["state"] = States.SETUP

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
        st.session_state["point_selection_memory"] = SharedMemory(shem_name="point_selection", size=10, create=True)