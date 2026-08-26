/**
 * @file test_segmentation.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for TBCISegmentationNode using the Unity test framework.
 */

#include "unity/unity.h"
#include "../include/nodes/core/tbci_core.h"
#include "../include/ioutils/tbci_input.h"
#include <string.h>
#include <tbci_config.h>
#include <tbci_context.h>

#define SIG_CAPACITY     1024
#define SIG_CHANNELS       8
#define TRIG_CAPACITY     16
#define EPOCH_CAPACITY     4
#define TARGET_SRATE     256.0f   /* 256 Hz */
#define PRE_STIMULUS_MS  200
#define POST_STIMULUS_MS 800
#define OVERLAP_MS       400
/* total frames = (200 + 800) / 1000 * 256 = 256 frames */
#define TOTAL_FRAMES     256
#define PRE_FRAMES        51      /* 200 / 1000 * 256 = 51 */
#define POST_FRAMES      205      /* 800 / 1000 * 256 = 205 */
#define OVERLAP_FRAMES  ((size_t)(OVERLAP_MS / 1000.0f * TARGET_SRATE))  /* 102 */
#define STEP_FRAMES     (TOTAL_FRAMES - OVERLAP_FRAMES)                   /* 154 */

#define END_TRIAL    10u

static float           sig_storage[SIG_CAPACITY * SIG_CHANNELS];
static uint64_t        sig_timestamps[SIG_CAPACITY];
static uint32_t        sig_indices[SIG_CAPACITY];
static TBCI_Trigger     trig_storage[TRIG_CAPACITY];
static TBCI_Epoch       epoch_storage[EPOCH_CAPACITY];
static float           epoch_pool[EPOCH_CAPACITY * TOTAL_FRAMES * SIG_CHANNELS];
static TBCI_Epoch       features_storage[EPOCH_CAPACITY];
static float           features_pool[EPOCH_CAPACITY * TOTAL_FRAMES * SIG_CHANNELS];

static TBCI_SignalBuffer      signal_buf;
static TBCI_SignalBuffer      proc_signal_buf;
static TBCI_TriggerQueue      trigger_queue;
static TBCI_EpochQueue        epoch_queue;
static TBCI_EpochQueue        features_queue;
static TBCI_Input            inputs;
static TBCI_Config            config;
static TBCI_Context           ctx;
static TBCI_CoreConfig    seg_config;
static TBCI_Core  core_node;

void setUp(void)
{
    memset(sig_storage,    0, sizeof(sig_storage));
    memset(sig_timestamps, 0, sizeof(sig_timestamps));
    memset(sig_indices,    0, sizeof(sig_indices));
    memset(trig_storage,   0, sizeof(trig_storage));
    memset(epoch_storage,  0, sizeof(epoch_storage));
    memset(epoch_pool,     0, sizeof(epoch_pool));

    sb_init(&signal_buf, sig_storage, sig_timestamps, sig_indices,
            SIG_CAPACITY, SIG_CHANNELS);
    sb_init(&proc_signal_buf, sig_storage, sig_timestamps, sig_indices,
            SIG_CAPACITY, SIG_CHANNELS);
    tq_init(&trigger_queue, trig_storage, TRIG_CAPACITY);
    eq_init(&epoch_queue, epoch_storage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_init(&features_queue, features_storage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&epoch_queue, epoch_pool, SIG_CHANNELS);
    eq_configure(&features_queue, features_pool, SIG_CHANNELS);

    inputs.signal     = &signal_buf;
    inputs.triggers   = &trigger_queue;
    inputs.n_channels = SIG_CHANNELS;

    config.paradigm               = TBCI_PARADIGM_P300;
    config.nominal_srate          = TARGET_SRATE;
    config.target_srate           = TARGET_SRATE;
    config.n_channels             = SIG_CHANNELS;
    config.window_length_ms       = PRE_STIMULUS_MS + POST_STIMULUS_MS;
    config.use_preprocessing      = false;
    config.use_feature_extraction = false;
    seg_config.trial_end_code   = END_TRIAL;

    tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);

    seg_config.pre_stimulus_ms  = PRE_STIMULUS_MS;
    seg_config.post_stimulus_ms = POST_STIMULUS_MS;

    cn_init(&core_node, &seg_config,  &ctx);
}

