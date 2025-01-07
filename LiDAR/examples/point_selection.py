# TODO: only import necessary functions from libraries
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

    min_x = data.shape[0]
    max_x = 0
    min_y = data.shape[1]
    max_y = 0

    for i in range(data.shape[0]):
        for j in range(data.shape[1]):
            if data[i, j] == 1:
                if i < min_x:
                    min_x = i
                if i > max_x:
                    max_x = i
                if j < min_y:
                    min_y = j
                if j > max_y:
                    max_y = j

    # bottom left & top right corners
    return (min_x, min_y), (max_x, max_y)  

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

    # display on graph
    plt.imshow(sample_data, cmap='grey_r', interpolation='nearest')
    plt.colorbar()
    plt.gca().invert_yaxis()
    plt.scatter(origin[0], origin[1], color='red')

    plt.show()

    # find room size
    logger.debug(f"Starting to find room size at {time.time()}")
    bottom_left, top_right = find_room_size(sample_data)
    logger.debug(f"Room size found at {time.time()}")
    logger.debug(f"Bottom left: {bottom_left}")
    logger.debug(f"Top right: {top_right}")

    # add border of 1s around the room
    # may not need to do this if not using jps

    # find all reachable nodes using bfs

    # run fasterPAM to get all neighbourhoods

    # for each neighbourhood, find 5 additional points as far away from one another as possible
    # POTENTIAL ISSUE: are points all going to be on the border of the neighbourhood?
    # maybe add padding around the border to prevent this?