# TinyBCI — Architecture & Design Decisions

**Authors:** Michele Romani ([@BRomans](https://github.com/BRomans))
**Version:** 0.5.0
**Status:** In development

---

## Overview

TinyBCI is a lightweight C library for real-time BCI (Brain-Computer Interface) signal processing. It is designed to run on two distinct target environments:

- **Embedded hardware** — bare-metal MCUs (e.g. ARM Cortex-M) and single-board computers (e.g. Raspberry Pi), where memory and compute are constrained
- **Desktop DLL** — loaded into Unity or Unreal Engine for game-based BCI applications

All architectural decisions balance these two targets. Where they conflict, embedded constraints take precedence.

---

## Pipeline Overview

Data flows through a directed acyclic graph (DAG) of macro-group nodes. Two input streams — a continuous EEG signal and sparse trigger events — are produced externally by a `TBCI_Producer`, routed through the input gateway, and consumed by the pipeline. Every macro group exposes a single uniform `tick_fn(self, ctx)` entry point — `tbci_context_tick` iterates `ctx->nodes[]` calling `tick_fn` with no type dispatch.

```
┌─────────────────────────────────────────────────────────────────┐
│                        PRODUCER LAYER                            │
│            TBCI_Producer { init, tick, reset, close }            │
│   SyntheticProducer, UnicornProducer, LSLProducer, (file, ...)   │
│   optional TBCI_TriggerGenerator attached for synthetic triggers │
└───────────────────────────────┬─────────────────────────────────┘
                                │  producer->tick(producer, inputs, ctx)
                                ▼
                   ┌────────────────────────┐
                   │      TBCI_Input        │  ← unified input gateway
                   │  signal buffer         │  classifies triggers [1-191] vs
                   │  trigger queue         │  commands [192-255], routes EEG,
                   │  gateway config/state  │  dispatches state transitions
                   └────────────┬───────────┘
                                │  ctx->inputs->signal (raw)
                                ▼
         ┌────────────────────────────────────┐
         │       PreprocessingGroup           │  ← TBCI_Node.tick_fn = pp_process
         │   TBCI_NodeGroup { inner nodes }   │    ALWAYS runs — copies raw frame
         │   Notch → Bandpass → ...           │    to processed_signal unconditionally.
         │   (filtering_enabled gates only    │    use_preprocessing gates only the
         │    the inner transform chain)      │    inner-node transform chain.
         └───────────────┬────────────────────┘
                         │  ctx->processed_signal
                         ▼
         ┌────────────────────────────────────┐
         │    CoreNode (Sync + Seg)           │  ← TBCI_Node.tick_fn = cn_process
         │                                    │    mandatory, auto-registered
         │  ↓ RawOutNode (optional)           │    by tbci_build_pipeline
         │    writes raw EEG + triggers       │    enabled via config.log_enabled
         │    to CSV between sync and seg     │
         │                                    │
         │  SEG_MODE_TRIGGERED (P300)         │
         │    trigger → pre+post window       │
         │    → TBCI_Epoch                    │
         │                                    │
         │  SEG_MODE_SLIDING (MI/SSVEP)       │
         │    trial start → overlapping       │
         │    windows → TBCI_Epoch[]          │
         └───────────────┬────────────────────┘
                         │
                         ▼
         ┌────────────────────────────────────┐
         │         TBCI_EpochQueue            │  ← segmentation/classifier boundary
         │   fixed-capacity FIFO              │    enables parallelism on multi-core
         │   caller-provided sample pool      │
         └───────────────┬────────────────────┘
                         │
                         ▼
         ┌────────────────────────────────────┐
         │      FeatureExtractionGroup        │  ← TBCI_Node.tick_fn = fe_process
         │   TBCI_NodeGroup { inner nodes }   │    mandatory time-major→channel-major
         │   Transpose → CCA → CSP → ...      │    transpose before inner chain.
         │                                    │    reads epoch_queue, writes
         │                                    │    to features_queue
         └───────────────┬────────────────────┘
                         │  ctx->features_queue
                         ▼
         ┌────────────────────────────────────┐
         │         TBCI_Decoder               │  ← TBCI_Node.tick_fn = dc_process
         │   TBCI_NodeGroup { inner nodes }   │    state-driven dispatch:
         │   TBCI_Model instances             │    TRAINING → train()
         │   e.g. CCAModel, LDAModel, ONNX   │    INFERENCE → infer() + push
         │                                    │    IDLE → pending
         │                                    │    passthrough when disabled
         └───────────────┬────────────────────┘
                         │  ctx->output_queue
                         ▼
         ┌────────────────────────────────────┐
         │         TBCI_EpochQueue            │  ← output boundary
         │   (output_queue)                   │    optional, NULL valid
         │   caller-provided sample pool      │    when decoder disabled
         └───────────────┬────────────────────┘
                         │
                         ▼
                   ┌─────────────┐
                   │ LSL Writer  │  ← external consumer, not a DAG node
                   │ File/CB     │    reads output_queue in runner loop
                   └─────────────┘
```

---

## Trigger Protocol

Trigger codes are single bytes classified by value:

| Range | Decimal | Type | Description |
|---|---|---|---|
| `0000 0000` | 0 | RESERVED | Ignored |
| `0000 0001 - 1011 1111` | [1, 191] | Data class index | Stimulus label or sync signal |
| `1100 0000 - 1111 1111` | [192, 255] | Command | BCI state transition |

### Commands

| Value | Decimal | Meaning |
|---|---|---|
| `1100 0000` | 192 | IDLE — enter idle state |
| `1100 0001` | 193 | INFERENCE — enter inference state |
| `1100 0010` | 194 | TRAIN — enter training state |

---

## Key Design Decisions

### 1. Pure C (C99), no dynamic allocation

**Decision:** The library is written in C99 with no `malloc`/`free` in any core component. All memory is caller-provided.

**Reasoning:** Bare-metal MCUs often have no heap. By requiring the caller to provide all storage at init time, TinyBCI works identically on Cortex-M and desktop without ifdefs or allocator abstractions.

**Per-frame scratch buffers:** Fixed-size stack arrays sized by `TBCI_MAX_CHANNELS` (default 64, overridable via `-DTBCI_MAX_CHANNELS=N`). VLAs explicitly avoided.

---

### 2. Two separate input buffers

**Decision:** EEG samples and trigger events live in separate buffers — `TBCI_SignalBuffer` and `TBCI_TriggerQueue`.

---

### 3. Signal buffer overwrites; trigger queue rejects

**Decision:** `TBCI_SignalBuffer` silently overwrites the oldest frame when full. `TBCI_TriggerQueue` rejects new events when full and returns `TBCI_ERR_FULL`.

---

### 4. Unified input gateway: TBCI_Input

**Decision:** `TBCI_Input` serves two roles — shared reference for pipeline nodes, and input gateway for the producer. Push functions handle all routing internally, including command dispatch (codes [192-255]) to state transitions.

---

### 5. Timestamp-based synchronisation in CoreNode

**Decision:** SyncNode pops triggers, finds the corresponding frame via `sb_find_timestamp`, produces a `TBCI_SyncResult`. SegNode re-derives the logical frame index each tick.

**Note:** SyncNode searches `ctx->processed_signal` — extracted epochs contain preprocessed data when preprocessing is enabled.

---

### 6. Timestamp lookup: exact match with nearest-after fallback

**Decision:** `sb_find_timestamp` returns exact match, nearest-after, or `TBCI_ERR_NOT_YET`.

---

### 7. Push execution model with TBCI_NODE_PENDING short-circuit

**Decision:** DAG runner pushes data through nodes each tick. `TBCI_NODE_PENDING` short-circuits the chain without error.

---

### 8. Two-tier node interface

1. **Top-level DAG — `tick_fn(self, ctx)`** — uniform loop, no switch/dispatch.
2. **Inner-node — `process_fn(self, void *data, ctx)`** — `float*` for PreprocessingGroup, `TBCI_Epoch*` for FeatureExtractionGroup and DecoderGroup.

---

### 9. Developer-defined topology, mandatory nodes

`tbci_build_pipeline` always registers exactly four nodes:

1. `PreprocessingGroup` — copies raw frame to `processed_signal`. `use_preprocessing` gates inner chain.
2. `CoreNode` — sync + optional RawOutNode + segmentation. Writes to `epoch_queue`.
3. `FeatureExtractionGroup` — mandatory transpose then optional inner chain. `use_feature_extraction` gates inner chain.
4. `TBCI_Decoder` — state-driven model dispatch. `use_decoder` gates inner chain.

`ctx->n_nodes == 4` after init.

---

### 10. TBCI_EpochQueue as inter-group boundary

**Decision:** Same `TBCI_EpochQueue` type reused for `epoch_queue`, `features_queue`, and `output_queue`.

---

### 11. n_channels as runtime-discovered, single source of truth

**Decision:** `TBCI_Input.n_channels` set by producer at connect time. Two-step epoch queue init (`eq_init` + `eq_configure`) handles unknown channel count at startup.

---

### 12. Memory layout: time-major in buffers, channel-major in epochs

**Decision:** `TBCI_SignalBuffer` stores frames time-major. `fe_process` transposes in-place to channel-major unconditionally before any inner node.

---

### 13. Segmentation modes

- `SEG_MODE_TRIGGERED` — one trigger → one epoch. P300.
- `SEG_MODE_SLIDING` — overlapping windows between trial start/end triggers. MI/SSVEP.

---

### 14. LSL reader as external producer, not a node

**Decision:** `LSLProducer` implements `TBCI_Producer`. Guarded by `#ifdef TBCI_WITH_LSL`.

---

### 15. LSL writer as external consumer, not a node

**Decision:** `TBCI_LSLWriter` reads from `output_queue` in the runner loop. Output routing is runner-specific.

---

### 16. Timestamp-driven real-time pacing

**Decision:** Synthetic runner uses monotonic clock. LSL runner uses `tbci_sleep_us`.

**Platform portability:** `tbci_platform.h` abstracts `Sleep` (Windows), `nanosleep` (POSIX), no-op (bare-metal).

---

### 17. CoreNode as internal pipeline with optional logger

**Decision:** `TBCI_Core` contains SyncNode → optional `TBCI_RawOutNode` → SegNode.

**RawOutNode** writes one CSV row per signal frame continuously for the entire session.
Sits between sync and seg — after trigger is found, before epoch extraction.
Enabled via `config.log_enabled`, closed at `tbci_context_stop`.

Uses `sb_read_since` to drain all new frames since last write — guarantees no gaps
even when multiple samples arrive between pipeline ticks.

Trigger code attached to exactly one frame per tick — the first frame whose timestamp
meets or exceeds the trigger timestamp. All other frames get trigger_val = 0.

Optional tap callback `TBCI_RawOutFn` fires once per new frame alongside CSV logging.
Caller wires `on_frame` + `user_data` directly on the node after `ro_init`.

**CSV format:**
```
sample_idx,timestamp_us,trigger_val,ch0,ch1,...,chN
```

---

### 18. TBCI_NodeGroup — generic composite node

`TBCI_NodeGroup { TBCI_Node base; size_t n_nodes; TBCI_Node *nodes[TBCI_MAX_GROUP_NODES]; }` — `base` must be first member for safe casting.

---

### 19. TBCI_Decoder and TBCI_Model — classifier layer

**Decision:** `TBCI_Decoder` dispatches to inner `TBCI_Model` nodes based on `ctx->state`. `TBCI_Model` has `train`/`eval`/`infer` function pointers, extended by concrete types via composition.

**Concrete models:**
- `TBCI_CCAModel` — trainless SSVEP. Z-score + softmax + argmax on CCA correlations.

**Planned:** `TBCI_LDAModel`, `TBCI_ONNXModel`.

---

### 20. TBCI_CCANode — SSVEP feature extraction

All intermediate matrices owned by node struct. Reference signals generated at `init_fn` time from `(freqs, n_harmonics, srate, n_frames)`. Output: `n_channels = n_freqs`, `n_frames = 1`.

---

### 21. Shared math utilities

`tbci_math.h/.c`: `tbci_softmax`, `tbci_normalize_zscore`, `tbci_normalize_minmax`, `tbci_argmax`.

---

### 22. TBCI_MAX_CHANNELS — build-time upper bound

Default 64, override via `-DTBCI_MAX_CHANNELS=N`. Runtime guard returns `TBCI_NODE_ERROR` if violated.

---

### 23. IIR filter nodes — preprocessing inner nodes

**Decision:** Filtering implemented as a hierarchy of inner nodes:

- **`TBCI_IIRFilterState`** — not a node. Single Direct Form II transposed biquad. Per-channel state `w[TBCI_MAX_CHANNELS][2]`. Building block for filter nodes.
- **`TBCI_BandpassNode`** — inner node. Two cascaded `TBCI_IIRFilterState` (highpass + lowpass). Coefficients computed at `init_fn` from `(low_hz, high_hz, target_srate)` via bilinear transform.
- **`TBCI_NotchNode`** — inner node. One `TBCI_IIRFilterState` per harmonic. Computed at `init_fn` from `(freq_hz, q_factor, n_harmonics, target_srate)`. `TBCI_MAX_NOTCH_HARMONICS` default 3.

**All float** — sufficient for EEG, 2x faster than double on Cortex-M.

**Caller registration pattern — BEFORE `tbci_context_init`:**
```c
notch_init(&notch_node, &notch_config);
group_add_node(&ctx.preprocessing.group, (TBCI_Node *)&notch_node);
bp_init(&bp_node, &bp_config);
group_add_node(&ctx.preprocessing.group, (TBCI_Node *)&bp_node);
tbci_context_init(&ctx, ...);
```

---

## RAM Footprint (default build, 64 channels)

```
TBCI_Context:            880 bytes
TBCI_CCANode:          86552 bytes
TOTAL (ctx + CCANode):  85.4 KB
```

---

## File Structure

```
├── include/
│   ├── tbci_common.h
│   ├── tbci_config.h
│   ├── tbci_context.h
│   ├── mathutils/
│   │   ├── tbci_math.h
│   │   └── tbci_sizeof.h
│   ├── containers/
│   │   ├── tbci_signal_buffer.h
│   │   ├── tbci_trigger_queue.h
│   │   └── tbci_epoch_queue.h
│   ├── ioutils/
│   │   ├── tbci_input.h
│   │   ├── tbci_lsl_reader.h
│   │   └── tbci_lsl_writer.h
│   └── nodes/
│       ├── tbci_node.h
│       ├── tbci_node_group.h
│       ├── core/
│       │   ├── tbci_core.h
│       │   ├── tbci_sync_node.h
│       │   ├── tbci_seg_node.h
│       │   └── tbci_raw_out.h
│       ├── preprocessing/
│       │   ├── tbci_preprocessing.h
│       │   ├── tbci_iir_filter_state.h
│       │   ├── tbci_bandpass_node.h
│       │   └── tbci_notch_node.h
│       ├── features/
│       │   ├── tbci_features_extraction.h
│       │   ├── tbci_cca_node.h
│       └── decoder/
│           ├── tbci_decoder.h
│           ├── tbci_cca_model.h
│           ├── tbci_onnx_model.h          ← #ifdef TBCI_WITH_ONNX
│           ├── tbci_label_encoder_node.h
│           └── tbci_trial_averaging_node.h
├── src/
│   ├── tbci_config.c
│   ├── tbci_context.c
│   ├── mathutils/
│   │   └── tbci_math.c
│   ├── containers/
│   │   ├── tbci_signal_buffer.c
│   │   ├── tbci_trigger_queue.c
│   │   └── tbci_epoch_queue.c
│   ├── ioutils/
│   │   ├── tbci_input.c
│   │   ├── tbci_lsl_reader.c
│   │   └── tbci_lsl_writer.c
│   └── nodes/
│       ├── tbci_node.c
│       ├── tbci_node_group.c
│       ├── core/
│       │   ├── tbci_core_node.c
│       │   ├── tbci_sync_node.c
│       │   ├── tbci_seg_node.c
│       │   └── tbci_raw_out.c
│       ├── preprocessing/
│       │   ├── tbci_preprocessing.c
│       │   ├── tbci_iir_filter_state.c
│       │   ├── tbci_bandpass_node.c
│       │   └── tbci_notch_node.c
│       ├── features/
│       │   ├── tbci_features_extraction.c
│       │   └── tbci_cca_node.c
│       └── decoder/
│           ├── tbci_decoder.c
│           ├── tbci_cca_model.c
│           ├── tbci_onnx_model.c          ← #ifdef TBCI_WITH_ONNX
│           ├── tbci_label_encoder_node.c
│           └── tbci_trial_averaging_node.c
├── producer/
│   ├── tbci_producer.h
│   ├── tbci_producer_factory.h
│   ├── tbci_producer_factory.c
│   ├── tbci_trigger_generator.h
│   ├── tbci_trigger_generator.c
│   ├── synthetic_producer.h
│   ├── synthetic_producer.c
│   ├── unicorn_producer.h
│   ├── unicorn_producer.c
│   ├── lsl_producer.h                     ← #ifdef TBCI_WITH_LSL
│   └── lsl_producer.c                     ← #ifdef TBCI_WITH_LSL
├── examples/
│   ├── run_synthetic.c
│   ├── run_unicorn.c
│   ├── run_lsl.c                          ← #ifdef TBCI_WITH_LSL
│   └── run_onnx.c                         ← #ifdef TBCI_WITH_ONNX
├── scripts/
│   ├── simulate_lsl.py
│   ├── receive_lsl.py
│   ├── plot_raw.py
│   └── raw_plot.py
├── tests/
│   ├── unity/
│   ├── models/
│   │   └── model.onnx                     ← test ONNX model
│   ├── test_signal_buffer.c
│   ├── test_trigger_queue.c
│   ├── test_epoch_queue.c
│   ├── test_context.c
│   ├── test_segmentation.c
│   ├── test_node.c
│   ├── test_node_group.c
│   ├── test_preprocessing.c
│   ├── test_features_extraction.c
│   ├── test_cca_node.c
│   ├── test_math.c
│   ├── test_decoder.c
│   ├── test_iir_filter.c
│   ├── test_input.c
│   ├── test_pipeline.c
│   ├── test_onnx_model.c                  ← #ifdef TBCI_WITH_ONNX
│   ├── test_label_encoder.c
│   ├── test_trial_averaging_node.c
│   └── test_raw_out.c
├── tools/
│   └── tbci_sizeof.c
├── CMakeLists.txt
└── README.md
```

---

## Naming Conventions

| Category | Convention | Example |
|---|---|---|
| File names | `tbci_` prefix, snake_case | `tbci_signal_buffer.h` |
| Struct types | `TBCI_` prefix, PascalCase | `TBCI_SignalBuffer`, `TBCI_Epoch` |
| Enums | `TBCI_` prefix, UPPER_CASE values | `TBCI_Status`, `TBCI_STATE_IDLE` |
| Functions | short prefix, snake_case | `sb_init()`, `bp_process()`, `notch_process()` |
| Global/shared functions | `tbci_` prefix | `tbci_context_init()` |
| Macros | ALL_CAPS | `TBCI_MAX_NODES`, `TBCI_MAX_CHANNELS`, `TBCI_MAX_NOTCH_HARMONICS` |

---

## What's Next

```
✓ TBCI_SignalBuffer         — complete, tested
✓ sb_read_since             — iterate all frames with timestamp > since_ts
                               TBCI_FrameCallback typedef
                               used by RawOutNode for gap-free logging
✓ TBCI_TriggerQueue         — complete, tested
✓ TBCI_EpochQueue           — complete, tested
✓ TBCI_Config               — defined, includes CoreNode + decoder + logging parameters
✓ TBCI_Node                 — tick_fn (top-level) + process_fn/void* (inner) split
                               instance_size field added for runtime sizeof reporting
✓ TBCI_NodeGroup            — generic composite, group_init/process/reset/add/remove
✓ TBCI_Preprocessing        — always-run pass-through + optional inner chain, tested
✓ TBCI_FeatureExtraction    — mandatory transpose + optional inner chain, tested
✓ TBCI_Context              — 4-node pipeline, tested
✓ TBCI_Core                 — SyncNode + RawOutNode + SegNode, reads processed_signal
✓ DAG runner                — tbci_context_tick fully switch-free (uniform tick_fn)
✓ TBCI_Producer             — abstracted interface, Synthetic+Unicorn+LSL
✓ TBCI_TriggerGenerator     — reusable synthetic trigger, attachable to any producer
✓ Unicorn producer          — verified on hardware, BLE self-healing reconnect
✓ LSLProducer               — drain-all-per-tick, direct push, 3 modes, 2 resolve strategies
                               auto-reconnect on stream loss, reconnect_ticks backoff
✓ TBCI_LSLWriter            — LSL outlet wrapper, push epoch to output stream
                               predicted_label prepended to sample buffer
✓ TBCI_MAX_CHANNELS         — build-time override, VLA eliminated, runtime guard
✓ TBCI_CCANode              — full CCA implementation, no statics, no malloc, tested
✓ TBCI_Decoder              — state-driven dispatch, passthrough when disabled, tested
                               scans inner nodes by name for TBCI_LabelEncoderNode
                               calls le_decode before pushing to output_queue
✓ TBCI_Model                — base classifier interface, train/eval/infer
                               eval_score, confidence, predicted_class, scorer on base
✓ TBCI_CCAModel             — trainless SSVEP classifier, z-score + softmax + argmax
✓ TBCI_ONNXModel            — ONNX runtime inference (#ifdef TBCI_WITH_ONNX)
                               pretrained mode: load .onnx, infer only
                               sigmoid + softmax output modes
                               K-fold cross-validation in eval() with pluggable scorer
                               caller-provided calibration trial storage
                               auto-download ORT via CMake FetchContent
✓ tbci_math                 — softmax, zscore, minmax, argmax, tested
                               tbci_score_accuracy, tbci_score_f1, tbci_score_mcc
                               TBCI_ScorerFn function pointer type
✓ tbci_platform             — portable tbci_sleep_us (Windows/POSIX/bare-metal)
✓ tbci_sizeof               — DAG-aware runtime RAM reporter
                               walks ctx->nodes[], prints inner nodes indented
                               uses node->name and node->instance_size
                               TBCI_SizeofReport reduced to caller-provided buffer sizes
✓ Pipeline integration      — end-to-end test passing, real epochs from live EEG + LSL
✓ TBCI_RawOutNode           — continuous CSV logger, tested
                               sb_read_since: all frames per tick, no gaps
                               trigger attached to exactly one frame per tick
                               TBCI_RawOutFn tap callback fires per frame
                               RoWriteCtx bundles per-tick state for callback
✓ TBCI_IIRFilterState       — single biquad building block, Direct Form II transposed
✓ TBCI_BandpassNode         — 2nd order Butterworth bandpass, coefficients at init, tested
✓ TBCI_NotchNode            — cascaded notch, one biquad per harmonic, tested
✓ SyntheticProducer noise   — additive white noise via noise_amplitude config field
✓ TBCI_Epoch                — confidence, eval_score, encoded_label, predicted_label fields
✓ TBCI_LabelEncoderNode     — trigger code ↔ class index mapping [1-127] → [0-126]
                               encode: encoded_label = label - 1
                               decode: label = predicted_label + 1 (or encoded_label + 1)
                               og_map stores original label for target mode decode
                               found by name in dc_process, no type field needed
✓ TBCI_TrialAveragingNode   — accumulates n_reps epochs, averages probabilities
                               early stopping via confidence + margin thresholds
                               resets automatically after output
                               paradigm-agnostic: works for P300, MI, SSVEP
✓ run_onnx.c                — example runner with ONNX inference
✓ run_lsl.c                 — LSL runner, verified end-to-end with real EEG

○ TBCI_LDAModel             — linear discriminant analysis for MI/P300, low priority
○ TBCI_ONNXModel backbone+FC — TBCI_MLP as trainable FC head after ORT backbone
○ TBCI_MLP                  — generic trainable MLP, no malloc
                               forward/backward/update (Adam/SGD)
○ Python functional tests   — ctypes bindings, MNE visual inspection of epochs
○ LSL simulation update     — simulate_lsl.py: add SSVEP paradigm mode
```