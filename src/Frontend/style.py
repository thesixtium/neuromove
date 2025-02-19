BUTTON_WIDTH = 100
BUTTON_HEIGHT = 100

PURPLE = "#8757b3"
BLACK = "#000000"
WHITE = "#FFFFFF"

def make_value(background, color, border):
    return "button { background-color: " + background + "; color: " + color + "; border-color: " + border + "; }"

BACKGROUND_KEY = "purple"
BACKGROUND_VALUE = make_value(PURPLE, PURPLE, PURPLE)

BUTTON_KEY = "white"
BUTTON_VALUE = make_value(WHITE, BLACK, BLACK)