void set_triggered_mode(void)
{
    seg_config.mode = SEG_MODE_TRIGGERED;
    seg_config.overlap_ms = 0;
    cn_init(&core_node, &seg_config,   &ctx);
}

void set_sliding_mode(void)
{
    seg_config.mode = SEG_MODE_SLIDING;
    seg_config.overlap_ms = OVERLAP_MS;
    cn_init(&core_node, &seg_config,   &ctx);
}

void tearDown(void) {}

/* --------------------------------------------------------------------------
 * Helper: push n synthetic frames into the signal buffer
 * starting at a given timestamp, spaced by 1000000/TARGET_SRATE us
 * -------------------------------------------------------------------------- */
static void push_signal_frames(uint64_t start_us, size_t n)
{
    float samples[SIG_CHANNELS];
    uint32_t spacing_us = (uint32_t)(1000000.0f / TARGET_SRATE);

    for (size_t i = 0; i < n; i++) {
        memset(samples, 0, sizeof(samples));
        sb_put(&signal_buf, samples, start_us + i * spacing_us, (uint32_t)i);
        sb_put(&proc_signal_buf, samples, start_us + i * spacing_us, (uint32_t)i);
    }
}

/* --------------------------------------------------------------------------
 * Helper: push a trigger at a given timestamp
 * -------------------------------------------------------------------------- */
static void push_trigger(uint64_t timestamp_us, uint16_t code)
{
    TBCI_Trigger t = { .timestamp_us = timestamp_us, .code = code };
    tq_push(&trigger_queue, &t);
}

/* ============================================================
 * GROUP 1 — Init
 * ============================================================ */

