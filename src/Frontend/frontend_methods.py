from random import shuffle

import streamlit as st

from pylsl import local_clock

from src.RaspberryPi.States import States

def send_marker(number_of_options: int, flashed_as_num: int, current_target: int = -1):
    st.session_state["marker_outlet"].push_sample([f"p300,s,{number_of_options},{current_target},{flashed_as_num}"], local_clock())

def give_local_sequence_list(total_list_appends: int = 5):
    all_buttons = ["up", "left", "right", "stop", "switch"]
    return_list = []
    list_appends = 0

    while list_appends < total_list_appends:
        shuffle(all_buttons)
        if len(return_list) == 0 or return_list[-1] != all_buttons[0]:
            return_list += all_buttons
            list_appends += 1

    return_list = [item for pair in zip(return_list, ["break"] * len(return_list)) for item in pair][:-1]
    st.session_state["flash_sequence"] = return_list

def give_map_sequence_list():
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

def direction_update(direction):
    st.session_state["local_driving_memory"].write_string(direction)

def switch():
    if st.session_state["state"] == States.LOCAL:
        st.session_state["state"] = States.DESTINATION
        st.session_state["requested_next_state_memory"].write_string("4")
    else:
        st.session_state["state"] = States.LOCAL
        st.session_state["requested_next_state_memory"].write_string("3")