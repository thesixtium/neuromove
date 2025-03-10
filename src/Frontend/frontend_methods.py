from random import shuffle
from time import sleep
from os.path import join, exists, dirname

import streamlit as st
from streamlit_extras.stylable_container import stylable_container

from pylsl import local_clock

from src.Frontend.enums import ScreenPosition
from src.Frontend.style import *
from src.RaspberryPi.States import SetupStates, States

NUMBER_OF_TRAINING_CYCLES = 20
NUMBER_OF_DECISION_CYCLES = 5

def send_marker(number_of_options: int, flashed_as_num: int, current_target: int = -1):
    st.session_state["marker_outlet"].push_sample([f"p300,s,{number_of_options},{current_target},{flashed_as_num}"], local_clock())

def send_special_marker(string: str):
    st.session_state["marker_outlet"].push_sample([string], local_clock())


def start_training_next_target():
    # don't do anything if currently flashing
    if len(st.session_state["flash_sequence"]):
        return

    st.session_state["training_target"] += 1
    st.session_state["currently_training"] = True
    give_local_sequence_list(NUMBER_OF_TRAINING_CYCLES)

    
def give_local_sequence_list(total_list_appends: int = NUMBER_OF_DECISION_CYCLES):
    all_buttons = ["up", "left", "right", "stop", "switch"]
    return_list = []
    list_appends = 0

    while list_appends < total_list_appends:
        shuffle(all_buttons)
        if len(return_list) == 0 or return_list[-1] != all_buttons[0]:
            return_list += all_buttons
            list_appends += 1

    return_list = [item for pair in zip(return_list, ["break"] * len(return_list)) for item in pair][:-1]
    return_list = ["Trial Started"] + return_list + ["Trial Ends"]
    
    if st.session_state["currently_training"] == False:
        st.session_state["waiting_for_bci_response"] = True

    if st.session_state["training_target"] == 4 and st.session_state["state"] == States.SETUP:
        return_list = return_list + ["Training Complete"]

    st.session_state["flash_sequence"] = return_list

def give_map_sequence_list(total_list_appends: int = NUMBER_OF_DECISION_CYCLES):
    all_buttons = ["1", "2", "3", "4", "switch"]
    return_list = []
    list_appends = 0

    while list_appends < total_list_appends:
        shuffle(all_buttons)
        if len(return_list) == 0 or return_list[-1] != all_buttons[0]:
            return_list += all_buttons
            list_appends += 1

    return_list = [item for pair in zip(return_list, ["0"] * len(return_list)) for item in pair]
    return_list = ["Trial Started"] + return_list + ["Trial Ends"]

    if st.session_state["currently_training"] == False: 
        st.session_state["waiting_for_bci_response"] = True

    st.session_state["map_sequence"] = return_list

def direction_update(direction):
    st.session_state["local_driving_memory"].write_string(direction)

def destination_driving_update(target_region):
    # TODO: Implement
    print(f"Destination selected {target_region}, doing nothing right now")

def switch():
    if st.session_state["state"] == States.LOCAL:
        st.session_state["state"] = States.DESTINATION
        st.session_state["requested_next_state_memory"].write_string("4")
    else:
        st.session_state["state"] = States.LOCAL
        st.session_state["requested_next_state_memory"].write_string("3")

