import lsl_xdf_reader

filepath = "D:/DATA/xdf/"
streams = lsl_xdf_reader.parse_xdf(filepath)
print(streams)