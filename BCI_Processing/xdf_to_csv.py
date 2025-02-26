import lsl_xdf_reader

filepath = "C:/Users/danij/OneDrive/Documents/CurrentStudy/sub-DEBUG/ses-S003/eeg"
streams = lsl_xdf_reader.parse_xdf(filepath)
print(streams)