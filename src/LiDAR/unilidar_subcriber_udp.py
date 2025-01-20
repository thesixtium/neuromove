from src.RaspberryPi.SharedMemory import SharedMemory
from matplotlib import pyplot as plt

sm = SharedMemory("occupancy_grid", 284622, create=False)

while True:
    value = sm.read_grid()
    
    if value:
        print(value.split("|")[:-1])
        data = {'x': [], 'y': [], 'z': []}
        grid = [ [int(j) for j in i] for i in value.split("|")[:-1] ]

        with open('testData', 'w') as f:
            f.write(str(grid))
        for x in range(len(grid)):
            for y in range(len(grid[0])):
                data['x'].append(x)
                data['y'].append(y)
                data['z'].append(grid[x][y])
        
        fig = plt.figure(figsize=(6,6))
        ax = fig.add_subplot()
        ax.scatter(data['x'], data['y'], data['z'])
        ax.scatter([len(grid)//2], [len(grid)//2], [1])
        plt.show()
        input()


