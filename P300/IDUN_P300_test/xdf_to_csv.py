import lsl_xdf_reader

filepath = "C:/Users/danij/Documents/neuromove/IDUN_P300_test/data/a"
streams = lsl_xdf_reader.parse_xdf(filepath)
print(streams)