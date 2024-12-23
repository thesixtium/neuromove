import unittest
from venv import create

from src.RaspberryPi.SharedMemory import SharedMemory
from src.RaspberryPi.InternalException import DidNotCreateSharedMemory


class SharedMemoryTest(unittest.TestCase):

    def test_string_one_process(self):
        expected = "a"

        memory = SharedMemory("test", 8, create=True)
        memory.write_string(expected)

        actual = memory.read_string()

        memory.close()

        self.assertEqual(expected, actual)

    def test_string_two_process(self):
        expected = "abcde"

        memory1 = SharedMemory("test1", 8, create=True)
        memory2 = SharedMemory("test1", 8)

        memory1.write_string(expected)

        actual = memory2.read_string()

        memory1.close()
        memory2.close()

        self.assertEqual(expected, actual)

    def test_error_not_created(self):
        self.assertRaises(DidNotCreateSharedMemory, SharedMemory, "test2", 8)

    def test_occupancy_grid(self):
        occupancy_grid = [
            [0, 0, 1, 0, 0],
            [0, 1, 1, 1, 0],
            [0, 0, 1, 0, 0],
            [0, 0, 1, 0, 0],
            [0, 0, 0, 0, 0],
            [1, 0, 0, 0, 1],
        ]

        memory = SharedMemory("test3", 35, create=True)
        memory.write_grid(occupancy_grid)

        actual = memory.read_grid()

        memory.close()

        self.assertEqual(occupancy_grid, actual)


if __name__ == '__main__':
    unittest.main()