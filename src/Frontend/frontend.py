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
if "flash_sequence" not in st.session_state:
    st.session_state["flash_sequence"] = []
if "map_sequence" not in st.session_state:
    st.session_state["map_sequence"] = ["0"]
if "training_target" not in st.session_state:
    st.session_state["training_target"] = -1

print(st.session_state["state"])

match st.session_state["state"]:
    case States.SETUP:
        # training sequence
        col1, col2, col3= st.columns([5,1,1])
        targets = ["↑", "←", "-", "→", "S"]

        with col1:
            st.header("Target")
        with col2:
            current_target = 0 if st.session_state["training_target"] < 0 else st.session_state["training_target"]
            st.header(f"{targets[current_target]}")
        with col3:
            button_label = "Start" if st.session_state["training_target"] < 0 else "Continue"

            if st.session_state["training_target"] < len(targets) - 1:
                st.button(label=button_label, on_click=start_training_next_target)
            else:
                st.button("Got to Local", on_click=start)

        local_driving_grid(training=True)
        
        if len(st.session_state["flash_sequence"]) > 0:
            st.session_state["flash_sequence"] = st.session_state["flash_sequence"][1:]
            time.sleep(0.1)
            st.rerun()
        else:
            st.session_state["training_target"] += 1

    case States.LOCAL:
        local_driving_grid()

        if len(st.session_state["flash_sequence"]) > 0:
            st.session_state["flash_sequence"] = st.session_state["flash_sequence"][1:]
            time.sleep(0.1)
            st.rerun()


    case States.DESTINATION:
        data = np.loadtxt('Frontend/data.txt')
        medoid_coordinates = np.loadtxt('Frontend/middles.txt')
        neighbourhood_points = np.loadtxt('Frontend/neighbourhood_points.txt').reshape((4, 4, 2))
        origin = np.loadtxt('Frontend/origin.txt')
        number_of_neighbourhoods = neighbourhood_points.shape[0]

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
            case _:
                colours = [BLACK, GREEN, GREEN, GREEN, GREEN]
                switch_value = BUTTON_VALUE

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
            st.button("Run", on_click=give_map_sequence_list)
        with col2:
            with stylable_container("switch", css_styles=switch_value):
                st.button("S", on_click=switch)
