import streamlit as st
from src.RaspberryPi.SharedMemory import SharedMemory

requested_next_state_memory = SharedMemory(shem_name="requested_next_state", size=10, create=False)

if st.button("Done"):
    requested_next_state_memory.write_string("3")
    st.switch_page("pages/local.py")