void test_sn_init_valid_returns_ok(void)
{
    TBCI_Status status = cn_init(&core_node, &seg_config, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}

void test_sn_init_null_node_returns_invalid_arg(void)
{
    TBCI_Status status = cn_init(NULL, &seg_config,   &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_sn_init_null_config_returns_invalid_arg(void)
{
    TBCI_Status status = cn_init(&core_node, NULL,   &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_sn_init_seg_state_phase_is_idle_after_init(void)
{
    cn_init(&core_node, &seg_config, &ctx);
    TEST_ASSERT_EQUAL(SEG_IDLE, core_node.seg.state.phase);
}

void test_sn_init_null_ctx_returns_invalid_arg(void)
{
    TBCI_Status status = cn_init(&core_node, &seg_config, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_sn_init_zero_post_stimulus_returns_invalid_arg(void)
{
    seg_config.post_stimulus_ms = 0;
    TBCI_Status status = cn_init(&core_node, &seg_config,   &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
    seg_config.post_stimulus_ms = POST_STIMULUS_MS;
}

void test_sn_init_phase_is_idle(void)
{
    TBCI_Status status = cn_init(&core_node, &seg_config,   &ctx);
    TBCI_SegmentationState* seg_state = &core_node.seg.state;
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(SEG_IDLE, seg_state->phase);
}

void test_sn_init_pre_frames_computed_correctly(void)
{
    TBCI_Status status = cn_init(&core_node, &seg_config,   &ctx);
    TBCI_SegmentationState* seg_state = &core_node.seg.state;
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_size_t(PRE_FRAMES, seg_state->pre_frames);
}

void test_sn_init_post_frames_computed_correctly(void)
{
    TBCI_Status status = cn_init(&core_node, &seg_config,   &ctx);
    TBCI_SegmentationState* seg_state = &core_node.seg.state;
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_size_t(POST_FRAMES, seg_state->post_frames);
}

void test_sn_init_total_frames_computed_correctly(void)
{
    TBCI_Status status = cn_init(&core_node, &seg_config,   &ctx);
    TBCI_SegmentationState* seg_state = &core_node.seg.state;
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_size_t(TOTAL_FRAMES, seg_state->total_frames);
}

void test_sn_init_zero_pre_stimulus_is_valid(void)
{
    seg_config.pre_stimulus_ms = 0;
    TBCI_Status status = cn_init(&core_node, &seg_config,   &ctx);
    TBCI_SegmentationState* seg_state = &core_node.seg.state;

    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_size_t(0, seg_state->pre_frames);
    TEST_ASSERT_EQUAL_size_t(POST_FRAMES, seg_state->post_frames);
    seg_config.pre_stimulus_ms = PRE_STIMULUS_MS;
}

void test_sn_init_triggered_mode_valid(void)
{
    set_triggered_mode();
    TBCI_Status status = cn_init(&core_node, &seg_config,   &ctx);
    TBCI_SegmentationState* seg_state = &core_node.seg.state;

    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(SEG_MODE_TRIGGERED, core_node.config->mode);
    TEST_ASSERT_EQUAL(SEG_IDLE, seg_state->phase);
}

void test_sn_init_sliding_mode_valid(void)
{
    config.paradigm = TBCI_PARADIGM_MI;
    tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
    set_sliding_mode();
    TBCI_Status status = cn_init(&core_node, &seg_config,   &ctx);
    TBCI_SegmentationState* seg_state = &core_node.seg.state;

    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(SEG_MODE_SLIDING, core_node.config->mode);
    TEST_ASSERT_EQUAL(SEG_IDLE, seg_state->phase);
}

void test_sn_init_p300_with_sliding_returns_warning(void)
{
    config.paradigm = TBCI_PARADIGM_P300;
    tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
    set_sliding_mode();
    TBCI_Status status = cn_init(&core_node, &seg_config,   &ctx);
    TEST_ASSERT_EQUAL(TBCI_WARN_PARADIGM_MODE_MISMATCH, status);
}

void test_sn_init_mi_with_triggered_returns_warning(void)
{
    config.paradigm = TBCI_PARADIGM_MI;
    tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
    set_triggered_mode();
    TBCI_Status status = cn_init(&core_node, &seg_config,   &ctx);
    TEST_ASSERT_EQUAL(TBCI_WARN_PARADIGM_MODE_MISMATCH, status);
}

void test_sn_init_ssvep_with_triggered_returns_warning(void)
{
    config.paradigm = TBCI_PARADIGM_SSVEP;
    tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
    set_triggered_mode();
    TBCI_Status status = cn_init(&core_node, &seg_config,   &ctx);
    TEST_ASSERT_EQUAL(TBCI_WARN_PARADIGM_MODE_MISMATCH, status);
}

void test_sn_init_sliding_zero_overlap_is_valid(void)
{
    config.paradigm = TBCI_PARADIGM_MI;
    tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
    set_sliding_mode();
    seg_config.overlap_ms = 0;
    TBCI_Status status = cn_init(&core_node, &seg_config,   &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}

void test_sn_init_sliding_overlap_gte_window_returns_invalid_arg(void)
{
    set_sliding_mode();
    seg_config.overlap_ms = (PRE_STIMULUS_MS + POST_STIMULUS_MS);
    TBCI_Status status = cn_init(&core_node, &seg_config,   &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_sn_init_sliding_overlap_frames_computed_correctly(void)
{
    set_sliding_mode();
    TBCI_SegmentationState* seg_state = &core_node.seg.state;

    TEST_ASSERT_EQUAL_size_t(OVERLAP_FRAMES, seg_state->overlap_frames);
}

void test_sn_init_sliding_step_frames_computed_correctly(void)
{
    set_sliding_mode();
    TBCI_SegmentationState* seg_state = &core_node.seg.state;

    TEST_ASSERT_EQUAL_size_t(STEP_FRAMES, seg_state->step_frames);
}

/* ============================================================
 * GROUP 2 — Process: no data
 * ============================================================ */

void test_sn_process_null_node_returns_error(void)
{
    TBCI_NodeResult res = cn_process(NULL, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, res);
}

void test_sn_process_null_inputs_returns_error(void)
{
    ctx.inputs = NULL;
    TBCI_NodeResult res = cn_process(&core_node, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, res);
}

void test_sn_process_null_epoch_queue_returns_error(void)
{
    ctx.epoch_queue = NULL;
    TBCI_NodeResult res = cn_process(&core_node, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, res);
}

void test_sn_process_null_ctx_returns_error(void)
{
    TBCI_NodeResult res = cn_process(&core_node, NULL);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, res);
}

void test_sn_process_empty_trigger_queue_returns_pending(void)
{
    push_signal_frames(0, 10);
    TBCI_NodeResult res = cn_process(&core_node, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, res);
}

void test_sn_process_trigger_but_no_signal_returns_pending(void)
{
    set_triggered_mode();
    push_trigger(0, 1);
    TBCI_NodeResult res = cn_process(&core_node, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, res);
}

void test_sn_process_state_is_waiting_after_trigger_found(void)
{
    set_triggered_mode();
    push_signal_frames(0, 10);  // some signal but not enough for full window
    push_trigger(0, 1u);
    cn_process(&core_node, &ctx);

    // sync is now SYNC_MATCHING — waiting for enough data
    // seg stays SEG_IDLE since sync hasn't produced a result yet
    TEST_ASSERT_EQUAL(SYNC_MATCHING, core_node.sync.state.synch_phase);
    TEST_ASSERT_EQUAL(SEG_IDLE, core_node.seg.state.phase);
}

/* ============================================================
 * GROUP 3 — Process: not enough data
 * ============================================================ */

void test_sn_process_insufficient_post_stimulus_returns_pending(void)
{
    push_signal_frames(0, PRE_FRAMES + 10);   // some signal but not enough for full window
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];
    push_trigger(trigger_ts, 1u);

    TBCI_NodeResult res = cn_process(&core_node, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, res);
}

void test_sn_process_insufficient_pre_stimulus_returns_pending(void)
{
    push_signal_frames(0, POST_FRAMES + 10);
    uint64_t trigger_ts = sig_timestamps[5];
    push_trigger(trigger_ts, 1u);

    TBCI_NodeResult res = cn_process(&core_node, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, res);
}

/* ============================================================
 * GROUP 4 — Process: successful epoch extraction
 * ============================================================ */

void test_sn_process_full_window_returns_ok(void)
{
    push_signal_frames(0, SIG_CAPACITY);
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];;
    push_trigger(trigger_ts, 1u);

    TBCI_NodeResult res = cn_process(&core_node, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);
}

void test_sn_process_epoch_pushed_to_queue(void)
{
    push_signal_frames(0, SIG_CAPACITY);
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];;
    push_trigger(trigger_ts, 1u);

    TBCI_NodeResult res = cn_process(&core_node, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);
    TEST_ASSERT_EQUAL(false, eq_is_empty(&epoch_queue));
}

void test_sn_process_epoch_label_matches_trigger_code(void)
{
    push_signal_frames(0, SIG_CAPACITY);
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];;
    push_trigger(trigger_ts, 1u);

    TBCI_NodeResult res = cn_process(&core_node, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);
    TBCI_Epoch epoch;
    eq_pop(&epoch_queue, &epoch);
    TEST_ASSERT_EQUAL(1u, epoch.label);
}

void test_sn_process_epoch_timestamp_matches_trigger(void)
{
    push_signal_frames(0, SIG_CAPACITY);
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];;
    push_trigger(trigger_ts, 1u);

    TBCI_NodeResult res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);
    TBCI_Epoch epoch;
    eq_pop(&epoch_queue, &epoch);
    TEST_ASSERT_EQUAL(trigger_ts, epoch.timestamp_us);
}

void test_sn_process_trigger_consumed_after_extraction(void)
{
    push_signal_frames(0, SIG_CAPACITY);
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];;
    push_trigger(trigger_ts, 1u);

    TBCI_NodeResult res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);
    TEST_ASSERT_EQUAL(true, tq_is_empty(inputs.triggers));
}

void test_sn_process_zero_pre_stimulus_returns_ok(void)
{
    seg_config.pre_stimulus_ms = 0;
    cn_init(&core_node, &seg_config,   &ctx);
    push_signal_frames(0, POST_FRAMES + 10);
    uint64_t trigger_ts = sig_timestamps[0];
    push_trigger(trigger_ts, 1u);

    TBCI_NodeResult res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);
    seg_config.pre_stimulus_ms = PRE_STIMULUS_MS;
}

/* ============================================================
 * GROUP 5 — Reset
 * ============================================================ */

void test_sn_reset_phase_is_idle(void)
{
    TBCI_Status res = cn_reset(&core_node);
    TEST_ASSERT_EQUAL(TBCI_OK, res);
    TEST_ASSERT_EQUAL(SEG_IDLE, core_node.seg.state.phase);
}

void test_sn_reset_null_returns_invalid_arg(void)
{
    TBCI_Status res = cn_reset(NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, res);
}

void test_sn_reset_usable_after_reset(void)
{
    TBCI_Status res = cn_reset(&core_node);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);
    push_signal_frames(0, SIG_CAPACITY);
    push_trigger(sig_timestamps[PRE_FRAMES], 1u);

    TBCI_NodeResult node_res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, node_res);
}

