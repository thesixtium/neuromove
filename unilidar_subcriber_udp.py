from src.RaspberryPi.SharedMemory import SharedMemory
from matplotlib import pyplot as plt
from src.RaspberryPi.point_selection import occupancy_grid_to_points
import numpy as np
from src.RaspberryPi.jps import jps, get_full_path

sm = SharedMemory("occupancy_grid", 284622, create=True)

while True:
    value = sm.read_grid()
    
    if value:
        print(value.split("|")[:-1])
        data = {'x': [], 'y': [], 'z': []}
        grid = [ [int(j) for j in i] for i in value.split("|")[:-1] ]

        #with open('testData', 'w') as f:
        #    f.write(str(grid))
        for x in range(len(grid)):
            for y in range(len(grid[0])):
                data['x'].append(x)
                data['y'].append(y)
                data['z'].append(grid[x][y])
        
        #fig = plt.figure(figsize=(6,6))
        #ax = fig.add_subplot()
        #ax.scatter(data['x'], data['y'], data['z'])
        #ax.scatter([len(grid)//2], [len(grid)//2], [1])
        #plt.show()
        #input()

        occupancy_grid = np.array(value)
        origin = (occupancy_grid.shape[0] // 2, occupancy_grid.shape[1] // 2)
        selected_points = occupancy_grid_to_points(occupancy_grid, origin, plot_result=True)
        print(selected_points)


        short_path = jps(value, origin[0], origin[1], 100, 100)
        full_path = get_full_path(short_path)

        fig = plt.figure(figsize=(6, 6))
        ax = fig.add_subplot()
        ax.scatter(data['x'], data['y'], data['z'])
        max_value = max(data['x'] + data['y'])
        enhance = 0.1

        ax.scatter([origin], [origin], s=5, c='red')
        ax.set_xlim([-(max_value * enhance), max_value * (1 + enhance)])
        ax.set_ylim([-(max_value * enhance), max_value * (1 + enhance)])

        p_x = []
        p_y = []
        for k in full_path:
            p_x.append(k[0])
            p_y.append(k[1])
        ax.plot(p_x, p_y, c="red")
        plt.show()



