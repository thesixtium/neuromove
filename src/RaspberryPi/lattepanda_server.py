import socket
from time import sleep

from src.RaspberryPi.InternalException import LattePandaError, LattePandaNotResponding
from src.RaspberryPi.SharedMemory import SharedMemory

#TODO: add elegant shutdown signal to Panda

def main():
    # Create a socket object
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # Use the actual IP address of the server device
    host = "10.0.0.56"  # Replace with your server's IP address
    port = 12345
    
    # Bind to the port
    server_socket.bind((host, port))
    server_socket.settimeout(1)
    
    # Start listening for incoming connections
    server_socket.listen(5)
    
    print(f"Server started on {host}:{port}")

    try:
        while True:
            try:
                # Establish a connection
                client_socket, addr = server_socket.accept()
                print(f"Got a connection from {addr}")
                
                # check for life every 1 second
                while True:
                    try:
                        data = client_socket.recv(1024).decode('ascii')
                        print(f"Received: {data}")
                        sleep(1)

                        if "Error" in data:
                            raise LattePandaError(data)
                        sleep(1)
                    except socket.timeout:
                        raise LattePandaNotResponding()

                # print("Sending stop signal to client")
                # client_socket.send('STOP'.encode("ascii"))
                
                # # Close the connection
                # client_socket.close()
            except socket.timeout:
                pass
    except KeyboardInterrupt:
        print("\nServer is shutting down...")

    finally:
        server_socket.close()

if __name__ == "__main__":
    main()