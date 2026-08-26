# TinyBCI Runner

The TinyBCI runner demonstrates the full pipeline with a synthetic EEG producer.

## Build

```bash
cmake --build . --target tinybci_runner
```

## Usage

```bash
./bin/tinybci_runner [n_epochs] [mode]
```

| Argument | Description | Default |
|---|---|---|
| `n_epochs` | Epochs to collect before stopping. `0` = run until Ctrl+C. | `10` |
| `mode` | `triggered` or `sliding` | `triggered` |

## Examples

```bash
# Collect 10 epochs in triggered mode (P300)
./bin/tinybci_runner 10

# Run endlessly in triggered mode
./bin/tinybci_runner 0

# Collect 20 epochs in sliding mode (MI/SSVEP)
./bin/tinybci_runner 20 sliding

# Run endlessly in sliding mode
./bin/tinybci_runner 0 sliding
```

## Expected Output

**Triggered mode:**
```
TinyBCI Runner
  channels=8  srate=256 Hz  window=1000 ms  mode=triggered  target=10 epochs

Running — press Ctrl+C to stop

[Epoch 0] label=1  timestamp=1.805 s  frames=256  channels=8
[Epoch 1] label=1  timestamp=2.808 s  frames=256  channels=8
...
Done. Collected 10 / 10 epochs.
```

**Sliding mode:**
```
TinyBCI Runner
  channels=8  srate=256 Hz  window=1000 ms  mode=sliding  target=20 epochs

Running — press Ctrl+C to stop

[Epoch 0] label=1  timestamp=1.805 s  frames=256  channels=8
[Epoch 1] label=1  timestamp=2.406 s  frames=256  channels=8
[Epoch 2] label=1  timestamp=3.008 s  frames=256  channels=8
...
Done. Collected 20 / 20 epochs.
```

## Pipeline Parameters

| Parameter | Triggered | Sliding |
|---|---|---|
| Paradigm | P300 | MI |
| Pre-stimulus | 200 ms | 200 ms |
| Post-stimulus | 800 ms | 800 ms |
| Overlap | — | 400 ms |
| Step | — | 600 ms |
| Trigger interval | 1000 ms | 2000 ms |
| Trial duration | — | 3000 ms |
| Trial end code | — | 10 |