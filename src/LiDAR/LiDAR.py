import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR.replace(r"/LiDAR", r"")))

from src.LiDAR.build.RunLiDAR import RunLiDAR
from src.RaspberryPi.SharedMemory import SharedMemory

class LiDAR:
    def __init__(self):
        self.edge_buffer = 2
        self.orign_buffer = 4

        self.occupancy_grid_memory = SharedMemory(shem_name="occupancy_grid", size=28462200, create=True)
        RunLiDAR()

    def get_grid(self) -> tuple[list[list[int]], tuple[int, int]]:
        while True:
            grid = self.occupancy_grid_memory.read_grid()
            if len(grid) > 1:
                # Convolve
                convolved_grid = [[0 for _ in grid[0]] for _ in grid]
                for i in range(len(grid)):
                    for j in range(len(grid[0])):
                        if grid[i][j] == 1:
                            x_range = range(max(0, i-self.edge_buffer), min(len(grid), i+self.edge_buffer+1))
                            y_range = range(max(0, j-self.edge_buffer), min(len(grid[0]), j+self.edge_buffer+1))
                            for x in x_range:
                                for y in y_range:
                                    convolved_grid[x][y] = 1

                # Shrink
                rows = len(convolved_grid)
                cols = len(convolved_grid[0])
                min_row = rows
                max_row = -1
                min_col = cols
                max_col = -1
                for r in range(rows):
                    for c in range(cols):
                        if convolved_grid[r][c] == 1:
                            min_row = min(min_row, r)
                            max_row = max(max_row, r)
                            min_col = min(min_col, c)
                            max_col = max(max_col, c)
                cropped_grid = [row[min_col:max_col + 1] for row in convolved_grid[min_row:max_row + 1]]

                # Clear Origin
                origin = (len(grid[0]) // 2 - min_col, len(grid) // 2 - min_row)
                x_range = range(max(0, origin[1] - self.orign_buffer), min(len(cropped_grid), origin[1] + self.orign_buffer + 1))
                y_range = range(max(0, origin[0] - self.orign_buffer), min(len(cropped_grid[0]), origin[0] + self.orign_buffer + 1))
                for x in x_range:
                    for y in y_range:
                        cropped_grid[x][y] = 0

                return cropped_grid, origin
