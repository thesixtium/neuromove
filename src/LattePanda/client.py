import socket
import subprocess
import threading
import time
from os.path import join

'''
NOTE: DSI Streamer and DSI2LSL must be present in the LattePanda folder
Ignored as the software is not open source, so each new machine must install to this directory

DSI Streamer must be v1.08.119 in a folder called DSI_Streamer
DSI2LSL must be in a folder called dsi2lsl
'''

RASPBERRY_PI_PORT = 12345   # port to connect to on the Raspberry Pi
DSI_PORT = "COM3"   # COM port of the DSI7
SERVER_IP_ADDR = "10.0.0.56"    # local IP address of the Raspberry Pi # TODO: figure out static addressing

def init_socket():
    # Create socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((SERVER_IP_ADDR, RASPBERRY_PI_PORT))
 
    return client_socket

def run_dsi_streamer():
    # DSI Streamer (for headset impedance checking)
    try:
        dsi_streamer_path = join("DSI_Streamer", "DSI-Streamer-v.1.08.119.exe")
        subprocess.run([dsi_streamer_path], check = True)
    except subprocess.CalledProcessError as e:
        print(f"DSI Streamer failed with error code {e.returncode}")
        raise Exception(f"Error running DSI Streamer: {e}")
    print("DSI Streamer closed")

def run_dsi2lsl():
    # DSI2LSL
    try:
        dsi2lsl_path = join("dsi2lsl", "dsi2lsl.exe")
        subprocess.run([dsi2lsl_path, f'port={DSI_PORT}', 'lsl-stream-name=DSI7', 'montage=F4,C4,S1,S3,C3,F3'], check=True)
    except subprocess.CalledProcessError as e:
        print(f"DSI2LSL failed with error code {e.returncode}")
        raise Exception(f"Error running DSI2LSL: {e}")
    
def send_ping(socket: socket.socket, stop_event: threading.Event):
    while not stop_event.set():
        try:
            socket.sendall(b'ping')
            time.sleep(1)
        except Exception as e:
            print(f"Error sending ping: {e}")
            break

def listen_to_pi(socket: socket.socket, stop_event: threading.Event):
    while not stop_event.set():
        try:
            data = socket.recv(1024).decode('ascii')
            print(f"Received: {data}")
            if "STOP" in data:
                stop_event.set()
                print("Stop signal received from Pi")
        except Exception as e:
            print(f"Error receiving data: {e}")
            break

def main():
    socket = init_socket()
    stop_event= threading.Event()

    # ping Pi
    ping_thread = threading.Thread(target=send_ping, args=(socket, stop_event,))
    ping_thread.daemon = True
    ping_thread.start()

    # listen to Pi
    listen_thread = threading.Thread(target=listen_to_pi, args=(socket, stop_event,))
    listen_thread.daemon = True
    listen_thread.start()

    # start DSI Streamer
    try:
        run_dsi_streamer()
    except Exception as e:
        error_msg = f"DSI Streamer failed: {e}"
        print(error_msg)
        socket.send(error_msg.encode('ascii'))

    # start DSI2LSL
    try:
        run_dsi2lsl()
    except Exception as e:
        error_msg = f"DSI2LSL failed: {e}"
        print(error_msg)
        socket.send(error_msg.encode('ascii'))

    listen_thread.join()
    print("shutting down...")

    socket.close()

if __name__ == "__main__":
    main()