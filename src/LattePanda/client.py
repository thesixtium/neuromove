import socket

def start_client():
    # Create a socket object
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # Get local machine name
    host = "10.0.0.237"
    port = 12345
    
    # Connection to hostname on the port
    client_socket.connect((host, port))
    
    # Receive no more than 1024 bytes
    message = client_socket.recv(1024)
    
    print(message.decode('ascii'))
    
    # Close the connection
    client_socket.close()

if __name__ == "__main__":
    start_client()