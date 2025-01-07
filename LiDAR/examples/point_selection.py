# TODO: only import necessary functions from libraries
import numpy as np
import ast
import matplotlib.pyplot as plt
import logging

if __name__ == "__main__":
    # load in data from testData
    with open('testData', 'r') as file:
        data_str = file.read()
    
    # Convert the string representation of the list to an actual list
    sample_data = ast.literal_eval(data_str)
    
    # Convert the list to a numpy array
    sample_data = np.array(sample_data)
    
    print(sample_data.shape)

    # display on graph
    plt.imshow(sample_data, cmap='grey_r', interpolation='nearest')
    plt.colorbar()
    plt.show()

    # find room size

    # add border

    # find all reachable nodes

    # run fasterPAM to get all neighbourhoods

    # for each neighbourhood, find 5 additional points as far away from one another as possible
    # POTENTIAL ISSUE: are points all going to be on the border of the neighbourhood?
    # maybe add padding around the border to prevent this?