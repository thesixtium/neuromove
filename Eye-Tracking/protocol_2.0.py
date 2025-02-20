import cv2
import numpy as np
import time
from datetime import datetime

current_datetime = datetime.now().strftime("%Y-%m-%d %H-%M-%S")
str_current_datetime = str(current_datetime)

# Open a maximized window for the grid
cv2.namedWindow('Grid', cv2.WND_PROP_FULLSCREEN)
cv2.setWindowProperty('Grid', cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_FULLSCREEN)

# Calculate the frame rate of the camera
cap = cv2.VideoCapture(0)
frame_rate = cap.get(cv2.CAP_PROP_FPS)
print(frame_rate)
cap.release()

# Set the frame rate of the video file to match the frame rate of the camera
fourcc = cv2.VideoWriter_fourcc(*'mp4v')
out = cv2.VideoWriter(str_current_datetime + '.mp4', fourcc, frame_rate, (640, 480))

# Open the default camera
cap = cv2.VideoCapture(0)

row = 0
col = 0
last_move_time = time.time()
record_flag = True
black_screen_time = None
black_screen_duration = 240
cont = 0
black_screen_shown = False

while True:
    # Capture frame-by-frame from the camera
    ret, frame = cap.read()
    if not ret:
        break

    # Create a black background for the grid
    grid_frame = np.zeros((480, 640, 3), dtype=np.uint8)

    current_time = time.time()

    # Move to the next square every 3 seconds
    if current_time - last_move_time >= 30:
        col = (col + 1) % 3
        if col == 0:
            row = (row + 1) % 3
        last_move_time = current_time
        cont += 1

    # Highlight the square at the current row and col
    cell_width = 640 // 3
    cell_height = 480 // 3
    top_left_x = col * cell_width
    top_left_y = row * cell_height
    bottom_right_x = (col + 1) * cell_width
    bottom_right_y = (row + 1) * cell_height
    grid_frame = cv2.rectangle(grid_frame, (top_left_x, top_left_y), (bottom_right_x, bottom_right_y), (250, 250, 250), -1)

    # Display the grid in the 'Grid' window
    cv2.imshow('Grid', grid_frame)

    # Record the frame if the flag is set
    if record_flag:
        out.write(frame)

    # Check if all positions have been highlighted
    if cont >= 9 and black_screen_time is None:
        black_screen_time = current_time  # Start the black screen timer

    # Display black screen if needed
    if black_screen_time is not None:
        if current_time - black_screen_time <= black_screen_duration:
            grid_frame = np.zeros_like(grid_frame)  # Black screen
            cv2.imshow('Grid', grid_frame)
        else:
            break  # Exit the loop after black screen duration is over

    # Check for the key press to stop recording
    key = cv2.waitKey(1) & 0xFF
    if key == ord('q'):
        break

# Release the VideoWriter and camera objects
out.release()
cap.release()
cv2.destroyAllWindows()
