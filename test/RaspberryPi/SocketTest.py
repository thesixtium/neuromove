import json
import unittest

from src.RaspberryPi.InternalException import InvalidSocketExpectedType, CantConvertSocketData
from src.RaspberryPi.Socket import Socket


class SocketTest(unittest.TestCase):

    def test_string(self):
        sender = Socket(12344, 12345)
        receiver = Socket(12345, 12344)

        send = "test"
        sender.send(send)
        data = receiver.receive(1000)

        sender.close()
        receiver.close()

        self.assertEqual(send, data)

    def test_error_wrong_expected_type(self):
        sender = Socket(12346, 12347)
        receiver = Socket(12347, 12346)

        send = "test"
        sender.send(send)

        self.assertRaises(InvalidSocketExpectedType, receiver.receive, 1000, None)

        sender.close()
        receiver.close()

    def test_error_wrong_type(self):
        sender = Socket(12346, 12347)
        receiver = Socket(12347, 12346)

        send = "test"
        sender.send(send)

        self.assertRaises(CantConvertSocketData, receiver.receive, 1000, float)

        sender.close()
        receiver.close()

    def test_int(self):
        sender = Socket(12348, 12349)
        receiver = Socket(12349, 12348)

        send = 1
        sender.send(send)
        data = receiver.receive(1000, expected_type=int)

        sender.close()
        receiver.close()

        self.assertEqual(send, data)

    def test_negative_int(self):
        sender = Socket(12350, 12351)
        receiver = Socket(12351, 12350)

        send = -3
        sender.send(send)
        data = receiver.receive(1000, expected_type=int)

        sender.close()
        receiver.close()

        self.assertEqual(send, data)

    def test_float(self):
        sender = Socket(12352, 12353)
        receiver = Socket(12353, 12352)

        send = 1.1
        sender.send(send)
        data = receiver.receive(1000, expected_type=float)

        sender.close()
        receiver.close()

        self.assertEqual(send, data)

    def test_negative_float(self):
        sender = Socket(12354, 12355)
        receiver = Socket(12355, 12354)

        send = -3.8
        sender.send(send)
        data = receiver.receive(1000, expected_type=float)

        sender.close()
        receiver.close()

        self.assertEqual(send, data)

    def test_dict(self):
        sender = Socket(12356, 12357)
        receiver = Socket(12357, 12356)

        send = "{'muffin' : 'lolz', 'foo' : 'kitty'}"
        sender.send(send)
        data = receiver.receive(1000, expected_type=dict)

        sender.close()
        receiver.close()

        self.assertEqual(eval(send), data)

    def test_json(self):
        sender = Socket(12358, 12359)
        receiver = Socket(12359, 12358)

        send = json.loads('{"http://example.org/about": {"http://purl.org/dc/terms/title": [{"type": "literal", "value": "Annas Homepage"}] } }')
        sender.send(send)
        data = receiver.receive(1000, expected_type="json")

        sender.close()
        receiver.close()

        self.assertEqual(send, data)

if __name__ == '__main__':
    unittest.main()