import random
from pylsl import StreamInfo, StreamOutlet, local_clock
import time
from psychopy import visual, core, event as psychopy_event

num_squares = 9
num_cycles = 200
unix_offset = time.time() - local_clock()
print("unix_offset:", unix_offset)

marker_info = StreamInfo(name='MarkerStream',
                         type='Markers',
                         channel_count=1,
                         nominal_srate=250,
                         channel_format='string',
                         source_id='Marker_Outlet')
marker_outlet = StreamOutlet(marker_info, 20, 360)

def get_timestamp():
    # return time.time()
    return local_clock() + unix_offset

def start_window():
    window = visual.Window([800, 600], color='black')

    start_text = visual.TextStim(window, text="Press Enter to start", color='white', height=0.1, pos=(0, 0.85))
    start_text.draw()
    window.flip()

    start_timestamp = get_timestamp()
    marker_outlet.push_sample(["Experiment started"], start_timestamp)

    while True:
        keys = psychopy_event.waitKeys()
        if 'return' in keys:
            # countdown from 3
            for i in range(3, 0, -1):
                countdown = visual.TextStim(window, text=str(i), color='white', height=0.1, pos=(0, 0))
                countdown.draw()
                window.flip()
                core.wait(1)
            break
        elif 'escape' in keys:
            window.close()
            core.quit()
    
    # positions for the 3x3 grid
    positions = [(-0.5, 0.5), (0, 0.5), (0.5, 0.5),
             (-0.5, 0), (0, 0), (0.5, 0),
             (-0.5, -0.5), (0, -0.5), (0.5, -0.5)]

    # create 3x3 grid of squares and labels
    squares = [visual.Rect(window, width=0.4, height=0.4, pos=pos, fillColor='grey') for pos in positions]
    labels = [visual.TextStim(window, text=chr(i + 65), color='white', height=0.1, pos=pos) for i, pos in enumerate(positions)]

    for square in squares:
        square.draw()
    for label in labels:
        label.draw()

    window.flip()
    prev_cycle = None

    for num in range(num_cycles):
        # randomly order the 9 squares
        cycle = random.sample(range(num_squares), num_squares)

        # ensure that the same square doesn't appear in consecutive cycles
        if prev_cycle:
            while cycle[0] == prev_cycle[-1]:
                cycle = random.sample(range(num_squares), num_squares)
    
        for i in cycle:
            check_close(window)

            # flash for 100ms
            squares[i].fillColor = 'red'
            for square in squares:
                square.draw()
            for label in labels:
                label.draw()

            window.flip()

            # mark start of flash
            timestamp = get_timestamp()
            marker_outlet.push_sample([f"{chr(i+65)}"], timestamp)
            # print(f"Flashing square {chr(i+65)} at {timestamp}")
            if start_timestamp is None:
                start_timestamp = timestamp

            # wait 100ms
            core.wait(0.1)

            # return to grey for 100ms
            squares[i].fillColor = 'grey'
            for square in squares:
                square.draw()
            for label in labels:
                label.draw()
            window.flip()
            
            # wait between 50-200ms
            wait_time = random.uniform(0.05, 0.2)
            core.wait(wait_time)

            # do we need to mark end of flash?

        prev_cycle = cycle

        # mark end of cycle
        timestamp = get_timestamp()
        marker_outlet.push_sample([f"Cycle {num + 1}"], timestamp)
        
        # wait 200ms between cycles
        core.wait(0.2)
        print(f"Cycle #{num + 1}")

    # mark end of experiment
    end_timestamp = get_timestamp()
    marker_outlet.push_sample(["Experiment ended"], end_timestamp)
    print(f"Experiment started at {start_timestamp} and ended at {end_timestamp}. Duration of {end_timestamp - start_timestamp} seconds.")

    finished_text = visual.TextStim(window, text="Finished", color='white', height=0.1, pos=(0, 0))
    finished_text.draw()
    window.flip()

    # After all blocks are complete, send the stop signal
    with open("stop_signal.txt", "w") as stop_file:
        stop_file.write("STOP")
    print("Stop signal sent.")

    while True:
        check_close(window)

def check_close(window):
    if 'escape' in psychopy_event.getKeys():
        window.close()
        core.quit()


if __name__ == "__main__":
    start_window()