/* ============================================================
 * GROUP 6 — Sliding window process
 * ============================================================ */

/* trial boundaries */
void test_sn_sliding_no_trigger_returns_pending(void)
{
    set_sliding_mode();
    push_signal_frames(0, SIG_CAPACITY);
    TBCI_NodeResult res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, res);
}

void test_sn_sliding_start_trigger_transitions_to_running(void)
{
    set_sliding_mode();
    push_signal_frames(0, SIG_CAPACITY);
    push_trigger(sig_timestamps[PRE_FRAMES], 1u);

    cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(SEG_RUNNING, core_node.seg.state.phase);
}

void test_sn_sliding_end_trigger_transitions_to_idle(void)
{
    set_sliding_mode();
    push_signal_frames(0, SIG_CAPACITY);

    push_trigger(sig_timestamps[PRE_FRAMES], 1u);
    cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(SEG_RUNNING, core_node.seg.state.phase);

    push_trigger(sig_timestamps[SIG_CAPACITY - 1], seg_config.trial_end_code);
    cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(SEG_IDLE, core_node.seg.state.phase);
}

void test_sn_sliding_non_boundary_trigger_ignored(void)
{
    set_sliding_mode();
    push_signal_frames(0, SIG_CAPACITY);

    push_trigger(sig_timestamps[PRE_FRAMES], 1u);
    cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(SEG_RUNNING, core_node.seg.state.phase);

    push_trigger(sig_timestamps[10], 3u);
    cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(SEG_RUNNING, core_node.seg.state.phase);
}

