/**
* @file test_pipeline.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for test_pipeline.c using the Unity test framework.
 *
 * Tests are grouped by functional area:
 *
 *
 * Build and run via CTest:
 *  cmake --build . && ctest --verbose
 *
 * @note Unity (https://github.com/ThrowTheSwitch/Unity) must be present at
 *       tests/unity/unity.c and tests/unity/unity.h.
 * @note This test file does NOT test thread safety.
 */

#include "unity/unity.h"
#include "tbci_context.h"
#include "../include/nodes/core/tbci_core.h"
#include <string.h>

#include "nodes/decoder/tbci_label_encoder_node.h"

#define SIG_CAPACITY      512
#define SIG_CHANNELS        8
#define TRIG_CAPACITY      16
#define EPOCH_CAPACITY      4
#define TARGET_SRATE     256.0f
#define PRE_STIMULUS_MS   200
#define POST_STIMULUS_MS  800
#define TOTAL_FRAMES      256
#define PRE_FRAMES         51

// all static storage
static float           sig_storage[SIG_CAPACITY * SIG_CHANNELS];
static uint64_t        sig_timestamps[SIG_CAPACITY];
static uint32_t        sig_indices[SIG_CAPACITY];
static float           proc_storage   [SIG_CAPACITY * SIG_CHANNELS];
static uint64_t        proc_timestamps[SIG_CAPACITY];
static uint32_t        proc_indices   [SIG_CAPACITY];
static TBCI_Trigger    trig_storage[TRIG_CAPACITY];
static TBCI_Epoch      epoch_storage[EPOCH_CAPACITY];
static float           epoch_pool[EPOCH_CAPACITY * TOTAL_FRAMES * SIG_CHANNELS];
static TBCI_Epoch      features_storage[EPOCH_CAPACITY];
static float           features_pool[EPOCH_CAPACITY * TOTAL_FRAMES * SIG_CHANNELS];
static TBCI_Epoch      output_storage[EPOCH_CAPACITY];
static float           output_pool[EPOCH_CAPACITY * TOTAL_FRAMES * SIG_CHANNELS];

static TBCI_SignalBuffer       signal_buf;
static TBCI_SignalBuffer       proc_signal_buf;
static TBCI_TriggerQueue       trigger_queue;
static TBCI_EpochQueue         epoch_queue;
static TBCI_EpochQueue         features_queue;
static TBCI_EpochQueue         output_queue;
static TBCI_Input              inputs;
static TBCI_Config             config;
static TBCI_Context            ctx;
static TBCI_LabelEncoderConfig le_cfg;
static TBCI_LabelEncoderNode encoder;

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static void push_signal_frames(uint64_t start_us, size_t n)
{
    float samples[SIG_CHANNELS];
    uint32_t spacing_us = (uint32_t)(1000000.0f / TARGET_SRATE);

    for (size_t i = 0; i < n; i++) {
        memset(samples, 0, sizeof(samples));
        sb_put(&signal_buf, samples, start_us + i * spacing_us, (uint32_t)i);
    }
}

static void push_trigger(uint64_t timestamp_us, uint16_t code)
{
    TBCI_Trigger t = { .timestamp_us = timestamp_us, .code = code };
    tq_push(&trigger_queue, &t);
}

static void push_signal_and_trigger(uint64_t timestamp_us, uint16_t code)
{
    uint32_t spacing_us = (uint32_t)(1000000.0f / TARGET_SRATE);
    float samples[SIG_CHANNELS];
    memset(samples, 0, sizeof(samples));

    bool trigger_pushed = false;

    for (size_t i = 0; i < SIG_CAPACITY; i++) {
        uint64_t ts = (uint64_t)(i + 1) * spacing_us;
        sb_put(&signal_buf, samples, ts, (uint32_t)i);

        if (i == PRE_FRAMES && !trigger_pushed) {
            push_trigger(ts, code);
            trigger_pushed = true;
        }

        TBCI_Status status = tbci_context_tick(&ctx);
        TEST_ASSERT_EQUAL(TBCI_OK, status);
    }
}

