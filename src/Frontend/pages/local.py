import streamlit as st
from streamlit_extras.stylable_container import stylable_container
from src.RaspberryPi.SharedMemory import SharedMemory

from src.Frontend.style import *

with open("Frontend/pages/local.css") as f:
    st.markdown("<style>{}</style>".format(f.read()), unsafe_allow_html=True)


local_driving_memory = SharedMemory(shem_name="local_driving", size=10, create=False)


def direction_update(direction):
    local_driving_memory.write_string(direction)

col1, col2, col3 = st.columns([1,1,1])
with col1:
    with stylable_container( BACKGROUND_KEY, css_styles=BACKGROUND_VALUE ):
        st.button("1.1")
    st.button("←", on_click=direction_update, args=("l",))
    with stylable_container( BACKGROUND_KEY, css_styles=BACKGROUND_VALUE ):
        st.button("1.3")
with col2:
    st.button("↑", on_click=direction_update, args=("f",))
    st.button("-", on_click=direction_update, args=("s",))
    st.button("↓", on_click=direction_update, args=("b",))
with col3:
    with stylable_container( BACKGROUND_KEY, css_styles=BACKGROUND_VALUE ):
        st.button("3.1")
    st.button("→", on_click=direction_update, args=("r",))
    with stylable_container( BACKGROUND_KEY, css_styles=BACKGROUND_VALUE ):
        st.button("3.3")