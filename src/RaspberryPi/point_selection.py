import numpy as np
from ast import literal_eval    # only used to read in sample data
import matplotlib.pyplot as plt
import logging
from queue import Queue
from kmedoids import fasterpam
from scipy.spatial.distance import pdist, squareform, cdist

# NOTE: data in the grid is stored with x direction in the columns and y direction in the rows
# so to index the coordinates, it is data[y, x]

# set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("point selection")
logger.setLevel(logging.DEBUG)
logger.debug("Logging initialized")

# TODO: make object oriented to prevent running helper functions. aka only allow running of occupancy_grid_to_points

def occupancy_grid_to_points(
        data: np.ndarray, 
        origin: tuple = None, 
        number_of_neighbourhoods: int = 5, number_of_points_per_neighbourhood: int = 5,
        plot_result: bool = False) -> np.ndarray:
    '''
    Takes an occupancy grid with 0s as open spaces and 1s as obstacles and returns a list of points sorted into neighbourhoods.

    Args: 
    -------
        data : np.ndarray
            2D array containig the occupancy grid. Must be a 2D array with 0s as 
            open spaces and 1s as obstacles.
        origin : tuple, optional
            The origin of the grid. If not provided, the origin is set to the 
            middle of the grid.
        number_of_neighbourhoods : int, optional
            The number of neighbourhoods to divide the data into. Default is 5.
        number_of_points_per_neighbourhood : int, optional
            The number of points to select per neighbourhood. Default is 5.

    Returns:
    -------
        np.ndarray
            A 3D array containing the points in each neighbourhood. The first 
            dimension is the neighbourhood, the second dimension is the point, 
            and the third dimension is the x and y coordinates of the point.

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

    # error checking
    if data is None:
        raise ValueError("Data cannot be None")
    
    if len(data.shape) != 2:
        raise ValueError("Data must be a 2D array")
    
    if number_of_neighbourhoods < 1 or number_of_points_per_neighbourhood < 1:
        raise ValueError("Number of neighbourhoods and number of points per neighbourhood must be greater than 0")
    elif number_of_neighbourhoods > 5 or number_of_points_per_neighbourhood > 5:
        raise ValueError("Maximum number of neighbourhoods and points per neighbourhood is 5")
    
    #NOTE: do we need to account for cases where number of points or neighbourhoods is greater than the number of points in the data?
    
    if origin is None:
        # initialize origin to middle of the data
        origin = (data.shape[0] // 2, data.shape[1] // 2) 
    elif origin[0] < 0 or origin[0] >= data.shape[1] or origin[1] < 0 or origin[1] >= data.shape[0]:
        raise ValueError(f"Origin is not within the data with shape {data.shape}")
    
    logger.debug(f"Data has shape {data.shape}")
    logger.debug(f"Origin: {origin}")

    #NOTE: is it ok if we overwrite data?
    data, origin = find_room_size(data, origin)

    logger.debug(f"Trimmed data shape: {data.shape}. New origin:  {origin}")

    # find all reachable nodes using breadth-first search
    reachable_points = bfs(data, origin)
    logger.debug(f"Reachable points found")

    # run PAM to get the neighbourhoods
    medoid_coordinates, data = run_PAM(reachable_points, number_of_neighbourhoods)

    if medoid_coordinates is None or len(medoid_coordinates) != number_of_neighbourhoods:
        raise ValueError(f"Expected {number_of_neighbourhoods} neighbourhoods, but only found {len(medoid_coordinates)}")

    # get points in each neighbourhood
    neighbourhood_points = get_points_in_neighbourhood(data, medoid_coordinates, number_of_points_per_neighbourhood)
    logger.debug(f"neighbourhood_points shape: {neighbourhood_points.shape}")

    if plot_result:
        plt.imshow(data, cmap='Pastel1', interpolation='nearest')
        plt.colorbar()
        plt.gca().invert_yaxis()
        plt.scatter(origin[0], origin[1], color='red')
        colours = ['steelblue', 'darkslateblue', 'darkgoldenrod', 'darkmagenta', 'slategrey']

        for i in range(number_of_neighbourhoods):
            plt.scatter(neighbourhood_points[i][:, 1], neighbourhood_points[i][:, 0], color=colours[i])
        plt.scatter(medoid_coordinates[:, 1], medoid_coordinates[:, 0], color='black')
        plt.show()

    logger.info("Point selection complete")
    return neighbourhood_points

def get_points_in_neighbourhood(data: np.ndarray, medoids: np.ndarray, num_points: int) -> np.ndarray:
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

    selected_points = np.array(medoids)
    remaining_points = np.array([p for p in coords if not np.any(np.all(p == medoids, axis=1))])

    i = 0
    while selected_points.shape[0] < num_points * num_neighbourhoods:
        current_neighbourhood = neighbourhoods[i]
        logger.debug(f"Current neighbourhood: {current_neighbourhood}")

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
        raise ValueError("Data must be convertible to float")

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
        raise ValueError(f"Origin {origin} is not within the found room size")
    
    # +1 is needed due to the padding added to the room
    origin = (origin[0] - bottom_left[0]+1, origin[1] - bottom_left[1]+1)

    return trimmed_data, origin

if __name__ == "__main__":
    # load in data from testData
    with open('../LiDAR/testData', 'r') as file:
        data_str = file.read()
    
    # Convert the string representation of the list to an actual list
    sample_data = literal_eval(data_str)

    # TODO: make sure this works properly with data from shared memory
    # rotate data 90 degress ccw & mirror over y-axis
    sample_data = np.array(sample_data).T

    plt.imshow(sample_data, cmap='grey_r', interpolation='nearest')
    plt.colorbar()
    plt.gca().invert_yaxis()
    plt.scatter(88, 88, color='red')
    plt.show()
    
    # Convert the list to a numpy array
    sample_data = np.array(sample_data)
    origin = (sample_data.shape[0] // 2, sample_data.shape[1] // 2) # origin based on measurements from Aleks
    # origin = (97, 84) # origin based on where Aleks said it was in our call

    selected_points = occupancy_grid_to_points(sample_data, origin, plot_result=True)
