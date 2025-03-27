import time
import numpy as np
from io import BytesIO
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
from frontend_methods import *

import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR.replace(r"/Frontend", "")))

from src.RaspberryPi.InternalException import CannotReadSharedMemory, UnknownDestinationDrivingState
from src.RaspberryPi.point_selection import occupancy_grid_to_points


def state_destination():
    match st.session_state["destination_driving_state"]:
        case DestinationDrivingStates.MAP_ROOM:
            occupancy_grid = []
            while True:
                occupancy_grid = st.session_state['point_selection_memory'].read_np_array()
                if len(occupancy_grid) != 0:
                    st.session_state["occupancy_grid"] = occupancy_grid
                    st.session_state["destination_driving_state"] = DestinationDrivingStates.SELECT_DESTINATION
                    break
            # st.session_state["destination_driving_state"] = DestinationDrivingStates.SELECT_DESTINATION
            st.rerun()
        case DestinationDrivingStates.SELECT_DESTINATION:
            select_destination()
        case DestinationDrivingStates.TRANSLATE_TO_MOVEMENT:
            display_path(st.session_state["cropped_data"], st.session_state["origin"], st.session_state["target_location"], st.session_state["path"])
            st.button("# back to select", on_click=back_to_select)
        case _:
            raise UnknownDestinationDrivingState(st.session_state["destination_driving_state"])

def back_to_select():
    st.session_state["destination_driving_state"] = DestinationDrivingStates.SELECT_DESTINATION
    st.rerun()

def select_destination():
    # data = np.loadtxt('Frontend/data.txt')
    # origin = np.loadtxt('Frontend/origin.txt')
    # medoid_coordinates = [[14,12], [5,17],[17,26],[6,5],[5,29]]

    start_time = time.time()
    np.savetxt("raw_occupancy_grid.txt", st.session_state["occupancy_grid"])

    data, cropped_data, medoid_coordinates, neighbourhood_points, origin = occupancy_grid_to_points(st.session_state["occupancy_grid"], plot_result=True)
    for i in range(len(medoid_coordinates)):
        x = medoid_coordinates[i][0]
        y = medoid_coordinates[i][1]
        medoid_coordinates[i][0] = y
        medoid_coordinates[i][1] = x

        print(f"({y}, {x}) -> {len(data)} x {len(data[0])}")
        data[y][x] = 0

    st.session_state["neighbourhood_grid"] = data
    st.session_state["origin"] = origin

    np.savetxt("after_occupancy_grid_to_points.txt", data)
    print("occupancy_grid_to_points:\t%s" % (time.time() - start_time))

    start_time = time.time()
    move_content()
    print("move_content:\t%s" % (time.time() - start_time))

    colours = [BLACK, GREEN, REAL_PURPLE, PINK, ORANGE]
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

    display_map(data, origin, medoid_coordinates, colours)

    # TODO: GET RID OF THIS WHEN DONE DEBUGGING
    c1, c2, c3, c4 = st.columns(4)
    print()
    for coord in medoid_coordinates:
        print(coord)
    with c1:
        with stylable_container("c1", make_value(GREEN, BLACK, BLACK)):
            st.button("# 0", on_click=destination_driving_update, args=("0", data, origin, medoid_coordinates[0]))
    with c2:
        with stylable_container("c2", make_value(REAL_PURPLE, BLACK, BLACK)):
            st.button("# 1", on_click=destination_driving_update, args=("1", data, origin, medoid_coordinates[1]))
    with c3:
        with stylable_container("c3", make_value(PINK, BLACK, BLACK)):
            st.button("# 2", on_click=destination_driving_update, args=("2", data, origin, medoid_coordinates[2]))
    with c4:
        with stylable_container("c4", make_value(ORANGE, BLACK, BLACK)):
            st.button("# 3", on_click=destination_driving_update, args=("3", data, origin, medoid_coordinates[3]))

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
                destination_driving_update(target_region="0", point=medoid_coordinates[0])
            case "[1]":
                destination_driving_update(target_region="1", point=medoid_coordinates[1])
            case "[2]":
                destination_driving_update(target_region="2", point=medoid_coordinates[2])
            case "[3]":
                destination_driving_update(target_region="3", point=medoid_coordinates[3])
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

def display_map(data, origin, medoid_coordinates, colours):
    start_time = time.time()
    fig = plt.figure(figsize=(6, 4))
    fig.patch.set_visible(False)
    colourmap = ListedColormap(colours)
    plt.imshow(data, cmap=colourmap, interpolation='nearest')
    plt.gca().invert_yaxis()
    plt.axis('off')
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    plt.scatter(origin[0], origin[1], color='#fff59f', marker='*', s=[200])
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)

    for medoid in medoid_coordinates:
        plt.scatter(medoid[0], medoid[1], color='#ff0000', marker='*', s=[200])

    buf = BytesIO()
    fig.savefig(buf, format="png")
    st.image(buf)
    print("plot image:\t%s" % (time.time() - start_time))

def display_path(data, origin, target, path):
    fig = plt.figure(figsize=(6, 4))
    fig.patch.set_visible(False)
    plt.imshow(data)
    plt.plot(*zip(*path))
    plt.gca().invert_yaxis()
    plt.axis('off')

    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    plt.scatter(origin[0], origin[1], color='#fff59f', marker='*', s=[200])
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)

    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    plt.scatter(target[0], target[1], color='#fff59f', marker='*', s=[200])
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)

    buf = BytesIO()
    fig.savefig(buf, format="png")
    st.image(buf)
