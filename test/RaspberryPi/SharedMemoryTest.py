import unittest
import numpy as np

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

    def test_np_array(self):
        test_array = np.arange(12).reshape(3, 4)

        memory = SharedMemory("test3", 1000, create=True)
        memory.write_np_array(test_array)

        actual = memory.read_np_array()

        memory.close()

        for i in range(len(test_array)):
            for j in range(len(test_array[0])):
                self.assertEqual(test_array[i][j], actual[i][j])


if __name__ == '__main__':
    unittest.main()