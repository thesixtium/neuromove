# TODO: only import necessary functions from libraries
import numpy as np
import ast
import matplotlib.pyplot as plt
import logging
import time
from queue import Queue
import kmedoids
from scipy.spatial.distance import pdist, squareform, cdist

# NOTE: data in the grid is stored with x direction in the columns and y direction in the rows
# so to index the coordinates, it is data[y, x]

# set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("point selection")
logger.setLevel(logging.DEBUG)
logger.debug("Logging initialized")

def get_points_in_neighbourhood(points: np.ndarray, neighbourhoods: np.ndarray, num_points: int, num_neighbourhoods: int, medoids: np.ndarray) -> np.ndarray:
    selected_points = np.array(medoids)
    remaining_points = np.array([p for p in points if not np.any(np.all(p == medoids, axis=1))])
 
    logger.debug(f"Number of selected points: {len(selected_points)}, number of remaining points: {len(remaining_points)}")

    i = 0
    while selected_points.shape[0] < num_points * num_neighbourhoods:
        current_neighbourhood = neighbourhoods[i]

        # calculate distances between all points in the neighbourhood and the selected points
        distances = cdist(current_neighbourhood, selected_points, metric='euclidean')

        # find the minimum distance for each point in the neighbourhood
        min_distances = np.min(distances, axis=1)

        # find the point with the maximum minimum distance
        best_point_index = np.argmax(min_distances)

        # add the best point to the selected points
        selected_points = np.append(selected_points, [current_neighbourhood[best_point_index]], axis=0)

        # remove the best point from the remaining points
        remaining_points = np.array([p for p in remaining_points if not np.all(p == current_neighbourhood[best_point_index])])

        i = (i + 1) % num_neighbourhoods

    logger.debug(f"Selected points: {selected_points.shape}")

    selected_points_split = []
    for i in range(num_neighbourhoods):
        selected_points_split.append(selected_points[i::num_neighbourhoods])

    return selected_points_split

def bfs(data: np.ndarray, start: tuple) -> np.ndarray:
    # initialize visited array & queue
    visited = np.zeros(data.shape, dtype=bool)
    queue = Queue()

    # add start to queue
    queue.put(start)
    visited[start[1], start[0]] = True

    # while queue is not empty
    while not queue.empty():
        # pop from queue
        current = queue.get()

        # check all neighbours (including diagonals)
        for y_offset in range (-1, 2):
            for x_offset in range (-1, 2):
                # add anything that is a 0 and hasn't been visited
                if data[current[1]+y_offset, current[0]+x_offset] == 0 and not visited[current[1]+y_offset, current[0]+x_offset]:
                    queue.put((current[0]+x_offset, current[1]+y_offset))
                    visited[current[1]+y_offset, current[0]+x_offset] = True

    # invert the array to return to the 0 = reachable, 1 = blocked format
    visited = np.invert(visited)

    return visited

    
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
    # plt.imshow(trimmed_data, cmap='gray_r', interpolation='nearest')
    # plt.colorbar()
    # plt.gca().invert_yaxis()
    # plt.scatter(origin[0], origin[1], color='red')
    # plt.gca().set_xticks(np.arange(-0.5, trimmed_data.shape[1], 1))
    # plt.gca().set_yticks(np.arange(-0.5, trimmed_data.shape[0], 1))

    # plt.grid(color='lightgrey', linestyle='-', linewidth=0.5)
    # plt.show(block=True)

    logger.debug(f"Trimmed data shape: {trimmed_data.shape}")

    # find all reachable nodes using bfs
    logger.debug(f"Starting bfs at {time.time()}")
    reachable = bfs(trimmed_data, origin)

    # display on graph
    # plt.imshow(reachable, cmap='gray_r', interpolation='nearest')
    # plt.colorbar()
    # plt.gca().invert_yaxis()
    # plt.scatter(origin[0], origin[1], color='red')
    # plt.show()

    # convert reachable the necessary input format
    # TODO: validate all of this section
    reachable = reachable.astype(float) # convert to float
    reachable_coordinates = np.argwhere(reachable == 0) # get coordinates of all reachable points
    dissimilarities = squareform(pdist(reachable_coordinates, metric='euclidean')) # calculate dissimilarities between all points

    # run fasterPAM to get all neighbourhoods
    start_time = time.time()
    logger.debug(f"Starting fasterPAM at {start_time}")
    pam_result = kmedoids.fasterpam(dissimilarities, 5, random_state=42)
    logger.debug(f"FasterPAM took {time.time() - start_time} seconds")

    logger.info(f"Loss: {pam_result.loss}")

    # Map medoid indices back to original coordinates
    medoid_coords = reachable_coordinates[pam_result.medoids]
    logger.debug(f"Medoid coordinates: {medoid_coords}")
    
    # re-map to array for visualization
    cluster_map = np.full(reachable.shape, -1)
    for i, coord in enumerate(reachable_coordinates):
        cluster_map[coord[0], coord[1]] = pam_result.labels[i]

    # display on graph
    # plt.imshow(cluster_map, cmap='tab10', interpolation='nearest')
    # plt.colorbar()
    # plt.gca().invert_yaxis()
    # plt.scatter(origin[0], origin[1], color='red')
    # plt.scatter(medoid_coords[:, 1], medoid_coords[:, 0], color='black')
    # plt.show()

    # organize the data into neighbourhoods
    neighbourhoods = []
    for i in range(5):
        cur_neighbourhood = np.argwhere(cluster_map == i)

        neighbourhoods.append(cur_neighbourhood)

        logger.debug(f"neighbourhood {i} has  {len(cur_neighbourhood)} points")

    # convert cluster_map to a list of coordinates
    cluster_map_coords = np.argwhere(cluster_map != -1)
    
    logger.debug(f"cluster_map shape: {cluster_map_coords.shape}, medoid_coords shape: {medoid_coords.shape}")
    logger.debug(f'asdf: {np.all(np.isin([14, 12], medoid_coords))}')

    # for each neighbourhood, find 5 additional points as far away from one another as possible
    neighbourhood_points = get_points_in_neighbourhood(cluster_map_coords, neighbourhoods, 5, 5, medoid_coords)

    # display on graph
    plt.imshow(cluster_map, cmap='Pastel1', interpolation='nearest')
    plt.colorbar()
    plt.gca().invert_yaxis()
    plt.scatter(origin[0], origin[1], color='red')
    colours = ['steelblue', 'darkslateblue', 'darkgoldenrod', 'darkmagenta', 'slategrey']

    for i in range(5):
        plt.scatter(neighbourhood_points[i][:, 1], neighbourhood_points[i][:, 0], color=colours[i])
    plt.scatter(medoid_coords[:, 1], medoid_coords[:, 0], color='black')
    plt.show()

