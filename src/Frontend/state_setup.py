from frontend_methods import *
from enums import *
from ..RaspberryPi.States import SetupStates


def state_setup():
    match st.session_state["setup_substate"]:
        case SetupStates.SELECT_USER:
            with stylable_container("input-header", css_styles="""
                        .stText {
                            font-size: 24px;
                            font-weight: bold;  
                            justify-content: start;                  
                        }
                    """):
                st.text("Please enter the user's first and last name")

            name = st.text_input("", "", placeholder="First Name Last Name")

            with stylable_container("name_button_container", css_styles="""
                        .stButton {
                            justify-content: start;                    
                        }
                    """):
                st.button("# Submit", on_click=check_name, args=(name,))
        case SetupStates.SELECT_POSITION:
            with stylable_container("position_text", css_styles="""
                        div {font-size: 30px;
                        font-weight: bold;}
                    """):
                st.text("Which the area is most visible?")

            def set_screen_position(position: ScreenPosition):
                st.session_state["screen_position"] = position
                st.session_state["setup_substate"] = SetupStates.TRAIN

            with stylable_container("button-container", css_styles="""
                    .stHorizontalBlock {
                        display: flex;
                        flex-direction: row;
                        justify-content: space-around;
                    }

                    .stVerticalBlock {
                        width: 100vw;
                    }
                    """):
                col1, col2, col3 = st.columns(3)
                with col1:
                    st.button("# Left", on_click=set_screen_position, args=(ScreenPosition.LEFT,))
                with col2:
                    st.button("# Centre", on_click=set_screen_position, args=(ScreenPosition.CENTRE,))
                with col3:
                    st.button("# Right", on_click=set_screen_position, args=(ScreenPosition.RIGHT,))

            st.markdown(
                """
                <div class="screen-area-container">
                    <div class="screen-area", id="left">Left</div>
                    <div class="spacer"></div>
                    <div class="screen-area", id="centre">Centre</div>
                    <div class="spacer"></div>
                    <div class="screen-area", id="right">Right</div>
                </div>
                """,
                unsafe_allow_html=True
            )

        case SetupStates.TRAIN:
            move_content()

            # training sequence
            training()