import serial
import pandas as pd
import streamlit as st
import time

if "ser" not in st.session_state:
    st.session_state["ser"] = ser = serial.Serial('COM3', 9600, timeout=1)
    st.session_state["ser"].reset_input_buffer()

if "df" not in st.session_state:
    st.session_state["df"] = pd.DataFrame(columns=['Raspberry Pi Current', 'Raspberry Pi Voltage'])

read = st.session_state["ser"].read().decode()
values = read.split(',')
st.session_state["df"].loc[len(st.session_state["df"])] = values

st.line_chart(st.session_state["df"], x=st.session_state["df"].index, y=st.session_state["df"]['Raspberry Pi Current'])
st.line_chart(st.session_state["df"], x=st.session_state["df"].index, y=st.session_state["df"]['Raspberry Pi Voltage'])

time.sleep(0.5)
st.rerun()
