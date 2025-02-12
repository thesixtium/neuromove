from time import sleep
from pylsl import StreamInfo, StreamInlet, resolve_byprop, FOREVER

info = StreamInfo("MarkerStream", type='Markers')
stream = resolve_byprop("type", "Markers", timeout=FOREVER)[0]
inlet = StreamInlet(stream, processing_flags=0)

while True:
    samples, timestamps = inlet.pull_chunk(timeout=0.10)

    if len(samples) != 0 and len(timestamps) != 0:
        print(f"Got marker {samples} at timestamp {timestamps}")
    sleep(0.1)