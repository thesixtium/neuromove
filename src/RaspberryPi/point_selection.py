import time

current_time = time.time()
print("\tImporting matplotlib.colors... ", end="")
from matplotlib.colors import ListedColormap
print(f"done ({time.time() - current_time}s)")
current_time = time.time()

print("\tImporting numpy... ", end="")
import numpy as np
print(f"done ({time.time() - current_time}s)")
current_time = time.time()

print("\tImporting ast... ", end="")
from ast import literal_eval    # only used to read in sample data
print(f"done ({time.time() - current_time}s)")
current_time = time.time()

print("\tImporting typing... ", end="")
from typing import Tuple
print(f"done ({time.time() - current_time}s)")
current_time = time.time()

print("\tImporting matplotlib.pyplot... ", end="")
import matplotlib.pyplot as plt
print(f"done ({time.time() - current_time}s)")
current_time = time.time()

print("\tImporting logging... ", end="")
import logging
print(f"done ({time.time() - current_time}s)")
current_time = time.time()

print("\tImporting queue... ", end="")
from queue import Queue
print(f"done ({time.time() - current_time}s)")
current_time = time.time()

print("\tImporting kmedoids... ", end="")
from kmedoids import fasterpam
print(f"done ({time.time() - current_time}s)")
current_time = time.time()

print("\tImporting scipy.spatial.distance... ", end="")
from scipy.spatial.distance import pdist, squareform, cdist
print(f"done ({time.time() - current_time}s)")

from src.RaspberryPi.InternalException import InvalidValueToPointSelection, NotEnoughSpaceInRoom, PamFailedPointSelection

# NOTE: data in the grid is stored with x direction in the columns and y direction in the rows
# so to index the coordinates, it is data[y, x]

# set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("point selection")
logger.setLevel(logging.INFO)
logger.debug("Logging initialized")

