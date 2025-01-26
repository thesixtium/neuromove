from lib.bci_essentials.bci_essentials.io.xdf_sources import XdfEegSource, XdfMarkerSource, load_xdf_stream
import numpy as np

TRAIN_PORTION = 0.85
NUM_PER_TRIAL = 20
TARGET_START = 0
TARGET_INCREASING = True
NUM_TARGETS = 9


#TODO: implement logger
class OldXdfFormatInput(XdfMarkerSource):
    '''
    This class is used to feed markers into bci_essentials. It is a wrapper around MarkerSource to allow for easy
    integration with existing data collected.

    It is **very** hardcoded, taking in data from our old XDF file format and updating to be compatible with Bessy
    '''

    def __init__(self, filename):
        samples, timestamps, info = load_xdf_stream(filename, "Markers")
        self.__samples = samples
        self.__timestamps = timestamps
        self.__info = info


    def get_markers(self) -> tuple[list[list], list]:
        '''
        Read XDF data. Return contents on the first call, otherwise return empty lists
        '''

        # return empty lists on all future iterations
        if self.__samples == [[]]:
            return [[[]], []]

        markers = self.__samples
        timestamps = self.__timestamps
        print(f"sample size: {len(markers)}")

        new_markers = []
        new_timestamps = []
        validation_markers = []
        validation_timestamps = []
        current_target = TARGET_START
        i = 2 # ignore "Program start" & "Experiment Start"

        new_markers.append(["Trial Started"])
        new_timestamps.append(timestamps[1])

        while i < len(markers):
            m = markers[i]
            t = timestamps[i]

            if "End of Block" in m[0]:
                new_markers.append(["Trial Ends"])
                new_timestamps.append(timestamps[i])
                current_target = current_target + (1 * TARGET_INCREASING)

                next_timestamp = timestamps[i+1]
                middle_timestamp = (t + next_timestamp) / 2
                new_timestamps.append(middle_timestamp)
                new_markers.append(['Trial Started'])

                i += 1

            elif len(m[0]) == 1: # letter marker
                # training data
                for _ in range(round(NUM_PER_TRIAL * TRAIN_PORTION) * (NUM_TARGETS + 1)):
                    if "End of Trial" not in markers[i][0]:
                        flashed = ord(markers[i][0]) -65
                        new_marker = f"p300,s,{NUM_TARGETS},{current_target},{flashed}"
                        new_markers.append([new_marker])
                        new_timestamps.append(timestamps[i])
                    i += 1

                # validation data
                validation_markers.append(["Trial Started"])
                validation_timestamps.append((timestamps[i] + timestamps[i+1]) / 2)
                for _ in range(round(NUM_PER_TRIAL * (1 - TRAIN_PORTION)) * (NUM_TARGETS + 1)):
                    if "End of Trial" not in markers[i][0]:
                        flashed = ord(markers[i][0]) -65
                        new_marker = f"p300,s,{NUM_TARGETS},-1,{flashed}"
                        validation_markers.append([new_marker])
                        validation_timestamps.append(timestamps[i])
                    i += 1

                validation_markers.append(["Trial Ends"])
                middle_timestamp = (timestamps[i] + timestamps[i-1]) / 2
                validation_timestamps.append(middle_timestamp)

            else: 
                i += 1

        # pop last "Trial started"
        new_markers.pop()
        new_timestamps.pop()

        # add "Training complete"
        new_timestamps.append(new_timestamps[-1])
        new_markers.append(["Training Complete"])

        # add validation data 
        new_markers += validation_markers
        new_timestamps += validation_timestamps
        # # convert to form that Bessy needs
        # new_markers = []
        # new_timestamps = []
        # current_target = 0
        # for i in range(len(markers)):
        #     s = markers[i]
        #     new_timestamps.append(self.__timestamps[i])

        #     if "Experiment Start" in s[0]:
        #         new_markers.append(["Trial Started"])
        #     elif "End of Block" in s[0]:
        #         new_markers.append(["Trial Ends"])

        #         # add new timestamp with trial started
        #         end_timestamp = self.__timestamps[i]
        #         next_timestamp = self.__timestamps[i+1]

        #         marker_timestamp = (end_timestamp + next_timestamp) / 2
        #         new_timestamps.append(marker_timestamp)
        #         new_markers.append(["Trial Started"])

        #         current_target += 1
        #     elif len(s[0]) == 1:
        #         # one of the letters
        #         flashed = ord(s[0]) - 65

        #         new_marker = f"p300,s,9,{current_target},{flashed}"
        #         # new_marker = ['p300', 's', 9, current_target, flashed]
        #         new_markers.append([new_marker])
                
        #     else:
        #         # one of the ignored cases
        #         new_timestamps.pop()

        # # add training complete marker to trigger training model
        # new_timestamps.append(new_timestamps[-1])
        # new_markers.append(["Training Complete"])

        # # add simulated validation data
        # validation_samples = self.__generate_validation_data(new_markers, )

        # new_markers += validation_samples
        # new_timestamps += new_timestamps

        # # remove extra "training complete" & "trial started"
        # new_markers.pop()
        # new_timestamps.pop()
        # new_markers.pop()
        # new_timestamps.pop()

        # self.__samples = [[]]
        # self.__timestamps = []
        return [new_markers, new_timestamps]
    
    def __generate_validation_data(self, markers: list) -> list:
        # replace al targets with -1
        new_markers = []

        for m in markers:
            if "p300,s" in m[0]:
                parts = m[0].split(',')
                parts[3] = '-1'

                new_marker = ','.join(parts)
                new_markers.append([new_marker])
            else:
                new_markers.append(m)

        return new_markers
