import time

import streamlit as st
from src.RaspberryPi.States import States
from src.RaspberryPi.SharedMemory import SharedMemory

st.text("Loading...")

requested_next_state_memory = SharedMemory(shem_name="requested_next_state", size=10, create=False)
time.sleep(5)

if st.button("Done"):
    requested_next_state_memory.write_string("3")
    st.switch_page("local.py")