/* epoch emission */
void test_sn_sliding_emits_first_epoch_when_window_full(void)
{
    set_sliding_mode();
    push_signal_frames(0, PRE_FRAMES + POST_FRAMES + 10);
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];
    push_trigger(trigger_ts, 1u);

    TBCI_NodeResult res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);
    TEST_ASSERT_FALSE(eq_is_empty(&epoch_queue));
}

void test_sn_sliding_emits_multiple_epochs_per_trial(void)
{
    set_sliding_mode();
    push_signal_frames(0, PRE_FRAMES + POST_FRAMES + STEP_FRAMES + 10);
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];
    push_trigger(trigger_ts, 1u);

    // first epoch
    TBCI_NodeResult res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);

    // second epoch
    res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);

    TEST_ASSERT_EQUAL_size_t(2, eq_size(&epoch_queue));
}

void test_sn_sliding_step_advances_by_step_frames(void)
{
    set_sliding_mode();
    push_signal_frames(0, SIG_CAPACITY);
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];
    push_trigger(trigger_ts, 1u);

    // first epoch
    TBCI_NodeResult res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);
    TBCI_Epoch first_epoch;
    eq_pop(&epoch_queue, &first_epoch);

    // second epoch
    res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);
    TBCI_Epoch second_epoch;
    eq_pop(&epoch_queue, &second_epoch);

    uint64_t spacing_us = (uint64_t)(1000000.0f / TARGET_SRATE); // = 3906
    uint64_t expected_step_us = STEP_FRAMES * spacing_us;
    TEST_ASSERT_EQUAL_UINT64(first_epoch.timestamp_us + expected_step_us, second_epoch.timestamp_us);
}

