import streamlit as st
from streamlit_extras.stylable_container import stylable_container
from src.RaspberryPi.SharedMemory import SharedMemory

BUTTON_WIDTH = 100
BUTTON_HEIGHT = 100

st.markdown("""
            <style>
                div[data-testid="stColumn"] {
                    width: fit-content !important;
                    flex: unset;
                }
                div[data-testid="stColumn"] * {
                    width: 100px !important;
                }
            </style>
            """, unsafe_allow_html=True)

st.markdown(
    """
<style>
button {
    height: 100px;
    width: 100px !important;
}
</style>
""",
    unsafe_allow_html=True,
)

BLACK_KEY = "black"
BLACK_VALUE = """
            button {
            background-color: #000000;
            color: #000000;
            border-color: #000000;

        }"""
local_driving_memory = SharedMemory(shem_name="local_driving", size=1, create=False)


def direction_update(direction):
    local_driving_memory.write_string(direction)

col1, col2, col3 = st.columns([1,1,1])
with col1:
    with stylable_container( BLACK_KEY, css_styles=BLACK_VALUE ):
        st.button("1.1")
    st.button("<-", on_click=direction_update, args=("l",))
    with stylable_container( BLACK_KEY, css_styles=BLACK_VALUE ):
        st.button("1.3")
with col2:
    st.button("/\\", on_click=direction_update, args=("f",))
    st.button("-", on_click=direction_update, args=("s",))
    st.button("\\/", on_click=direction_update, args=("b",))
with col3:
    with stylable_container( BLACK_KEY, css_styles=BLACK_VALUE ):
        st.button("3.1")
    st.button("->", on_click=direction_update, args=("r",))
    with stylable_container( BLACK_KEY, css_styles=BLACK_VALUE ):
        st.button("3.3")