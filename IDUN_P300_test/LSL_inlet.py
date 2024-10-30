from pylsl import resolve_stream, StreamInlet
import numpy as np
import os
import time

# Data storage
subj = "003"  # UPDATE WITH EACH PARTICIPANT
directory = 'C:/Users/Adam Luoma/BCI4Kids/IDUN_Test'  # CHANGE DEPENDING ON COMPUTER
subdir = os.path.join(directory, 'data')
eeg_path = os.path.join(subdir, f'eeg_data_{subj}.csv')
marker_path = os.path.join(subdir, f'marker_data_{subj}.csv')

# Function to try resolving streams
def resolve_lsl_streams():
    print("Resolving EEG and Marker streams...")
    eeg_streams = resolve_stream('type', 'EEG')
    marker_streams = resolve_stream('type', 'Markers')
    return eeg_streams, marker_streams

# Initialize an empty list to store combined data
eeg_samples = []
eeg_timestamps = []
marker_samples = []
marker_timestamps = []

# Attempt to resolve streams
eeg_streams, marker_streams = resolve_lsl_streams()

# If streams are found, create inlets
if eeg_streams and marker_streams:
    eeg_inlet = StreamInlet(eeg_streams[0])
    print("EEG stream successfully found")
    marker_inlet = StreamInlet(marker_streams[0])
    print("Marker stream successfully found")

# Flag to control the loop
    streams_active = True

    # Collect and combine data
    while streams_active:
        try:
            # Pull EEG data
            eeg_sample, eeg_timestamp = eeg_inlet.pull_sample(timeout=1.0)
            eeg_samples.append(eeg_sample)
            eeg_timestamps.append(eeg_timestamp)
            
            # Check if the EEG stream has ended
            if eeg_sample is None:
                print("EEG stream ended.")
                streams_active = False
                break

            # Pull Marker data (check with timeout to avoid blocking)
            marker_sample, marker_timestamp = marker_inlet.pull_sample(timeout=0.0)
            marker_samples.append(marker_sample)
            marker_timestamps.append(marker_timestamp)
            

        except Exception as e:
            print(f"Error: {e}. Reconnecting...")
            time.sleep(2)  # Wait before attempting to reconnect
            eeg_streams, marker_streams = resolve_lsl_streams()
            if eeg_streams and marker_streams:
                eeg_inlet = StreamInlet(eeg_streams[0])
                marker_inlet = StreamInlet(marker_streams[0])
            else:
                print("Unable to reconnect. Ending data collection.")
                streams_active = False
                break

else:
    print("No streams found in LSL INLET SCRIPT Exiting.")

# Clean and Save
cleaned_eeg_samples = [sample[0] for sample in eeg_samples if sample is not None]
cleaned_eeg_samples_array = np.array(cleaned_eeg_samples)
eeg_timestamps_array = np.array(eeg_timestamps[:len(cleaned_eeg_samples)])
eeg_data_matrix = np.vstack([cleaned_eeg_samples_array, eeg_timestamps_array])
np.savetxt(eeg_path, eeg_data_matrix.T, delimiter=',')

# Filter out None values and keep only the markers [1] and [2] with their corresponding timestamps
filtered_markers_and_timestamps = [
    (sample[0], timestamp) for sample, timestamp in zip(marker_samples, marker_timestamps)
    if sample is not None and isinstance(sample, list) and sample[0] in [1, 2, 3, 4, 5, 6, 7, 8, 9]
]

if not filtered_markers_and_timestamps:
    print("No markers [1]-[9] found in the data.")
else:
    # Separate the filtered markers and timestamps
    cleaned_marker_samples, cleaned_marker_timestamps = zip(*filtered_markers_and_timestamps)
    
    # Convert the cleaned markers and timestamps to numpy arrays
    cleaned_marker_samples_array = np.array(cleaned_marker_samples).reshape(-1, 1)  # Reshape to (N, 1)
    cleaned_marker_timestamps_array = np.array(cleaned_marker_timestamps).reshape(-1, 1)  # Already (N, 1)
    
    # Stack the marker samples and their corresponding timestamps horizontally
    marker_data_matrix = np.hstack([cleaned_marker_samples_array, cleaned_marker_timestamps_array])
    
    # Save the data matrix to a CSV file
    np.savetxt(marker_path, marker_data_matrix, delimiter=',')


    