void test_sn_sliding_epoch_label_matches_start_trigger_code(void)
{
    set_sliding_mode();
    ctx.state = TBCI_STATE_TRAINING;

    push_signal_frames(0, PRE_FRAMES + POST_FRAMES + STEP_FRAMES);
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];
    push_trigger(trigger_ts, 42u);

    TBCI_NodeResult res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);
    TBCI_Epoch first_epoch;
    eq_pop(&epoch_queue, &first_epoch);

    TEST_ASSERT_EQUAL_UINT16(42u, first_epoch.label);

}


void test_sn_sliding_returns_pending_if_window_not_full(void)
{
    set_sliding_mode();
    ctx.state = TBCI_STATE_TRAINING;

    push_signal_frames(0, PRE_FRAMES + (POST_FRAMES / 2));
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];
    push_trigger(trigger_ts, 42u);

    TBCI_NodeResult res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, res);
}

void test_sn_sliding_returns_ok_when_epoch_emitted(void)
{
    set_sliding_mode();
    push_signal_frames(0, PRE_FRAMES + POST_FRAMES + 10);
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];
    push_trigger(trigger_ts, 1u);

    TBCI_NodeResult res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);
}

/* reset */
void test_sn_sliding_reset_clears_running_state(void)
{
    set_sliding_mode();
    push_signal_frames(0, PRE_FRAMES + POST_FRAMES + 10);
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];
    push_trigger(trigger_ts, 1u);

    TBCI_NodeResult res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);

    cn_reset(&core_node);
    TEST_ASSERT_EQUAL(SEG_IDLE, core_node.seg.state.phase);
}

void test_sn_sliding_reset_clears_window_start(void)
{
    set_sliding_mode();
    push_signal_frames(0, PRE_FRAMES + POST_FRAMES + 10);
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];
    push_trigger(trigger_ts, 1u);

    TBCI_NodeResult res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);

    cn_reset(&core_node);
    TEST_ASSERT_EQUAL(0, core_node.seg.state.window_start_us);
}

void test_sn_sliding_usable_after_reset(void)
{
    set_sliding_mode();
    push_signal_frames(0, PRE_FRAMES + POST_FRAMES + 10);
    uint64_t trigger_ts = sig_timestamps[PRE_FRAMES];
    push_trigger(trigger_ts, 1u);

    TBCI_NodeResult res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);

    cn_reset(&core_node);
    TEST_ASSERT_EQUAL(SEG_IDLE, core_node.seg.state.phase);

    push_signal_frames(0, PRE_FRAMES + POST_FRAMES + 10);
    trigger_ts = sig_timestamps[PRE_FRAMES];
    push_trigger(trigger_ts, 2u);

    res = cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, res);
}

/* ============================================================
 * GROUP 7 — Sync node internal state
 * ============================================================ */

void test_sync_idle_after_init(void)
{
    TEST_ASSERT_EQUAL(SYNC_IDLE, core_node.sync.state.synch_phase);
}

void test_sync_matching_after_trigger_popped(void)
{
    set_triggered_mode();
    push_signal_frames(0, 10);
    push_trigger(0, 1u);
    cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(SYNC_MATCHING, core_node.sync.state.synch_phase);
}

void test_sync_idle_after_successful_match(void)
{
    set_triggered_mode();
    push_signal_frames(0, SIG_CAPACITY);
    push_trigger(sig_timestamps[PRE_FRAMES], 1u);
    cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(SYNC_IDLE, core_node.sync.state.synch_phase);
}

void test_sync_pre_frames_computed_correctly(void)
{
    TEST_ASSERT_EQUAL_size_t(PRE_FRAMES, core_node.sync.state.pre_frames);
}

void test_sync_running_after_sliding_trial_start(void)
{
    set_sliding_mode();
    push_signal_frames(0, SIG_CAPACITY);
    push_trigger(sig_timestamps[PRE_FRAMES], 1u);
    cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(SYNC_RUNNING, core_node.sync.state.synch_phase);
}

void test_sync_idle_after_sliding_trial_end(void)
{
    set_sliding_mode();
    push_signal_frames(0, SIG_CAPACITY);

    push_trigger(sig_timestamps[PRE_FRAMES], 1u);
    cn_process(&core_node,  &ctx);

    push_trigger(sig_timestamps[SIG_CAPACITY - 1], seg_config.trial_end_code);
    cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(SYNC_IDLE, core_node.sync.state.synch_phase);
}