static void setup_pipeline(void)
{
    sb_init(&signal_buf, sig_storage, sig_timestamps, sig_indices,SIG_CAPACITY, SIG_CHANNELS);
    sb_init(&proc_signal_buf, proc_storage, proc_timestamps, proc_indices, SIG_CAPACITY, SIG_CHANNELS);
    tq_init(&trigger_queue, trig_storage, TRIG_CAPACITY);
    eq_init(&epoch_queue, epoch_storage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_init(&features_queue, features_storage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_init(&output_queue, output_storage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&epoch_queue, epoch_pool, SIG_CHANNELS);
    eq_configure(&features_queue, features_pool, SIG_CHANNELS);
    eq_configure(&output_queue, output_pool, SIG_CHANNELS);

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

    config.mode             = SEG_MODE_TRIGGERED;
    config.pre_stimulus_ms  = PRE_STIMULUS_MS;
    config.post_stimulus_ms = POST_STIMULUS_MS;
    config.overlap_ms       = 0;
    config.trial_end_code   = 0;

    tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, &output_queue);

}

void setUp(void)
{
    memset(sig_storage,    0, sizeof(sig_storage));
    memset(sig_timestamps, 0, sizeof(sig_timestamps));
    memset(sig_indices,    0, sizeof(sig_indices));
    memset(trig_storage,   0, sizeof(trig_storage));
    memset(epoch_storage,  0, sizeof(epoch_storage));
    memset(features_storage,  0, sizeof(features_storage));
    memset(epoch_pool,     0, sizeof(epoch_pool));
    memset(features_pool,     0, sizeof(features_pool));
    memset(output_storage, 0, sizeof(output_storage));
    memset(output_pool,    0, sizeof(output_pool));
    memset(&ctx, 0, sizeof(ctx));

    setup_pipeline();
}

void tearDown(void) {}

void test_pipeline_triggered_single_epoch_training(void)
{
    config.use_decoder = true;
    setup_pipeline();
    tbci_context_start(&ctx, TBCI_STATE_TRAINING);

    push_signal_and_trigger(0, 1u);

    /* epoch consumed from features_queue by decoder in training mode */
    TEST_ASSERT_TRUE(eq_is_empty(&features_queue));

    /* nothing pushed to output_queue during training */
    TEST_ASSERT_TRUE(eq_is_empty(&output_queue));
}

void test_pipeline_triggered_single_epoch_inference(void)
{
    config.use_decoder = true;
    setup_pipeline();
    tbci_context_start(&ctx, TBCI_STATE_INFERENCE);
    push_signal_and_trigger(0, 1u);

    TEST_ASSERT_FALSE(eq_is_empty(&output_queue));

    TBCI_Epoch epoch;
    eq_pop(&output_queue, &epoch);
    TEST_ASSERT_EQUAL_UINT16(1u, epoch.label);
    TEST_ASSERT_EQUAL_size_t(TOTAL_FRAMES, epoch.n_frames);
    TEST_ASSERT_EQUAL_size_t(SIG_CHANNELS, epoch.n_channels);
}

void test_pipeline_triggered_single_epoch_decoder_disabled(void)
{
    config.use_decoder = false;
    setup_pipeline();
    tbci_context_start(&ctx, TBCI_STATE_TRAINING);

    push_signal_and_trigger(0, 1u);

    /* disabled decoder — epoch passes through to output_queue regardless of state */
    TEST_ASSERT_FALSE(eq_is_empty(&output_queue));

    TBCI_Epoch epoch;
    eq_pop(&output_queue, &epoch);
    TEST_ASSERT_EQUAL_UINT16(1u, epoch.label);
    TEST_ASSERT_EQUAL_size_t(TOTAL_FRAMES, epoch.n_frames);
    TEST_ASSERT_EQUAL_size_t(SIG_CHANNELS, epoch.n_channels);
}

/* ============================================================
 * GROUP 2 — Command dispatch via in_push_trigger
 * ============================================================ */

void test_pipeline_command_train_starts_training(void)
{
    TBCI_Trigger cmd = { .timestamp_us = 0u, .code = 194u };
    TBCI_Status s = in_push_trigger(&inputs, &cmd, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL(TBCI_STATE_TRAINING, ctx.state);
    TEST_ASSERT_TRUE(tq_is_empty(inputs.triggers));
}

void test_pipeline_command_inference_starts_inference(void)
{
    TBCI_Trigger cmd = { .timestamp_us = 0u, .code = 193u };
    TBCI_Status s = in_push_trigger(&inputs, &cmd, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL(TBCI_STATE_INFERENCE, ctx.state);
    TEST_ASSERT_TRUE(tq_is_empty(inputs.triggers));
}

void test_pipeline_command_stop_returns_to_idle(void)
{
    tbci_context_start(&ctx, TBCI_STATE_TRAINING);
    TBCI_Trigger cmd = { .timestamp_us = 0u, .code = 192u };
    TBCI_Status s = in_push_trigger(&inputs, &cmd, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL(TBCI_STATE_IDLE, ctx.state);
}

void test_pipeline_command_train_then_stop_returns_idle(void)
{
    TBCI_Trigger train = { .timestamp_us = 0u, .code = 194u };
    in_push_trigger(&inputs, &train, &ctx);
    TEST_ASSERT_EQUAL(TBCI_STATE_TRAINING, ctx.state);

    TBCI_Trigger stop = { .timestamp_us = 0u, .code = 192u };
    in_push_trigger(&inputs, &stop, &ctx);
    TEST_ASSERT_EQUAL(TBCI_STATE_IDLE, ctx.state);
}

void test_pipeline_data_trigger_not_dispatched_as_command(void)
{
    TBCI_Trigger t = { .timestamp_us = 0u, .code = 42u };
    in_push_trigger(&inputs, &t, &ctx);
    TEST_ASSERT_EQUAL(TBCI_STATE_IDLE, ctx.state);  // state unchanged
    TEST_ASSERT_FALSE(tq_is_empty(inputs.triggers)); // went to queue
}

/* ============================================================
 * GROUP 3 — Label encoder integration
 * ============================================================ */


static void setup_pipeline_with_encoder(bool binarize)
{
    setup_pipeline();
    le_cfg.binarize_target = binarize;
    le_init(&encoder, &le_cfg);
    group_add_node(&ctx.decoder.group, (TBCI_Node *)&encoder);
}

void test_pipeline_encoder_encodes_label_before_model(void)
{
    config.use_decoder = true;
    setup_pipeline_with_encoder(false);
    tbci_context_start(&ctx, TBCI_STATE_INFERENCE);

    push_signal_and_trigger(0, 1u);

    TEST_ASSERT_FALSE(eq_is_empty(&output_queue));
    TBCI_Epoch epoch;
    eq_pop(&output_queue, &epoch);

    TEST_ASSERT_EQUAL(TBCI_STATE_INFERENCE, ctx.state);
    /* label should be decoded back to trigger code domain */
    TEST_ASSERT_EQUAL_UINT16(1u, epoch.label);
    /* encoded_label should be 0 (1 - 1) */
    TEST_ASSERT_EQUAL_UINT16(0u, epoch.encoded_label);
}

void test_pipeline_encoder_decodes_label_after_inference(void)
{
    config.use_decoder = true;
    setup_pipeline_with_encoder(false);
    tbci_context_start(&ctx, TBCI_STATE_INFERENCE);

    push_signal_and_trigger(0, 5u);  /* trigger code 5 */

    TBCI_Epoch epoch;
    eq_pop(&output_queue, &epoch);
    /* after decode, label back in trigger code domain */
    TEST_ASSERT_EQUAL_UINT16(5u, epoch.label);
    TEST_ASSERT_EQUAL_UINT16(4u, epoch.encoded_label);
}


/* ============================================================
 * GROUP 5 — Add node
 * ============================================================ */

void test_add_node_valid_returns_ok(void)
{
    static TBCI_Node node = {0};
    node.name    = "test_node";
    node.enabled = true;
    node.type    = TBCI_NODE_TYPE_PREPROCESSING;

    TBCI_Status status = group_add_node(&ctx.preprocessing.group, &node);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}

void test_add_node_increments_n_nodes(void)
{
    static TBCI_Node node = {0};
    node.name    = "test_node_preprocessing";
    node.enabled = true;
    node.type    = TBCI_NODE_TYPE_PREPROCESSING;

    static TBCI_Node node2 = {0};
    node2.name    = "test_node_features";
    node2.enabled = true;
    node2.type    = TBCI_NODE_TYPE_FEATURE_EXTRACTION;

    TBCI_Status status = group_add_node(&ctx.preprocessing.group, &node);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    status = group_add_node(&ctx.features.group, &node2);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_size_t(1, ctx.preprocessing.group.n_nodes);
    TEST_ASSERT_EQUAL_size_t(1, ctx.features.group.n_nodes);
}



void test_add_node_null_ctx_returns_invalid_arg(void)
{
    TBCI_Status status = group_add_node(&ctx.preprocessing.group, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_add_node_beyond_max_returns_full(void)
{
    static TBCI_Node node = {0};
    node.name    = "test_node_preprocessing";
    node.enabled = true;
    node.type    = TBCI_NODE_TYPE_PREPROCESSING;
    TBCI_Status status = tbci_context_add_node(&ctx, &node);
    TEST_ASSERT_EQUAL(TBCI_ERR_FULL, status);
}


int main(void)
{
    UNITY_BEGIN();

    // Group 1
    RUN_TEST(test_pipeline_triggered_single_epoch_training);
    RUN_TEST(test_pipeline_triggered_single_epoch_inference);
    RUN_TEST(test_pipeline_triggered_single_epoch_decoder_disabled);

    // Group 2
    RUN_TEST(test_pipeline_command_train_starts_training);
    RUN_TEST(test_pipeline_command_inference_starts_inference);
    RUN_TEST(test_pipeline_command_stop_returns_to_idle);
    RUN_TEST(test_pipeline_command_train_then_stop_returns_idle);
    RUN_TEST(test_pipeline_data_trigger_not_dispatched_as_command);

    // Group 3
    RUN_TEST(test_pipeline_encoder_encodes_label_before_model);
    RUN_TEST(test_pipeline_encoder_decodes_label_after_inference);

    // Group 5
    RUN_TEST(test_add_node_valid_returns_ok);
    RUN_TEST(test_add_node_increments_n_nodes);
    RUN_TEST(test_add_node_null_ctx_returns_invalid_arg);
    RUN_TEST(test_add_node_beyond_max_returns_full);

    return UNITY_END();
}