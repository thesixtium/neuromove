import time

import numpy as np
import pandas as pd
from matplotlib import pyplot as plt
from scipy.ndimage import convolve

border_size = 10

def _oc_to_grid_helper(x):
    return 0 if x != 0 else 1
_oc_to_grid_helper_vectorized = np.vectorize(_oc_to_grid_helper)

def oc_to_grid(oc, n):
    return _oc_to_grid_helper_vectorized(oc.pivot(index='x', columns='y', values='z').reindex(np.arange(0,n)).reindex(np.arange(0,n), axis=1).fillna(0).to_numpy())


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


def display_grid(xs, ys, zs, origin):
    fig = plt.figure(figsize=(6, 6))
    ax = fig.add_subplot()
    ax.scatter(xs, ys, zs)
    max_value = max(xs + ys)
    enhance = 0.1
    
    ax.scatter([origin], [origin], s=5, c='red')
    ax.set_xlim([-(max_value * enhance), max_value*(1+enhance)])
    ax.set_ylim([-(max_value * enhance), max_value*(1+enhance)])

    plt.show()
    
def display_grid_old(grid, old_grid):
    data = {'x': [], 'y': [], 'z': []}
    for x in range(0, len(grid), 2):
        for y in range(0, len(grid[0]), 2):
            data['x'].append(x)
            data['y'].append(y)
            data['z'].append(grid[x][y])
            
    data2 = {'x': [], 'y': [], 'z': []}
    for x in range(0, len(grid), 2):
        for y in range(0, len(grid[0]), 2):
            data2['x'].append(x)
            data2['y'].append(y)
            data2['z'].append(old_grid[x][y])

    fig = plt.figure(figsize=(6, 6))
    ax = fig.add_subplot()
    ax.scatter(data['x'], data['y'], data['z'], label='new')
    ax.scatter(data2['x'], data2['y'], data2['z'], label='old')
    
    plt.legend(loc='upper left')

    plt.show()


def display_path(xs, ys, zs, origin, path):
    fig = plt.figure(figsize=(6, 6))
    ax = fig.add_subplot()
    ax.scatter(xs, ys, zs)
    max_value = max(xs + ys)
    enhance = 0.1
    
    ax.scatter([origin], [origin], s=5, c='red')
    ax.set_xlim([-(max_value * enhance), max_value*(1+enhance)])
    ax.set_ylim([-(max_value * enhance), max_value*(1+enhance)])

    p_x = []
    p_y = []
    for k in path:
        p_x.append(k[0])
        p_y.append(k[1])
    ax.plot(p_x, p_y, c="red")
    plt.show()