def local_driving_grid(training: bool = False):
    left_value = BUTTON_VALUE
    right_value = BUTTON_VALUE
    up_value = BUTTON_VALUE
    stop_value = BUTTON_VALUE
    switch_value = BUTTON_VALUE

    current_target = st.session_state["training_target"] if training is True else -1

    if len(st.session_state["flash_sequence"]) > 0:
        match st.session_state["flash_sequence"][0]:
            case "up":
                up_value = FLASH_VALUE
                send_marker(5, 0, current_target)
            case "left":
                left_value = FLASH_VALUE
                send_marker(5, 1, current_target)
            case "right":
                right_value = FLASH_VALUE
                send_marker(5, 3, current_target)
            case "stop":
                stop_value = FLASH_VALUE
                send_marker(5, 2, current_target)
            case "switch":
                switch_value = FLASH_VALUE
                send_marker(5, 4, current_target) 
            case "Trial Started":      
                send_special_marker("Trial Started")
            case "Trial Ends":
                send_special_marker("Trial Ends")  
            case "Training Complete":
                send_special_marker("Training Complete")   

    col1, col2, col3 = st.columns([1, 1, 1], vertical_alignment="bottom")
    function_to_call = direction_update if training is False else None
    with col1:
        with stylable_container("left", css_styles=add_padding(left_value, 13)):
            st.button("←", on_click=function_to_call, args=("l",))
    with col2:
        with stylable_container("up", css_styles=add_padding(up_value, 13)):
            st.button("↑", on_click=function_to_call, args=("f",))

        with stylable_container("stop", css_styles=stop_value):
            st.button("⯃", on_click=function_to_call, args=("s",))
    with col3:
        with stylable_container("right", css_styles=add_padding(right_value, 13)):
            st.button("→", on_click=function_to_call, args=("r",))

    col1, col2 = st.columns([1, 1])
    function_to_call = switch if training is False else None
    with col1:
        if training is True:
            st.button("# Placeholder", on_click=None)
        else:
            st.button("# Run", on_click=give_local_sequence_list)

    with col2:
        with stylable_container("switch_mode", css_styles=add_padding(switch_value, 11)):
            st.button("⇄", on_click=function_to_call)

def start():
    st.session_state["requested_next_state_memory"].write_string("3")
    st.session_state["state"] = States.LOCAL

def training():
    col1, col2, col3= st.columns([5,1,1])
    targets = ["↑", "←", "⯃", "→", "⇄"]

    with col1:
        with stylable_container("training_header", get_training_header_style()):
            current_target = 0 if st.session_state["training_target"] < 0 else st.session_state["training_target"]
            st.text(f"Target: {targets[current_target]}")
            if current_target < len(targets) - 1:
                st.text(f"Next target: {targets[current_target + 1]}")
            else:
                # placeholder to get rid of undesired text when it's not needed
                st.text("")
    with col2:
        if st.session_state["currently_training"] is False and st.session_state['training_target'] != -1:
            st.text("Done!")
        else:
            # placeholder to get rid of undesired text when it's not needed
            st.text("")
    with col3:
        button_label = "Start" if st.session_state["training_target"] < 0 else "Continue"

        if st.session_state["training_target"] < len(targets) - 1:
            st.button(label=f"# {button_label}", on_click=start_training_next_target)
        else:
            st.button("# Go to Local", on_click=start)

    local_driving_grid(training=True)
    
    if len(st.session_state["flash_sequence"]) > 0:
        st.session_state["flash_sequence"] = st.session_state["flash_sequence"][1:]
        sleep(0.1)
        st.rerun()
    elif st.session_state["currently_training"] is True:
        st.session_state["currently_training"] = False
        st.rerun()

def check_name(name: str):
    fmt_name = name.upper().replace(" ", "_")
    if len(fmt_name) >= 20:
        fmt_name = fmt_name[:19]

    model_file_name = "save_on_exit_" + fmt_name + ".pk1"

    model_path = join(dirname(__file__), "..", "..", "models", model_file_name)

    if exists(model_path):
        st.session_state["bci_selection_memory"].write_string(fmt_name)

        print(f"File found. Wrote {fmt_name} to shared mem")
    else:
        st.session_state["bci_selection_memory"].write_string("N/A")

        print("File not found. Wrote N/A to shared mem")

    st.session_state["setup_substate"] = SetupStates.SELECT_POSITION

def move_content():
    '''
    Move content to the selected region of the screen.
    Left, right or centre.
    '''

    match st.session_state["screen_position"]:
        case ScreenPosition.LEFT:
            css_pos = "start"
        case ScreenPosition.CENTRE:
            css_pos = "center"
        case ScreenPosition.RIGHT:
            css_pos = "right"
    st.markdown("""<style>
    .stMain {
        display: flex;
        flex-direction: row;
        justify-content:""" + css_pos + """;
    } </style>
    """, unsafe_allow_html=True)
