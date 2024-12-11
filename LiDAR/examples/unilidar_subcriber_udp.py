import socket
import struct
import matplotlib.pyplot as plt
import re
import time
import pandas
from __future__ import print_function

import itertools, heapq

def _oc_to_grid_helper(x):
    return 0 if x != 0 else 1
_oc_to_grid_helper_vectorized = np.vectorize(_oc_to_grid_helper)

def oc_to_grid(oc):
    return _oc_to_grid_helper_vectorized(oc.pivot(index='x', columns='y', values='z').reindex(np.arange(0,100)).reindex(np.arange(0,100), axis=1).fillna(0).to_numpy())


edge_filter = [
    [-1, -1, -1],
    [-1, 8, -1],
    [-1, -1, -1]]


def _add_edge_buffer_helper(x):
    return 0 if x == 0 else 1


_add_edge_buffer_helper_vectorized = np.vectorize(_add_edge_buffer_helper)


# Need to add more stuff around border size
def add_edge_buffer(grid):
    if border_size == -1:
        return grid

    grid_with_edge_protection = [row[:] for row in grid]

    e2 = _add_edge_buffer_helper_vectorized(convolve(grid, edge_filter, mode='constant'))

    for x in range(len(e2)):
        for y in range(len(e2[0])):
            if e2[x][y] == 1:
                grid_with_edge_protection[x][y] = 0

    return grid_with_edge_protection


def display_grid(grid):
    data2 = {'x': [], 'y': [], 'z': []}
    for x in range(len(grid)):
        for y in range(len(grid[0])):
            data2['x'].append(x)
            data2['y'].append(y)
            data2['z'].append(grid[x][y])

    start_time = time.time()
    df2 = pd.DataFrame(data2)
    fig = plt.figure(figsize=(12, 12))
    ax = fig.add_subplot()
    ax.scatter(df2.x, df2.y, df2.z)

    plt.show()


# Define some constants representing the things that can be in a field.
OBSTACLE = 0
DESTINATION = -2
UNINITIALIZED = 1

DEBUG = False
VISUAL = True
expanded = [[False for j in range(150)] for i in range(200)]
visited = [[False for j in range(150)] for i in range(200)]


class FastPriorityQueue():
    """
    Because I don't need threading, I want a faster queue than queue.PriorityQueue
    Implementation copied from: https://docs.python.org/3.3/library/heapq.html
    """

    def __init__(self):
        self.pq = []  # list of entries arranged in a heap
        self.counter = itertools.count()  # unique sequence count

    def add_task(self, task, priority=0):
        'Add a new task'
        count = next(self.counter)
        entry = [priority, count, task]
        heapq.heappush(self.pq, entry)

    def pop_task(self):
        'Remove and return the lowest priority task. Raise KeyError if empty.'
        while self.pq:
            priority, count, task = heapq.heappop(self.pq)
            return task
        raise KeyError('pop from an empty priority queue')

    def empty(self):
        return len(self.pq) == 0