# TODO: make object oriented to prevent running helper functions. 
# aka only allow running of occupancy_grid_to_points
def occupancy_grid_to_points(
        input_data: str = None, 
        number_of_neighbourhoods: int = 4, number_of_points_per_neighbourhood: int = 4,
        plot_result: bool = False, save_result_to_disk: bool = False) -> Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, tuple]:
    '''
    Takes an occupancy grid with 0s as open spaces and 1s as obstacles and returns a list of points sorted into neighbourhoods.

    Args: 
    -------
        data : str
            The string representation of the data. None by default. When data is none, it will be read from shared memory. Only use this value for debugging
        origin : tuple, optional
            The origin of the grid. If not provided, the origin is set to the 
            middle of the grid.
        number_of_neighbourhoods : int, optional
            The number of neighbourhoods to divide the data into. Default is 4.
        number_of_points_per_neighbourhood : int, optional
            The number of points to select per neighbourhood. Default is 4.

    Returns:
    -------
        np.ndarray - data
            A 2D array containing all the data of the room. It is represented by values -1 to number_of_neighbourhoods where -1 is an obstacle and all other numbers are neighbourhoods. Access by using data[y][x]
        np.ndarray - medoid_coordinates
            a 2D array containing the centre point of each neighbourhood. Stored in order from neighbourhood 0 to N in [y,x] format.
        np.ndarray - neighbourhood_points
            a 3D array containing all selected points in each neighbourhood. The array will be of shape (N,M,2) where N is the number of neighbourhoods and M is the number of points per neighbourhood. The first point of each sub-list will be the medoid followed by all selected points. The lists are in order from neighbourhood 0 to N. Points are listed in [y,x] format
        tuple - origin
            The origin of the LiDAR sensor. In [y,x] format.

    Raises:
    -------
        ValueError
            If the data is None, not a 2D array, or if the number of neighbourhoods 
            or points per neighbourhood is less than 1 or greater than 5.

    Example:
    -------
    ```python
    data = np.array([[0, 1, 0], [0, 0, 1], [1, 0, 0]])
    origin = (1, 1)
    points = occupancy_grid_to_points(data, origin, 3, 2, True)
    print(points)
    ```
    '''

    data, origin = format_data(raw_data=input_data)
    
    if number_of_neighbourhoods < 1 or number_of_points_per_neighbourhood < 1:
        raise InvalidValueToPointSelection("Number of neighbourhoods and number of points per neighbourhood must be greater than 0")
    elif number_of_neighbourhoods > 5 or number_of_points_per_neighbourhood > 5:
        raise InvalidValueToPointSelection("Maximum number of neighbourhoods and points per neighbourhood is 5")
    
    #NOTE: do we need to account for cases where number of points or neighbourhoods is greater than the number of points in the data?
    
    if origin is None:
        # initialize origin to middle of the data
        origin = (data.shape[0] // 2, data.shape[1] // 2) 
    elif origin[0] < 0 or origin[0] >= data.shape[1] or origin[1] < 0 or origin[1] >= data.shape[0]:
        raise InvalidValueToPointSelection(f"Origin {origin} is not within the data with shape {data.shape}")
    
    logger.debug(f"Data has shape {data.shape}")
    logger.debug(f"Origin: {origin}")

    #NOTE: is it ok if we overwrite data?
    data, origin = find_room_size(data, origin)
    cropped_data = data.copy()

    logger.debug(f"Trimmed data shape: {data.shape}. New origin:  {origin}")

    # find all reachable nodes using breadth-first search
    reachable_points = bfs(data, origin)
    logger.debug(f"Reachable points found")

    if np.count_nonzero(reachable_points == 0) < number_of_neighbourhoods:
        raise NotEnoughSpaceInRoom(number_of_neighbourhoods, np.count_nonzero(reachable_points == 0))

    # run PAM to get the neighbourhoods
    medoid_coordinates, data = run_PAM(reachable_points, number_of_neighbourhoods)

    if medoid_coordinates is None or len(medoid_coordinates) != number_of_neighbourhoods:
        raise PamFailedPointSelection(number_of_neighbourhoods, len(medoid_coordinates))

    # get points in each neighbourhood
    # neighbourhood_points = get_points_in_neighbourhood(data, origin, medoid_coordinates, number_of_points_per_neighbourhood)
    # logger.debug(f"neighbourhood_points shape: {neighbourhood_points.shape}")
    neighbourhood_points = None

    if plot_result:
        colours = ['#202020', '#FFE18D', '#B3D88D', '#FF8383', '#E9BFE9']
        colourmap = ListedColormap(colours)
        img = plt.imshow(data, cmap=colourmap, interpolation='nearest')
        plt.gca().invert_yaxis()

        if save_result_to_disk:
            # save just colour zones
            plt.axis('off')
            plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
            plt.savefig('no-points.png', format='png', bbox_inches='tight', pad_inches=0)

        plt.scatter(origin[0], origin[1], color='red')
        plt.scatter(medoid_coordinates[:, 1], medoid_coordinates[:, 0], color='black')

        if save_result_to_disk:
            # save with origin and centers
            plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
            plt.savefig('center-points.png', format='png', bbox_inches='tight', pad_inches=0)

        dark_colours = ['#B78B14', '#547A2E', '#A62424', '#864385']

        # for i in range(number_of_neighbourhoods):
        #     plt.scatter(neighbourhood_points[i][:, 1], neighbourhood_points[i][:, 0], color=dark_colours[i])

        # replot medoids to make sure they're on top
        plt.scatter(medoid_coordinates[:, 1], medoid_coordinates[:, 0], color='black')

        if save_result_to_disk:
            # save with all points
            plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
            plt.savefig('all-points.png', format='png', bbox_inches='tight', pad_inches=0)
        
        plt.colorbar(img)
        plt.axis('on')
        
        # Remove axis labels and whitespace
        plt.show()

    logger.info("Point selection complete")

    if save_result_to_disk:
        np.savetxt("data.txt", data)
        np.savetxt('middles.txt', medoid_coordinates)
        # np.savetxt('neighbourhood_points.txt', neighbourhood_points.flatten())
        np.savetxt('origin.txt', origin)

    return data, cropped_data, medoid_coordinates, neighbourhood_points, origin

def format_data(raw_data: np.ndarray = None) -> Tuple[np.ndarray, tuple]:
    data = raw_data.T
    #data = literal_eval(raw_data)
    #data = np.array(data).T
    
    if len(data.shape) != 2:
        raise InvalidValueToPointSelection("Data must be a 2D array")

    origin = (data.shape[0] // 2, data.shape[1] // 2)

    return data, origin

def get_points_in_neighbourhood(data: np.ndarray, origin: np.ndarray, medoids: np.ndarray, num_points: int) -> np.ndarray:
    '''
    Given a map of clusters and their medoids, find num_points points in each neighbourhood.

    Args:
    -----
        data : np.ndarray
            2D array containing the cluster map. Each point is -1 if it is an obstacle,
            otherwise it is a number representing the cluster it belongs to
        medoids : np.ndarray
            2D array containing the coordinates of the medoids of each cluster
        num_points : int
            The number of points to select in each neighbourhood

    Returns:
    --------
        np.ndarray
            A 3D array containing the points in each neighbourhood. The first 
            dimension is the neighbourhood, the second dimension is the point, 
            and the third dimension is the x and y coordinates of the point.
    '''

    num_neighbourhoods = len(medoids)
    logger.debug(f"Detected {num_neighbourhoods} neighbourhoods")

    # convert cluster map to list of valid coordinates
    coords = np.argwhere(data != -1)

    # separate out neighbourhoods
    neighbourhoods = []
    for i in range(num_neighbourhoods):
        neighbourhoods.append(np.argwhere(data == i))

    # append origin temporarily to medoids
    # x and y are reversed in origin
    medoids = np.vstack([[origin[1], origin[0]], medoids])

    selected_points = np.array(medoids)
    remaining_points = np.array([p for p in coords if not np.any(np.all(p == medoids, axis=1))])

    # remove origin from medoids
    medoids = medoids[:-1]

    i = 0
    # need the +1 to account for the origin
    while selected_points.shape[0] < num_points * num_neighbourhoods + 1:
        current_neighbourhood = neighbourhoods[i]

        # calculate distances between all points in the neighbourhood and the selected points
        distances = cdist(current_neighbourhood, selected_points, metric='euclidean')

        # find the minimum distance for each point in the neighbourhood
        min_distances = np.min(distances, axis=1)

        # find the point with the maximum minimum distance
        best_point_index = np.argmax(min_distances)

        # check if the point is isolated from the neighbourhood
        valid_point = check_point_neighbours(current_neighbourhood[best_point_index], current_neighbourhood)

        if valid_point is True:
            # add the best point to the selected points
            selected_points = np.append(selected_points, [current_neighbourhood[best_point_index]], axis=0)

            i = (i + 1) % num_neighbourhoods
        else:
            # remove point from neighbourhood
            # since it has no neighbours, it shouldn't affect anything else
            neighbourhood_without_point = []
            for j in range(len(current_neighbourhood)):
                if j != best_point_index:
                    neighbourhood_without_point.append(current_neighbourhood[j])

            neighbourhoods[i] = np.array(neighbourhood_without_point)

        # remove the best point from the remaining points
        remaining_points = np.array([p for p in remaining_points if not np.all(p == current_neighbourhood[best_point_index])])

    # remove the first point from selected points (the origin)
    selected_points = selected_points[1:]

    # split the selected points into the different neighbourhoods
    selected_points_split = []
    for i in range(num_neighbourhoods):
        selected_points_split.append(selected_points[i::num_neighbourhoods])

    return np.array(selected_points_split)

def check_point_neighbours(point: list, neighbourhood: list) -> bool:
    '''
    Given a point and its neighbourhood, check if the point has any adjacent points in the neighbourhood. 

    Args:
    -----
        point : list
            a 2-long list with a coordinate point.
        neighbourhood : list
            List of coordinates included in the neighbourhood. 

    Returns:
    --------
        bool
            True if there is an adjacent point to the given one. False otherwise.
    '''

    adjacent_points = []
    for i in range(-1,2):
        for j in range (-1,2):
            if i == 0 and j == 0:
                continue

            adjacent_points.append([point[0]+i, point[1]+j])

    valid_point = any(np.any(np.all(neighbourhood == target, axis=1)) for target in adjacent_points)

    return valid_point

def run_PAM(data: np.ndarray, num_clusters: int) -> tuple[np.ndarray, np.ndarray]:
    '''
    Runs the Faster Partitioning Around Medoids (FasterPAM) algorithm on the given data.

    Args:
    -----
        data : np.ndarray
            2D array containing the occupancy grid. Each point is 0 if it is reachable,
            otherwise it is 1
        num_clusters : int
            The number of clusters to divide the data into. The k parameter of PAM

    Returns:
    --------
        tuple[np.ndarray, np.ndarray]
            A tuple containing the coordinates of the medoids and the cluster map.
            The cluster map contains a -1 if the point is an obstacle, otherwise it is 
            x where 0 <= x < num_clusters representing the cluster it belongs to.

    Raises:
    -------
        ValueError
            If the data is not convertible to float. Necessary for PAM to run
    '''

    # ensure that data is in float format
    try:
        data = data.astype(float)
    except:
        raise InvalidValueToPointSelection("Data must be convertible to float")

    # get coordinates of all reachable points
    reachable_coordinates = np.argwhere(data == 0)

    # calculate dissimilarities between all points
        # pdist -> pairwise distance between all points
            # in form [distance between 0 and 1, distance between 0 and 2, ...]
        # squareform -> convert to square matrix
            # where each index (i, j) is the distance between point i and point j
    dissimilarities = squareform(pdist(reachable_coordinates, metric='euclidean'))

    # run fasterPAM to get all neighbourhoods
    # random_state is set to 42 for reproducibility
    pam_result = fasterpam(dissimilarities, num_clusters, random_state=42)

    logger.debug(f"Loss: {pam_result.loss}")

    # map medoid indices to coordinates
    medoid_coords = reachable_coordinates[pam_result.medoids]

    # re-map data to include clusters
    # -1 now represents obstacles
    # 0-num_clusters represent the clusters
    cluster_map = np.full(data.shape, -1)
    for i, coord in enumerate(reachable_coordinates):
        cluster_map[coord[0]][coord[1]] = pam_result.labels[i]

    return medoid_coords, cluster_map

def bfs(data: np.ndarray, start: tuple) -> np.ndarray:
    '''
    Modified breadth-first search to find all reachable points from an occupancy grid.

    Args:
    -----
        data : np.ndarray
            2D array containing the occupancy grid. Each point is 0 if it is open,
            otherwise it is 1
        start : tuple
            The starting point for the search. The origin of the grid

    Returns:
    --------
        np.ndarray
            2D array containing the reachable points. Each point is 0 if it is reachable,
            otherwise it is 1. Modified to include the origin as a reachable point.
    '''

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
  
#TODO: handle edge case where there are no obstacles?
def find_room_size(data: np.ndarray, origin: tuple) -> tuple[np.ndarray, tuple]:
    '''
    Find the actual size of the room from the occupancy grid. The room is defined as the 
    smallest rectangle that contains all the obstacles.

    Args:
    -----
        data : np.ndarray
            2D array containing the occupancy grid. Each point is 0 if it is open,
            otherwise it is 1
        origin : tuple
            The origin of the grid. The point that the robot is at

    Returns:
    --------
        tuple[np.ndarray, tuple]
            A tuple containing the trimmed data and the new origin. The trimmed data
            is the smallest rectangle that contains all the obstacles. The new origin
            is the origin adjust to the new coordinate system.
    '''

    # initialize min and max values
    min_x = data.shape[1]
    max_x = 0
    min_y = data.shape[0]
    max_y = 0

    # find the min and max values
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

    # limit to only stuff in front of chair
    min_y = max(min_y, origin[1])

    # bottom left & top right corners
    top_right = (max_x, max_y)
    bottom_left = (min_x, min_y)  

    logger.debug(f"Bottom left: {bottom_left}, top right: {top_right}. Room size = {top_right[0] - bottom_left[0]} x {top_right[1] - bottom_left[1]}")

    # trim data to only include the room
    right_edge = np.min([data.shape[1], top_right[0]])
    top_edge = np.min([data.shape[0], top_right[1]])
    left_edge = np.max([0, bottom_left[0]])
    bottom_edge = np.max([0, bottom_left[1]])

    trimmed_data = data[bottom_edge:top_edge+1, left_edge:right_edge+1]

    # add border of 1s around the room
    trimmed_data = np.pad(trimmed_data, 1, constant_values=1)

    # adjust origin based on room size
    if origin[0] < bottom_left[0] or origin[0] > top_right[0] or origin[1] < bottom_left[1] or origin[1] > top_right[1]:
        raise InvalidValueToPointSelection(f"Origin {origin} is not within the found room size")
    
    # +1 is needed due to the padding added to the room
    origin = (origin[0] - bottom_left[0]+1, origin[1] - bottom_left[1]+1)

    return trimmed_data, origin

if __name__ == "__main__":
    # load in data from testData
    with open('raw_occupancy_grid.txt', 'r') as file:
        data_str = file.readlines()

    array_2d = np.array([list(map(float, line.split())) for line in data_str])

    selected_points = occupancy_grid_to_points(input_data=array_2d, plot_result=True, number_of_neighbourhoods=4, number_of_points_per_neighbourhood=4, save_result_to_disk=True)