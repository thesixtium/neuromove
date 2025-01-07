# TODO: only import necessary functions from libraries
from turtle import left
from matplotlib.pylab import f
import numpy as np
import ast
import matplotlib.pyplot as plt
import logging
from collections import deque
import time

# set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("point selection")
logger.setLevel(logging.DEBUG)
logger.debug("Logging initialized")

def bfs(data: np.ndarray, start: tuple) -> np.ndarray:
    # initialize values
    visited = np.zeros(data.shape, dtype=bool)
    stack = deque()

    # add start to stack
    stack.append(start)

    # while stack is not empty
    while stack:
        visited[start] = True
        stack.pop()

def find_room_size(data: np.ndarray) -> tuple:
    # find the farthest 1 in each direction

    min_x = data.shape[1]
    max_x = 0
    min_y = data.shape[0]
    max_y = 0

    for i in range(data.shape[0]):
        for j in range(data.shape[1]):
            if data[i, j] == 1:
                if i < min_y:
                    min_y = i
                if i > max_y:
                    max_y = i
                if j < min_x:
                    min_x = j
                if j > max_x:
                    max_x = j

    # bottom left & top right corners
    return (min_x, min_y), (max_x, max_y)  

def trim_and_border_data(data: np.ndarray, bottom_left: tuple, top_right: tuple) -> np.ndarray:
    # trim data to only include the room
    right_edge = np.min([data.shape[1], top_right[0]])
    top_edge = np.min([data.shape[0], top_right[1]])
    left_edge = np.max([0, bottom_left[0]])
    bottom_edge = np.max([0, bottom_left[1]])

    trimmed_data = data[bottom_edge:top_edge+1, left_edge:right_edge+1]

    # add border of 1s around the room
    trimmed_data = np.pad(trimmed_data, 1, constant_values=1)

    return trimmed_data

if __name__ == "__main__":
    # load in data from testData
    with open('testData', 'r') as file:
        data_str = file.read()
    
    # Convert the string representation of the list to an actual list
    sample_data = ast.literal_eval(data_str)
    
    # Convert the list to a numpy array
    sample_data = np.array(sample_data)
    origin = (sample_data.shape[0] // 2, sample_data.shape[1] // 2) # origin based on measurements from Aleks
    # origin = (97, 84) # origin based on where Aleks said it was
    
    logger.debug(f"Loaded data with shape {sample_data.shape}")
    logger.debug(f"Origin: {origin}")

    # find room size
    logger.debug(f"Starting to find room size at {time.time()}")
    bottom_left, top_right = find_room_size(sample_data)
    logger.debug(f"Room size found at {time.time()}")
    logger.debug(f"Bottom left: {bottom_left}")
    logger.debug(f"Top right: {top_right}")


    # adjust origin based on room size
    # +1 is needed due to the added padding
    origin = (origin[0] - bottom_left[0]+1, origin[1] - bottom_left[1]+1)
    logger.debug(f"Adjusted origin: {origin}")

    # add border of 1s around the room
    trimmed_data = trim_and_border_data(sample_data, bottom_left, top_right)

    # display on graph
    plt.imshow(trimmed_data, cmap='grey_r', interpolation='nearest')
    plt.colorbar()
    plt.gca().invert_yaxis()
    plt.scatter(origin[0], origin[1], color='red')
    plt.show()

    # find all reachable nodes using bfs

    # run fasterPAM to get all neighbourhoods

    # for each neighbourhood, find 5 additional points as far away from one another as possible
    # POTENTIAL ISSUE: are points all going to be on the border of the neighbourhood?
    # maybe add padding around the border to prevent this?