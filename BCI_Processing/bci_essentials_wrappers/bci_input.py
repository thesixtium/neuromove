from lib.bci_essentials.bci_essentials.io.xdf_sources import XdfEegSource, XdfMarkerSource, load_xdf_stream
import numpy as np


#TODO: implement logger
class BCIEssentialsInput(XdfMarkerSource):
    '''
    This class is used to feed markers into bci_essentials. It is a wrapper around MarkerSource to allow for easy
    integration with existing data collected.
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

        raw_samples = self.__samples
        print(f"sample size: {len(raw_samples)}")

        # convert to form that Bessy needs
        new_markers = []
        new_timestamps = []
        current_target = 0
        for i in range(len(raw_samples)):
            s = raw_samples[i]
            new_timestamps.append(self.__timestamps[i])

            if "Experiment Start" in s[0]:
                new_markers.append(["Trial Started"])
            elif "End of Block" in s[0]:
                new_markers.append(["Trial Ends"])

                # add new timestamp with trial started
                end_timestamp = self.__timestamps[i]
                next_timestamp = self.__timestamps[i+1]

                marker_timestamp = round((end_timestamp + next_timestamp) / 2)
                new_timestamps.append(marker_timestamp)
                new_markers.append(["Trial Started"])

                current_target += 1
            elif len(s[0]) == 1:
                # one of the letters
                flashed = ord(s[0]) - 65

                new_marker = f"p300,s,9,{current_target},{flashed}"
                # new_marker = ['p300', 's', 9, current_target, flashed]
                new_markers.append([new_marker])
                
            else:
                # one of the ignored cases
                new_timestamps.pop()

        # add training complete marker to trigger training model
        new_timestamps.append(new_timestamps[-1])
        new_markers.append(["Training Complete"])

        self.__samples = [[]]
        self.__timestamps = []
        return [new_markers, new_timestamps]