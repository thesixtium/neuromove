import random
from pylsl import StreamInfo, StreamOutlet, local_clock
import time
from psychopy import visual, core, event as psychopy_event

from typing import List, Tuple

# user-defined variables
USE_UNIX_OFFSET = False  # set to True if using IDUN, False otherwise
NUMBER_OF_BLOCKS = 3    # how many times to repeat the experiment
TRIALS_PER_BLOCK = 20   # how many cycles of flashes back to back
SQUARE_COLOR = 'white'    # color of the square when flashed
FLASHED_TEXT_COLOR = 'black'  # color of the text when square is flashed
FLASH_DURATION = 0.1    # duration of the flash in seconds
INTER_FLASH_MIN = 0.05  # minimum time between flashes in seconds
INTER_FLASH_MAX = 0.2  # maximum time between flashes in seconds
INTER_CYCLE_MIN = 0.05  # minimum time between cycles in seconds
INTER_CYCLE_MAX = 0.2  # maximum time between cycles in seconds

def init() -> Tuple[visual.Window, StreamOutlet]:
    # create psychopy window
    window = visual.Window([800, 600], color='black')

    start_text = visual.TextStim(window, text="Press Enter to start", color='white', height=0.1, pos=(0, 0.85))

    start_text.draw()
    window.flip()

    # create LSL stream
    marker_info = StreamInfo(name='MarkerStream',
                         type='Markers',
                         channel_count=1,
                         nominal_srate=250,
                         channel_format='string',
                         source_id='Marker_Outlet')
    marker_outlet = StreamOutlet(marker_info, 20, 360)  

    # wait for user to press Enter
    while True:
        keys = psychopy_event.getKeys()
        if 'return' in keys:
            start_countdown(window, 3)
            start_timestamp = get_timestamp()
            marker_outlet.push_sample(["Program Start"], start_timestamp)
            break 

        elif 'escape' in keys:
            window.close()
            return

    return window, marker_outlet

def start_countdown(window: visual.Window, countdown: int):
    countdown_text = visual.TextStim(window, text=str(countdown), color='white', height=0.1, pos=(0, 0))
    for i in range(countdown, 0, -1):
        countdown_text.text = str(i)
        countdown_text.draw()
        window.flip()
        core.wait(1)

def get_timestamp():
    if USE_UNIX_OFFSET:
        unix_offset = time.time() - local_clock()
        return local_clock() + unix_offset
    else:
        return local_clock()

def run_block(window: visual.Window, lsl_outlet: StreamOutlet, squares: List[visual.Rect], labels: List[visual.TextStim]):
    previous_cycle = None

    for trial in range(TRIALS_PER_BLOCK):
        # randomly order the 9 squares
        cycle = random.sample(range(9), 9)

        # ensure end of previous trial is not the same as start of this trial
        if previous_cycle:
            while cycle[0] == previous_cycle[-1]:
                cycle = random.sample(range(9), 9)

        for i in cycle:
            squares[i].fillColor = SQUARE_COLOR
            labels[i].color = FLASHED_TEXT_COLOR
            for square in squares:
                square.draw()
            for label in labels:
                label.draw()

            window.flip()

            # mark start of flash
            timestamp = get_timestamp()
            print(timestamp)
            lsl_outlet.push_sample([f"{chr(i+65)}"], timestamp)

            # wait 
            core.wait(FLASH_DURATION)

            # return to grey
            squares[i].fillColor = 'grey'
            labels[i].color = 'white'
            for square in squares:
                square.draw()
            for label in labels:
                label.draw()
            window.flip()

            # wait for random time between flashes
            core.wait(random.uniform(INTER_FLASH_MIN, INTER_FLASH_MAX))

        # mark end of trial
        timestamp = get_timestamp()
        lsl_outlet.push_sample([f"End of Trial #{trial}"], timestamp)

        print(f"End of Trial {trial}")

        # update previous cycle
        previous_cycle = cycle

        # wait for random time between trials
        core.wait(random.uniform(INTER_CYCLE_MIN, INTER_CYCLE_MAX))  

def run_experiment(window: visual.Window, lsl_outlet: StreamOutlet, squares: List[visual.Rect], labels: List[visual.TextStim]):
    # mark start of experiment
    timestamp = get_timestamp()
    lsl_outlet.push_sample(["Experiment Start"], timestamp)

    for block in range(NUMBER_OF_BLOCKS):
        run_block(window, lsl_outlet, squares, labels)

        # mark end of block
        timestamp = get_timestamp()
        lsl_outlet.push_sample([f"End of Block {block+1}"], timestamp)
        print(f"Block {block+1} complete")

        # wait for user to to say they're ready
        wait_label = visual.TextStim(window, text="Press Enter when ready for next block", color='white', height=0.1, pos=(0, 0))
        wait_label.draw()
        window.flip()

        if block >= NUMBER_OF_BLOCKS - 1:
            break

        while True:
            keys = psychopy_event.getKeys()
            if 'return' in keys:
                break
            elif 'escape' in keys:
                window.close()
                return
        
    # mark end of experiment
    timestamp = get_timestamp()
    lsl_outlet.push_sample(["Experiment End"], timestamp)

def main():
    window, lsl_outlet = init()
    
    # positions for 3x3 grid
    positions = [(-0.5, 0.5), (0, 0.5), (0.5, 0.5),
             (-0.5, 0), (0, 0), (0.5, 0),
             (-0.5, -0.5), (0, -0.5), (0.5, -0.5)]
    
    # create 3x3 grid of squares
    squares = [visual.Rect(window, width=0.4, height=0.4, pos=pos, fillColor='grey') for pos in positions]
    labels = [visual.TextStim(window, text=chr(i + 65), color='white', height=0.1, pos=pos) for i, pos in enumerate(positions)]

    # draw squares and labels
    for square in squares:
        square.draw()
    for label in labels:
        label.draw()

    window.flip()

    while True:
        run_experiment(window, lsl_outlet, squares, labels)

        restart_label = visual.TextStim(window, text="Press Enter to run experiment again", color='white', height=0.1, pos=(0, 0))
        restart_label.draw()
        window.flip()

        while True:
            keys = psychopy_event.getKeys()
            if 'escape' in keys:
                window.close()
                break
            elif 'return' in keys:
                break

        
    

if __name__ == "__main__":
    main()