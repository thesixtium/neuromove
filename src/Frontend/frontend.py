import os
print(f"Frontend: {os.listdir()}")

from src.Frontend.frontend_setup import run_setup
from src.Frontend.frontend_methods import *

from src.Frontend.state_local import state_local
from src.Frontend.state_setup import state_setup
from src.Frontend.state_destination import state_destination

run_setup()

match st.session_state["state"]:
    case States.SETUP:
        state_setup()

    case States.LOCAL:
        state_local()

    case States.DESTINATION:
        state_destination()
