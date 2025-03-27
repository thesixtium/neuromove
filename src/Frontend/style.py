BUTTON_WIDTH = 100
BUTTON_HEIGHT = 100

PURPLE = "#667FAD"
BLACK = "#000000"
WHITE = "#FFFFFF"
GREEN = "#206A00"
OTHER = "#0F24FF"
DARK_TEXT = "#00061A"

PINK = "#ed7ece"
ORANGE = "#d49624"
REAL_PURPLE = "#62278c"

def make_value(background, color, border):
    return "button { background-color: " + background + "; color: " + color + "; border-color: " + border + "; }"

def add_padding(value, amount: int):
    '''
    Add a pixel amount of padding to the bottom of a button.
    '''
    return value[:-1] + "padding-bottom: " + str(amount) + "px;}"

def get_training_header_style():
    return """
    div {
        font-size: 15px;
        font-weight: 800;
        justify-content: right;
        color: """ + DARK_TEXT + ";}"

BACKGROUND_KEY = "purple"
BACKGROUND_VALUE = make_value(PURPLE, PURPLE, PURPLE)

BUTTON_KEY = "white"
BUTTON_VALUE = make_value(WHITE, BLACK, BLACK)

FLASH_KEY = "black"
FLASH_VALUE = make_value(BLACK, PURPLE, BLACK)

