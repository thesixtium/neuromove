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
    marker_info = StreamInfo(name='MarkerStream', type='Markers', channel_count=1, nominal_srate=250,
                             channel_format='string', source_id='Marker_Outlet')
    st.session_state["marker_outlet"] =  StreamOutlet(marker_info, 20, 360)
if "flash_sequence" not in st.session_state:
    st.session_state["flash_sequence"] = []
if "map_sequence" not in st.session_state:
    st.session_state["map_sequence"] = ["0"]
if "training_target" not in st.session_state:
    st.session_state["training_target"] = -1

print(st.session_state["state"])

match st.session_state["state"]:
    case States.SETUP:
        def start():
            st.session_state["requested_next_state_memory"].write_string("3")
            st.session_state["state"] = States.LOCAL

        # training sequence
        col1, col2 = st.columns([3,1])
        targets = ["↑", "←", "-", "→", "S"]

        with col1:
            # print(f"current target: f{st.session_state["training_target"]}")
            current_target = 0 if st.session_state["training_target"] < 0 else st.session_state["training_target"]
            st.header(f"Target: {targets[current_target]}")
        with col2:
            button_label = "Start"
            st.button(label=button_label, on_click=give_local_sequence_list, args=(1,))

        left_value = BUTTON_VALUE
        right_value = BUTTON_VALUE
        up_value = BUTTON_VALUE
        stop_value = BUTTON_VALUE
        switch_value = BUTTON_VALUE

        if len(st.session_state["flash_sequence"]) > 0:
            match st.session_state["flash_sequence"][0]:
                case "up":
                    up_value = FLASH_VALUE
                    send_marker(5, 2)
                case "left":
                    left_value = FLASH_VALUE
                    send_marker(5, 0)
                case "right":
                    right_value = FLASH_VALUE
                    send_marker(5, 1)
                case "stop":
                    stop_value = FLASH_VALUE
                    send_marker(5, 3)
                case "switch":
                    switch_value = FLASH_VALUE
                    send_marker(5, 4)            

        col1, col2, col3 = st.columns([1, 1, 1])
        with col1:
            with stylable_container(BACKGROUND_KEY, css_styles=BACKGROUND_VALUE):
                st.button("1.1")

            with stylable_container("left", css_styles=left_value):
                st.button("←", on_click=direction_update, args=("l",))

            with stylable_container(BACKGROUND_KEY, css_styles=BACKGROUND_VALUE):
                st.button("1.3")
        with col2:
            with stylable_container("up", css_styles=up_value):
                st.button("↑", on_click=direction_update, args=("f",))

            with stylable_container("stop", css_styles=stop_value):
                st.button("-", on_click=direction_update, args=("s",))
        with col3:
            with stylable_container(BACKGROUND_KEY, css_styles=BACKGROUND_VALUE):
                st.button("3.1")

            with stylable_container("right", css_styles=right_value):
                st.button("→", on_click=direction_update, args=("r",))

            with stylable_container(BACKGROUND_KEY, css_styles=BACKGROUND_VALUE):
                st.button("3.3")
        col1, col2 = st.columns([1, 1])
        with col1:
            with stylable_container(BUTTON_KEY, css_styles=BUTTON_VALUE):
                st.button("# Done", on_click=start)
        with col2:
            with stylable_container("switch", css_styles=switch_value):
                st.button("S", on_click=switch)

        if len(st.session_state["flash_sequence"]) > 0:
            st.session_state["flash_sequence"] = st.session_state["flash_sequence"][1:]
            time.sleep(0.1)
            st.rerun()
        else:
            st.session_state["training_target"] += 1
            give_local_sequence_list(1)

    case States.LOCAL:
        left_value = BUTTON_VALUE
        right_value = BUTTON_VALUE
        up_value = BUTTON_VALUE
        stop_value = BUTTON_VALUE
        switch_value = BUTTON_VALUE

        if len(st.session_state["flash_sequence"]) > 0:
            match st.session_state["flash_sequence"][0]:
                case "up":
                    up_value = FLASH_VALUE
                    send_marker(5, 2)
                case "left":
                    left_value = FLASH_VALUE
                    send_marker(5, 0)
                case "right":
                    right_value = FLASH_VALUE
                    send_marker(5, 1)
                case "stop":
                    stop_value = FLASH_VALUE
                    send_marker(5, 3)
                case "switch":
                    switch_value = FLASH_VALUE
                    send_marker(5, 4)

        col1, col2, col3 = st.columns([1, 1, 1])
        with col1:
            with stylable_container(BACKGROUND_KEY, css_styles=BACKGROUND_VALUE):
                st.button("1.1")

            with stylable_container("left", css_styles=left_value):
                st.button("←", on_click=direction_update, args=("l",))

            with stylable_container(BACKGROUND_KEY, css_styles=BACKGROUND_VALUE):
                st.button("1.3")
        with col2:
            with stylable_container("up", css_styles=up_value):
                st.button("↑", on_click=direction_update, args=("f",))

            with stylable_container("stop", css_styles=stop_value):
                st.button("-", on_click=direction_update, args=("s",))
        with col3:
            with stylable_container(BACKGROUND_KEY, css_styles=BACKGROUND_VALUE):
                st.button("3.1")

            with stylable_container("right", css_styles=right_value):
                st.button("→", on_click=direction_update, args=("r",))

            with stylable_container(BACKGROUND_KEY, css_styles=BACKGROUND_VALUE):
                st.button("3.3")

        col1, col2 = st.columns([1, 1])
        with col1:
            st.button("Run", on_click=give_local_sequence_list)
        with col2:
            with stylable_container("switch", css_styles=switch_value):
                st.button("S", on_click=switch)

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