void test_sync_reset_clears_phase(void)
{
    set_triggered_mode();
    push_signal_frames(0, 10);
    push_trigger(0, 1u);
    cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL(SYNC_MATCHING, core_node.sync.state.synch_phase);

    cn_reset(&core_node);
    TEST_ASSERT_EQUAL(SYNC_IDLE, core_node.sync.state.synch_phase);
}

void test_sync_reset_clears_window_start(void)
{
    set_sliding_mode();
    push_signal_frames(0, SIG_CAPACITY);
    push_trigger(sig_timestamps[PRE_FRAMES], 1u);
    cn_process(&core_node,  &ctx);

    cn_reset(&core_node);
    TEST_ASSERT_EQUAL_size_t(0, core_node.sync.state.window_start);
}

/* ============================================================
 * GROUP 8 — Seg node receives sync result correctly
 * ============================================================ */

void test_seg_pending_trigger_set_from_sync(void)
{
    set_triggered_mode();
    push_signal_frames(0, SIG_CAPACITY);
    push_trigger(sig_timestamps[PRE_FRAMES], 42u);
    cn_process(&core_node,  &ctx);
    TEST_ASSERT_EQUAL_UINT16(42u, core_node.seg.state.pending_trigger.code);
}

void test_seg_window_start_set_from_sync_on_trial_start(void)
{
    set_sliding_mode();
    push_signal_frames(0, SIG_CAPACITY);
    push_trigger(sig_timestamps[PRE_FRAMES], 1u);

    cn_process(&core_node,  &ctx);

    // window_start_us advances by one step after first epoch
    uint64_t spacing_us = (uint64_t)(1000000.0f / TARGET_SRATE);
    uint64_t expected_us = STEP_FRAMES * spacing_us;
    TEST_ASSERT_EQUAL_UINT64(expected_us, core_node.seg.state.window_start_us);
}

void test_seg_window_advances_independently_of_sync(void)
{
    set_sliding_mode();
    push_signal_frames(0, SIG_CAPACITY);
    push_trigger(sig_timestamps[PRE_FRAMES], 1u);

    uint64_t spacing_us = (uint64_t)(1000000.0f / TARGET_SRATE);
    uint64_t step_us    = STEP_FRAMES * spacing_us;

    cn_process(&core_node,  &ctx);
    uint64_t after_first = core_node.seg.state.window_start_us;

    cn_process(&core_node,  &ctx);
    uint64_t after_second = core_node.seg.state.window_start_us;

    TEST_ASSERT_EQUAL_UINT64(after_first + step_us, after_second);
}

void test_seg_reset_clears_pending_trigger(void)
{
    set_triggered_mode();
    push_signal_frames(0, SIG_CAPACITY);
    push_trigger(sig_timestamps[PRE_FRAMES], 42u);
    cn_process(&core_node,  &ctx);

    cn_reset(&core_node);
    TEST_ASSERT_EQUAL_UINT16(0u, core_node.seg.state.pending_trigger.code);
}

