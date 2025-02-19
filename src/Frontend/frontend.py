import streamlit as st
from src.RaspberryPi.States import States
import time
from streamlit_extras.stylable_container import stylable_container
from src.RaspberryPi.SharedMemory import SharedMemory
from random import shuffle
from src.Frontend.style import *
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
from io import BytesIO

with open("Frontend/frontend.css") as f:
    st.markdown("<style>{}</style>".format(f.read()), unsafe_allow_html=True)

local_driving_memory = SharedMemory(shem_name="local_driving", size=10, create=False)
requested_next_state_memory = SharedMemory(shem_name="requested_next_state", size=10, create=False)

if "state" not in st.session_state:
    st.session_state["state"] = States.SETUP

def switch():
    if st.session_state["state"] == States.LOCAL:
        st.session_state["state"] = States.DESTINATION
    else:
        st.session_state["state"] = States.LOCAL

print(st.session_state["state"])

match st.session_state["state"]:
    case States.SETUP:
        def start():
            requested_next_state_memory.write_string("3")
            st.session_state["state"] = States.LOCAL

        with stylable_container(BUTTON_KEY, css_styles=BUTTON_VALUE):
            st.button("# Done", on_click=start)


    case States.LOCAL:
        def give_sequence_list():
            all_buttons = ["up", "down", "left", "right", "stop", "switch"]
            return_list = []
            list_appends = 0

            while list_appends < 5:
                shuffle(all_buttons)
                if len(return_list) == 0 or return_list[-1] != all_buttons[0]:
                    return_list += all_buttons
                    list_appends += 1

            return_list = [item for pair in zip(return_list, ["break"] * len(return_list)) for item in pair][:-1]
            st.session_state["flash_sequence"] = return_list


        def direction_update(direction):
            local_driving_memory.write_string(direction)


        if "flash_sequence" not in st.session_state or isinstance(st.session_state["flash_sequence"], bool):
            st.session_state["flash_sequence"] = []

        if len(st.session_state["flash_sequence"]) > 0:
            if st.session_state["flash_sequence"][0] == "up":
                left_value = BUTTON_VALUE
                right_value = BUTTON_VALUE
                up_value = FLASH_VALUE
                down_value = BUTTON_VALUE
                stop_value = BUTTON_VALUE
                switch_value = BUTTON_VALUE
            elif st.session_state["flash_sequence"][0] == "down":
                left_value = BUTTON_VALUE
                right_value = BUTTON_VALUE
                up_value = BUTTON_VALUE
                down_value = FLASH_VALUE
                stop_value = BUTTON_VALUE
                switch_value = BUTTON_VALUE
            elif st.session_state["flash_sequence"][0] == "left":
                left_value = FLASH_VALUE
                right_value = BUTTON_VALUE
                up_value = BUTTON_VALUE
                down_value = BUTTON_VALUE
                stop_value = BUTTON_VALUE
                switch_value = BUTTON_VALUE
            elif st.session_state["flash_sequence"][0] == "right":
                left_value = BUTTON_VALUE
                right_value = FLASH_VALUE
                up_value = BUTTON_VALUE
                down_value = BUTTON_VALUE
                stop_value = BUTTON_VALUE
                switch_value = BUTTON_VALUE
            elif st.session_state["flash_sequence"][0] == "stop":
                left_value = BUTTON_VALUE
                right_value = BUTTON_VALUE
                up_value = BUTTON_VALUE
                down_value = BUTTON_VALUE
                stop_value = FLASH_VALUE
                switch_value = BUTTON_VALUE
            elif st.session_state["flash_sequence"][0] == "switch":
                left_value = BUTTON_VALUE
                right_value = BUTTON_VALUE
                up_value = BUTTON_VALUE
                down_value = BUTTON_VALUE
                stop_value = BUTTON_VALUE
                switch_value = FLASH_VALUE
            else:
                left_value = BUTTON_VALUE
                right_value = BUTTON_VALUE
                up_value = BUTTON_VALUE
                down_value = BUTTON_VALUE
                stop_value = BUTTON_VALUE
                switch_value = BUTTON_VALUE
        else:
            left_value = BUTTON_VALUE
            right_value = BUTTON_VALUE
            up_value = BUTTON_VALUE
            down_value = BUTTON_VALUE
            stop_value = BUTTON_VALUE
            switch_value = BUTTON_VALUE

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

            with stylable_container("down", css_styles=down_value):
                st.button("↓", on_click=direction_update, args=("b",))
        with col3:
            with stylable_container(BACKGROUND_KEY, css_styles=BACKGROUND_VALUE):
                st.button("3.1")

            with stylable_container("right", css_styles=right_value):
                st.button("→", on_click=direction_update, args=("r",))

            with stylable_container(BACKGROUND_KEY, css_styles=BACKGROUND_VALUE):
                st.button("3.3")

        col1, col2 = st.columns([1, 1])
        with col1:
            st.button("Run", on_click=give_sequence_list)
        with col2:
            with stylable_container("switch", css_styles=switch_value):
                st.button("S", on_click=switch)

        if len(st.session_state["flash_sequence"]) > 0:
            st.session_state["flash_sequence"] = st.session_state["flash_sequence"][1:]
            time.sleep(0.1)
            st.rerun()


    case States.DESTINATION:

        def give_sequence_list():
            all_buttons = ["1", "2", "3", "4", "switch"]
            return_list = []
            list_appends = 0

            while list_appends < 5:
                shuffle(all_buttons)
                if len(return_list) == 0 or return_list[-1] != all_buttons[0]:
                    return_list += all_buttons
                    list_appends += 1

            return_list = [item for pair in zip(return_list, ["0"] * len(return_list)) for item in pair]
            st.session_state["map_sequence"] = return_list


        if "map_sequence" not in st.session_state or isinstance(st.session_state["map_sequence"], bool):
            st.session_state["map_sequence"] = ["0"]


        data = np.loadtxt('Frontend/data.txt')
        medoid_coordinates = np.loadtxt('Frontend/middles.txt')
        neighbourhood_points = np.loadtxt('Frontend/neighbourhood_points.txt').reshape((4, 4, 2))
        origin = np.loadtxt('Frontend/origin.txt')
        number_of_neighbourhoods = neighbourhood_points.shape[0]

        fig = plt.figure(figsize=(7, 5))
        fig.patch.set_visible(False)
        if st.session_state["map_sequence"][0] == "1":
            colours = [BLACK, WHITE, GREEN, GREEN, GREEN]
            switch_value = BUTTON_VALUE
        elif st.session_state["map_sequence"][0] == "2":
            colours = [BLACK, GREEN, WHITE, GREEN, GREEN]
            switch_value = BUTTON_VALUE
        elif st.session_state["map_sequence"][0] == "3":
            colours = [BLACK, GREEN, GREEN, WHITE, GREEN]
            switch_value = BUTTON_VALUE
        elif st.session_state["map_sequence"][0] == "4":
            colours = [BLACK, GREEN, GREEN, GREEN, WHITE]
            switch_value = BUTTON_VALUE
        elif st.session_state["map_sequence"][0] == "switch":
            colours = [BLACK, GREEN, GREEN, GREEN, GREEN]
            switch_value = FLASH_VALUE
        else:
            colours = [BLACK, GREEN, GREEN, GREEN, GREEN]
            switch_value = BUTTON_VALUE

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
            print(st.session_state["map_sequence"])
            time.sleep(0.1)
            st.rerun()

        col1, col2 = st.columns([1, 1])
        with col1:
            st.button("Run", on_click=give_sequence_list)
        with col2:
            with stylable_container("switch", css_styles=switch_value):
                st.button("S", on_click=switch)
