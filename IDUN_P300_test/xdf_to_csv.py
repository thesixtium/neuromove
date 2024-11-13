import lsl_xdf_reader

filepath = "C:/Users/danij/Documents/Capstone/neuromove/IDUN_P300_test/data/sub-Mom/ses-S002/eeg"
streams = lsl_xdf_reader.parse_xdf(filepath)
print(streams)