def jps(field, start_x, start_y, end_x, end_y):
    """
    Run a jump point search on a field with obstacles.

    Parameters
    field            - 2d array representing the cost to get to that node.
    start_x, start_y - the x, y coordinates of the starting position (must be ints)
    end_x, end_y     - the x, y coordinates of the destination (must be ints)

    Return:
    a list of tuples corresponding to the jump points. drawing straight lines betwen them gives the path.
    OR
    [] if no path is found.
    """
    global expanded, visited
    if VISUAL:
        expanded = [[False for j in range(len(field[0]))] for i in range(len(field))]
        visited = [[False for j in range(len(field[0]))] for i in range(len(field))]

        # handle obvious exception cases: either start or end is unreachable
    if field[start_x][start_y] == OBSTACLE:
        raise ValueError("No path exists: the start node is not walkable")
    if field[end_x][end_y] == OBSTACLE:
        raise ValueError("No path exists: the end node is not walkable")

    try:
        import queue
    except:
        import Queue as queue  # python 2 compatibility

    class FoundPath(Exception):
        """ Raise this when you found a path. it's not really an error,
        but I need to stop the program and pass it up to the real function"""
        pass

    def queue_jumppoint(node):
        """
        Add a jump point to the priority queue to be searched later. The priority is the minimum possible number of steps to the destination.
        Also check whether the search is finished.

        Parameters
        pq - a priority queue for the jump point search
        node - 2-tuple with the coordinates of a point to add.

        Return
        None
        """
        if node is not None:
            pq.add_task(node, field[node[0]][node[1]] + max(abs(node[0] - end_x), abs(node[1] - end_y)))

    def _jps_explore_diagonal(startX, startY, directionX, directionY):
        """
        Explores field along the diagonal direction for JPS, starting at point (startX, startY)

        Parameters
        startX, startY - the coordinates to start exploring from.
        directionX, directionY - an element from: {(1, 1), (-1, 1), (-1, -1), (1, -1)} corresponding to the x and y directions respectively.

        Return
        A 2-tuple containing the coordinates of the jump point if it found one
        None if no jumppoint was found.
        """
        cur_x, cur_y = startX, startY  # indices of current cell.
        curCost = field[startX][startY]

        while (True):
            cur_x += directionX
            cur_y += directionY
            curCost += 1

            if field[cur_x][cur_y] == UNINITIALIZED:
                field[cur_x][cur_y] = curCost
                sources[cur_x][cur_y] = startX, startY
                if VISUAL:
                    visited[cur_x][cur_y] = True
            elif cur_x == end_x and cur_y == end_y:  # destination found
                field[cur_x][cur_y] = curCost
                sources[cur_x][cur_y] = startX, startY
                if VISUAL:
                    visited[cur_x][cur_y] = True
                raise FoundPath()
            else:  # collided with an obstacle. We are done.
                return None

            # If a jump point is found,
            if field[cur_x + directionX][cur_y] == OBSTACLE and field[cur_x + directionX][
                cur_y + directionY] != OBSTACLE:
                return (cur_x, cur_y)
            else:  # otherwise, extend a horizontal "tendril" to probe the field.
                queue_jumppoint(_jps_explore_cardinal(cur_x, cur_y, directionX, 0))

            if field[cur_x][cur_y + directionY] == OBSTACLE and field[cur_x + directionX][
                cur_y + directionY] != OBSTACLE:
                return (cur_x, cur_y)
            else:  # extend a vertical search to look for anything
                queue_jumppoint(_jps_explore_cardinal(cur_x, cur_y, 0, directionY))

    def _jps_explore_cardinal(startX, startY, directionX, directionY):
        """
        Explores field along a cardinal direction for JPS (north/east/south/west), starting at point (startX, startY)

        Parameters
        startX, startY - the coordinates to start exploring from.
        directionX, directionY - an element from: {(1, 1), (-1, 1), (-1, -1), (1, -1)} corresponding to the x and y directions respectively.

        Result:
        A 2-tuple containing the coordinates of the jump point if it found one
        None if no jumppoint was found.
        """
        cur_x, cur_y = startX, startY  # indices of current cell.
        curCost = field[startX][startY]

        while (True):
            cur_x += directionX
            cur_y += directionY
            curCost += 1

            if field[cur_x][cur_y] == UNINITIALIZED:
                field[cur_x][cur_y] = curCost
                sources[cur_x][cur_y] = startX, startY
                if VISUAL:
                    visited[cur_x][cur_y] = True
            elif cur_x == end_x and cur_y == end_y:  # destination found
                field[cur_x][cur_y] = curCost
                sources[cur_x][cur_y] = startX, startY
                if VISUAL:
                    visited[cur_x][cur_y] = True
                raise FoundPath()
            else:  # collided with an obstacle or previously explored part. We are done.
                return None

            # check neighbouring cells, i.e. check if cur_x, cur_y is a jump point.
            if directionX == 0:
                if field[cur_x + 1][cur_y] == OBSTACLE and field[cur_x + 1][cur_y + directionY] != OBSTACLE:
                    return cur_x, cur_y
                if field[cur_x - 1][cur_y] == OBSTACLE and field[cur_x - 1][cur_y + directionY] != OBSTACLE:
                    return cur_x, cur_y
            elif directionY == 0:
                if field[cur_x][cur_y + 1] == OBSTACLE and field[cur_x + directionX][cur_y + 1] != OBSTACLE:
                    return cur_x, cur_y
                if field[cur_x][cur_y - 1] == OBSTACLE and field[cur_x + directionX][cur_y - 1] != OBSTACLE:
                    return cur_x, cur_y

    # MAIN JPS FUNCTION
    field = [[j for j in i] for i in field]  # this takes less time than deep copying.

    # Initialize some arrays and certain elements.
    sources = [[(None, None) for i in field[0]] for j in field]  # the jump-point predecessor to each point.
    field[start_x][start_y] = 0
    field[end_x][end_y] = DESTINATION

    pq = FastPriorityQueue()
    queue_jumppoint((start_x, start_y))

    # Main loop: iterate through the queue
    while (not pq.empty()):
        pX, pY = pq.pop_task()

        if VISUAL:
            expanded[pX][pY] = True

        try:
            queue_jumppoint(_jps_explore_cardinal(pX, pY, 1, 0))
            queue_jumppoint(_jps_explore_cardinal(pX, pY, -1, 0))
            queue_jumppoint(_jps_explore_cardinal(pX, pY, 0, 1))
            queue_jumppoint(_jps_explore_cardinal(pX, pY, 0, -1))

            queue_jumppoint(_jps_explore_diagonal(pX, pY, 1, 1))
            queue_jumppoint(_jps_explore_diagonal(pX, pY, 1, -1))
            queue_jumppoint(_jps_explore_diagonal(pX, pY, -1, 1))
            queue_jumppoint(_jps_explore_diagonal(pX, pY, -1, -1))
        except FoundPath:
            return _get_path(sources, start_x, start_y, end_x, end_y)

    raise ValueError("No path is found")
    # end of jps


