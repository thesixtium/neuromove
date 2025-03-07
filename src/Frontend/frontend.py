import time
import numpy as np
from io import BytesIO
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap

from streamlit_extras.stylable_container import stylable_container

from pylsl import StreamInfo, StreamOutlet

from src.RaspberryPi.SharedMemory import SharedMemory
from src.Frontend.style import *
from src.Frontend.frontend_methods import *

with open("Frontend/frontend.css") as f:
    st.markdown("<style>{}</style>".format(f.read()), unsafe_allow_html=True)

if "state" not in st.session_state:
    st.session_state["state"] = States.SETUP
if "local_driving_memory" not in st.session_state:
    st.session_state["local_driving_memory"] = SharedMemory(shem_name="local_driving", size=10, create=False)
if "requested_next_state_memory" not in st.session_state:
    st.session_state["requested_next_state_memory"] = SharedMemory(shem_name="requested_next_state", size=10, create=False)
if "marker_outlet" not in st.session_state:
    marker_info = StreamInfo(name='MarkerStream', type='LSL_Marker_Strings', channel_count=1, nominal_srate=250,
                             channel_format='string', source_id='Marker_Outlet')
    st.session_state["marker_outlet"] = StreamOutlet(marker_info, 20, 360)
if "bci_selection_memory" not in st.session_state:
    st.session_state["bci_selection_memory"] = SharedMemory(shem_name="bci_selection", size=10, create=False)
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

print(st.session_state["state"])

match st.session_state["state"]:
    case States.SETUP:
        match st.session_state["setup_substate"]:
            case SetupStates.SELECT_USER:
                with stylable_container("text_input", ""):
                    name = st.text_input("Full Name", "")

                st.button("# Submit", on_click=check_name, args=(name, ))  
            case SetupStates.SELECT_POSITION:
                pass
            case SetupStates.TRAIN:
                # training sequence
                training()

    case States.LOCAL:
        local_driving_grid()

        # TODO: Dani find a better way to check that a new result has been passed
        read_string = st.session_state['bci_selection_memory'].read_string()
        if len(read_string) > 0 and "[" in read_string:
            st.session_state["waiting_for_bci_response"] = False
            print(f"RECEIVED {read_string} FROM SHARED MEM")
            st.session_state['bci_selection_memory'].write_string("   ")

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
            

        if len(st.session_state["flash_sequence"]) > 0:
            st.session_state["flash_sequence"] = st.session_state["flash_sequence"][1:]
            time.sleep(0.1)
            st.rerun()
        elif st.session_state["waiting_for_bci_response"] == True:
            with st.spinner("Waiting for BCI..."):
                time.sleep(0.5)
                st.rerun()



    case States.DESTINATION:
        data = np.loadtxt('Frontend/data.txt')
        medoid_coordinates = np.loadtxt('Frontend/middles.txt')
        neighbourhood_points = np.loadtxt('Frontend/neighbourhood_points.txt').reshape((4, 4, 2))
        origin = np.loadtxt('Frontend/origin.txt')
        number_of_neighbourhoods = neighbourhood_points.shape[0]

        colours = [BLACK, GREEN, GREEN, GREEN, GREEN]
        switch_value = BUTTON_VALUE

        match st.session_state["map_sequence"][0]:
            case "1":
                colours = [BLACK, WHITE, GREEN, GREEN, GREEN]
                switch_value = BUTTON_VALUE
                send_marker(5, 1)
            case "2":
                colours = [BLACK, GREEN, WHITE, GREEN, GREEN]
                switch_value = BUTTON_VALUE
                send_marker(5, 2)
            case "3":
                colours = [BLACK, GREEN, GREEN, WHITE, GREEN]
                switch_value = BUTTON_VALUE
                send_marker(5, 3)
            case "4":
                colours = [BLACK, GREEN, GREEN, GREEN, WHITE]
                switch_value = BUTTON_VALUE
                send_marker(5, 4)
            case "switch":
                colours = [BLACK, GREEN, GREEN, GREEN, GREEN]
                switch_value = FLASH_VALUE
                send_marker(5, 0)
            case "Trial Started":      
                send_special_marker("Trial Started")
            case "Trial Ends":
                send_special_marker("Trial Ends")  

        fig = plt.figure(figsize=(7, 5))
        fig.patch.set_visible(False)
        colourmap = ListedColormap(colours)
        plt.imshow(data, cmap=colourmap, interpolation='nearest')
        plt.gca().invert_yaxis()
        plt.axis('off')
        plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
        plt.scatter(origin[0], origin[1], color='red', marker='*')
        plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
        buf = BytesIO()
        fig.savefig(buf, format="png")
        st.image(buf)

        if len(st.session_state["map_sequence"]) > 1:
            st.session_state["map_sequence"] = st.session_state["map_sequence"][1:]
            time.sleep(0.1)
            st.rerun()

        col1, col2 = st.columns([1, 1])
        with col1:
            st.button("# Run", on_click=give_map_sequence_list)
        with col2:
            with stylable_container("switch", css_styles=switch_value):
                st.button("⇄", on_click=switch)
