import lsl_xdf_reader

filepath = "C:/Users/danij/Documents/Capstone/neuromove/IDUN_P300_test/data/sub-NATHAN/ses-S001/eeg"
streams = lsl_xdf_reader.parse_xdf(filepath)
print(streams)