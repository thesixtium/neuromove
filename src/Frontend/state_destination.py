import time
import numpy as np
from io import BytesIO
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
from src.Frontend.frontend_methods import *


def state_destination():
    # read from shared memory
    try:
        point_selection_data = st.session_state['point_selection_memory'].read_np_array()
        data = point_selection_data[0]
        origin = point_selection_data[3]
    except:
        # use default data
        data = np.loadtxt('Frontend/data.txt')
        origin = np.loadtxt('Frontend/origin.txt')

    move_content()

    colours = [BLACK, GREEN, GREEN, GREEN, GREEN]
    switch_value = BUTTON_VALUE

    if len(st.session_state["map_sequence"]) > 0:
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
    plt.scatter(origin[0], origin[1], color='#fff59f', marker='*', s=[200])
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    buf = BytesIO()
    fig.savefig(buf, format="png")
    st.image(buf)

    col1, col2 = st.columns([1, 1])
    with col1:
        st.button("# Run", on_click=give_map_sequence_list)
    with col2:
        with stylable_container("switch", css_styles=switch_value):
            st.button("⇄", on_click=switch)

    read_string = st.session_state['bci_selection_memory'].read_string()
    if len(read_string) > 0 and "[" in read_string:
        st.session_state["waiting_for_bci_response"] = False
        print(f"RECEIVED {read_string} FROM SHARED MEM")
        st.session_state['bci_selection_memory'].write_string("   ")
        read_string = read_string.strip()

        match read_string:
            case "[0]":
                destination_driving_update(target_region="0")
            case "[1]":
                destination_driving_update(target_region="1")
            case "[2]":
                destination_driving_update(target_region="2")
            case "[3]":
                destination_driving_update(target_region="3")
            case "[4]":
                switch()
            case _:
                print("Not confident enough to make a decision")

    if len(st.session_state["map_sequence"]) > 0:
        st.session_state["map_sequence"] = st.session_state["map_sequence"][1:]
        time.sleep(0.1)
        st.rerun()

    elif st.session_state["waiting_for_bci_response"] == True:
        time.sleep(0.5)
        st.rerun()

    elif st.session_state["running"] == True and st.session_state["eye_tracking_memory"].read_string() == "0":
        time.sleep(0.1)
        st.rerun()
        
    elif st.session_state["waiting_for_bci_response"] == False and st.session_state["eye_tracking_memory"].read_string() == "1" and st.session_state["running"] == True and st.session_state["state"] == States.DESTINATION:
        give_map_sequence_list()
        st.rerun()