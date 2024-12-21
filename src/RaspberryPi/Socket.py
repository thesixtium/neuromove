import socket
import json
from src.RaspberryPi.InternalException import InvalidSocketExpectedType, CantLoadSocketJSON, CantConvertSocketData

class Socket:

    def __init__(self, port: int, to_port: int, ip="127.0.0.1"):
        self.to_port = to_port
        self.ip = ip

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((self.ip, port))

    def receive(self, message_size: int, expected_type=str):
        data, addr = self.sock.recvfrom(message_size)

        try:
            if expected_type == str:
                return data.decode()
            if expected_type == int:
                return int(data.decode())
            if expected_type == float:
                return float(data.decode())
            if expected_type == dict:
                return eval(data.decode())
            if expected_type == "json":
                data = data.decode()
                try:
                    return json.loads(data.replace("'", '"'))
                except Exception:
                    raise CantLoadSocketJSON(data)
            else:
                raise InvalidSocketExpectedType(expected_type)
        except Exception as e:
            if isinstance(e, InvalidSocketExpectedType):
                raise
            raise CantConvertSocketData(data, expected_type)

    def send(self, message):
        self.sock.sendto(str(message).encode(), (self.ip, self.to_port))

    def close(self):
        self.sock.close()