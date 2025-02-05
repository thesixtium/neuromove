import unittest

from src.RaspberryPi import point_selection

class PointSelectionTest(unittest.TestCast):
    def test_occupancy_grid_to_points(self):
        data_str = str([[0, 1, 0], [0, 0, 1], [1, 0, 0]])
        result = occupancy_grid_to_points(input_data=data_str, number_of_neighbourhoods=2, number_of_points_per_neighbourhood=2)
        self.assertEqual(len(result), 4)
        self.assertIsInstance(result[0], np.ndarray)
        self.assertIsInstance(result[1], np.ndarray)
        self.assertIsInstance(result[2], np.ndarray)
        self.assertIsInstance(result[3], tuple)

    def test_invalid_data_shape(self):
        data_str = str([0, 1, 1, 1, 0])
        with self.assertRaises(ValueError):
            occupancy_grid_to_points(input_data = data_str)

    def test_invalid_neighbourhoods(self):
        data_str = str([[0, 1, 0], [0, 0, 1], [1, 0, 0]])
        with self.assertRaises(ValueError):
            occupancy_grid_to_points(input_data=data_str, number_of_neighbourhoods=0)

    def test_invalid_points_per_neighbourhood(self):
        data_str = str([[0, 1, 0], [0, 0, 1], [1, 0, 0]])
        with self.assertRaises(ValueError):
            occupancy_grid_to_points(input_data=data_str, number_of_points_per_neighbourhood=0)

    def test_insufficient_reachable_points(self):
        data_str = str([[1, 1, 1], [1, 0, 1], [1, 1, 1]])
        with self.assertRaises(Exception):
            occupancy_grid_to_points(input_data=data_str, number_of_neighbourhoods=2)


if __name__ == '__main__':
    unittest.main()