def _get_path(sources, start_x, start_y, end_x, end_y):
    """
    Reconstruct the path from the source information as given by jps(...).

    Parameters
    sources          - a 2d array of the predecessor to each node
    start_x, start_y - the x, y coordinates of the starting position
    end_x, end_y     - the x, y coordinates of the destination

    Return
    a list of jump points as 2-tuples (coordinates) starting from the start node and finishing at the end node.
    """
    result = []
    cur_x, cur_y = end_x, end_y

    while cur_x != start_x or cur_y != start_y:
        result.append((cur_x, cur_y))
        cur_x, cur_y = sources[cur_x][cur_y]
    result.reverse()
    return [(start_x, start_y)] + result


def _signum(n):
    if n > 0:
        return 1
    elif n < 0:
        return -1
    else:
        return 0


def get_full_path(path):
    """
    Generates the full path from a list of jump points. Assumes that you moved in only one direction between
    jump points.

    Parameters
    path - a path generated by get_path

    Return
    a list of 2-tuples (coordinates) starting from the start node and finishing at the end node.
    """

    if path == []:
        return []

    cur_x, cur_y = path[0]
    result = [(cur_x, cur_y)]
    for i in range(len(path) - 1):
        while cur_x != path[i + 1][0] or cur_y != path[i + 1][1]:
            cur_x += _signum(path[i + 1][0] - path[i][0])
            cur_y += _signum(path[i + 1][1] - path[i][1])
            result.append((cur_x, cur_y))
    return result


def display_path(grid, path):
    data = {
        'x': [],
        'y': [],
        'z': [],
    }

    for x in range(len(grid)):
        for y in range(len(grid[0])):
            data['x'].append(x)
            data['y'].append(y)
            data['z'].append(grid[x][y])

    df = pd.DataFrame(data)

    fig = plt.figure(figsize=(12, 12))
    ax = fig.add_subplot()

    ax.scatter(df.x, df.y, (df.z + 1) % 2)

    p_x = []
    p_y = []
    for k in path:
        p_x.append(k[0])
        p_y.append(k[1])
    ax.plot(p_x, p_y, c="red")
    plt.show()


# IP and Port
UDP_IP = "127.0.0.1"
UDP_PORT = 12345

# Create UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

dataDict = {
            'x': [],
            'y': [],
            'z': []
        }

count = 0
points_scanned = 0
start_time = time.time()
points_for_full_room = 10000 

while True:
    points_scanned += 1
        
    # Recv data
    data, addr = sock.recvfrom(10000)
    data = data.decode()
    search = re.search('[(]([^,]+),([^,]+),([^)]+)[)]', data)
    if count % (points_for_full_room/100) == 0:
        print(f"{round(count/points_for_full_room * 100, 2)}")
    x = float(search.group(1))
    y = float(search.group(2))
    z = float(search.group(3))
    if z < 0.5:
        dataDict['x'].append(x)
        dataDict['y'].append(y)
        dataDict['z'].append(1)
        count += 1
        
    if count > points_for_full_room:
        count = 0
        print(f"Runtime: {(time.time() - start_time)} seconds")
        print(f"Points:  {points_scanned}")
        print(f"Seconds per point: {(time.time() - start_time) / points_scanned}")
        print(f"Seconds per 3000 points: {((time.time() - start_time) / points_scanned) * 3000}")
        start_time = time.time()

        df = pd.DataFrame(dataDict)

        start_time = time.time()

        grid_time = time.time()
        grid = oc_to_grid(df)
        grid = add_edge_buffer(grid)
        print(f"Path:\t{time.time() - grid_time}")

        path_time = time.time()
        path = get_full_path(jps(grid, 1, 1, 35, 36))
        print(f"Path:\t{time.time() - path_time}")

        print(f"Alg:\t{time.time() - start_time}")

        # display_grid(grid)
        display_path(grid, path)
        exit()

sock.close()
