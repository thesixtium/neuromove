from frontend_setup import run_setup
from frontend_methods import *

from state_local import state_local
from state_setup import state_setup
from state_destination import state_destination

run_setup()

page_background = '''
<style>
[data-testid="stAppViewContainer"] {
background-color: #667FAD;
back
}
</style>
'''

st.markdown(page_background, unsafe_allow_html=True)

match st.session_state["state"]:
    case States.SETUP:
        state_setup()

    case States.LOCAL:
        state_local()

    case States.DESTINATION:
        state_destination()