int main(void)
{
    UNITY_BEGIN();

    // Group 1
    RUN_TEST(test_sn_init_valid_returns_ok);
    RUN_TEST(test_sn_init_null_node_returns_invalid_arg);
    RUN_TEST(test_sn_init_null_config_returns_invalid_arg);
    RUN_TEST(test_sn_init_seg_state_phase_is_idle_after_init);
    RUN_TEST(test_sn_init_null_ctx_returns_invalid_arg);
    RUN_TEST(test_sn_init_zero_post_stimulus_returns_invalid_arg);
    RUN_TEST(test_sn_init_phase_is_idle);
    RUN_TEST(test_sn_init_pre_frames_computed_correctly);
    RUN_TEST(test_sn_init_post_frames_computed_correctly);
    RUN_TEST(test_sn_init_total_frames_computed_correctly);
    RUN_TEST(test_sn_init_zero_pre_stimulus_is_valid);
    RUN_TEST(test_sn_init_triggered_mode_valid);
    RUN_TEST(test_sn_init_sliding_mode_valid);
    RUN_TEST(test_sn_init_p300_with_sliding_returns_warning);
    RUN_TEST(test_sn_init_mi_with_triggered_returns_warning);
    RUN_TEST(test_sn_init_ssvep_with_triggered_returns_warning);
    RUN_TEST(test_sn_init_sliding_zero_overlap_is_valid);
    RUN_TEST(test_sn_init_sliding_overlap_gte_window_returns_invalid_arg);
    RUN_TEST(test_sn_init_sliding_overlap_frames_computed_correctly);
    RUN_TEST(test_sn_init_sliding_step_frames_computed_correctly);

    // Group 2
    RUN_TEST(test_sn_process_null_node_returns_error);
    RUN_TEST(test_sn_process_null_inputs_returns_error);
    RUN_TEST(test_sn_process_null_epoch_queue_returns_error);
    RUN_TEST(test_sn_process_null_ctx_returns_error);
    RUN_TEST(test_sn_process_empty_trigger_queue_returns_pending);
    RUN_TEST(test_sn_process_trigger_but_no_signal_returns_pending);
    RUN_TEST(test_sn_process_state_is_waiting_after_trigger_found);

    // Group 3
    RUN_TEST(test_sn_process_insufficient_post_stimulus_returns_pending);
    RUN_TEST(test_sn_process_insufficient_pre_stimulus_returns_pending);

    // Group 4
    RUN_TEST(test_sn_process_full_window_returns_ok);
    RUN_TEST(test_sn_process_epoch_pushed_to_queue);
    RUN_TEST(test_sn_process_epoch_label_matches_trigger_code);
    RUN_TEST(test_sn_process_epoch_timestamp_matches_trigger);
    RUN_TEST(test_sn_process_trigger_consumed_after_extraction);
    RUN_TEST(test_sn_process_zero_pre_stimulus_returns_ok);

    // Group 5
    RUN_TEST(test_sn_reset_phase_is_idle);
    RUN_TEST(test_sn_reset_null_returns_invalid_arg);
    RUN_TEST(test_sn_reset_usable_after_reset);

    // Group 6
    RUN_TEST(test_sn_sliding_no_trigger_returns_pending);
    RUN_TEST(test_sn_sliding_start_trigger_transitions_to_running);
    RUN_TEST(test_sn_sliding_end_trigger_transitions_to_idle);
    RUN_TEST(test_sn_sliding_non_boundary_trigger_ignored);

    RUN_TEST(test_sn_sliding_emits_first_epoch_when_window_full);
    RUN_TEST(test_sn_sliding_emits_multiple_epochs_per_trial);
    RUN_TEST(test_sn_sliding_step_advances_by_step_frames);
    RUN_TEST(test_sn_sliding_epoch_label_matches_start_trigger_code);
    RUN_TEST(test_sn_sliding_returns_pending_if_window_not_full);
    RUN_TEST(test_sn_sliding_returns_ok_when_epoch_emitted);

    RUN_TEST(test_sn_sliding_reset_clears_running_state);
    RUN_TEST(test_sn_sliding_reset_clears_window_start);
    RUN_TEST(test_sn_sliding_usable_after_reset);

    // Group 7
    RUN_TEST(test_sync_idle_after_init);
    RUN_TEST(test_sync_matching_after_trigger_popped);
    RUN_TEST(test_sync_idle_after_successful_match);
    RUN_TEST(test_sync_pre_frames_computed_correctly);
    RUN_TEST(test_sync_running_after_sliding_trial_start);
    RUN_TEST(test_sync_idle_after_sliding_trial_end);
    RUN_TEST(test_sync_reset_clears_phase);
    RUN_TEST(test_sync_reset_clears_window_start);

    // Group 8
    RUN_TEST(test_seg_pending_trigger_set_from_sync);
    RUN_TEST(test_seg_window_start_set_from_sync_on_trial_start);
    RUN_TEST(test_seg_window_advances_independently_of_sync);
    RUN_TEST(test_seg_reset_clears_pending_trigger);

    return UNITY_END();
}