/**
 * @file test_input.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for tbci_input.c using the Unity test framework.
 */

#include "unity/unity.h"
#include "../include/ioutils/tbci_input.h"
#include "tbci_context.h"
#include <string.h>

#define SIG_CAPACITY   64
#define SIG_CHANNELS    8
#define TRIG_CAPACITY  16
#define EPOCH_CAPACITY  4
#define EPOCH_N_FRAMES 16
#define TARGET_SRATE   256.0f

static float           sig_storage   [SIG_CAPACITY * SIG_CHANNELS];
static uint64_t        sig_timestamps [SIG_CAPACITY];
static uint32_t        sig_indices    [SIG_CAPACITY];
static TBCI_Trigger    trig_storage   [TRIG_CAPACITY];
static TBCI_Epoch      epoch_storage  [EPOCH_CAPACITY];
static float           epoch_pool     [EPOCH_CAPACITY * EPOCH_N_FRAMES * SIG_CHANNELS];
static TBCI_Epoch      features_storage  [EPOCH_CAPACITY];
static float           features_pool     [EPOCH_CAPACITY * EPOCH_N_FRAMES * SIG_CHANNELS];

static TBCI_SignalBuffer  signal_buf;
static TBCI_SignalBuffer  proc_signal_buf;
static TBCI_TriggerQueue  trigger_queue;
static TBCI_EpochQueue    epoch_queue;
static TBCI_EpochQueue    features_queue;
static TBCI_Input         inputs;
static TBCI_Config        config;
static TBCI_Context       ctx;

static float dummy_samples[SIG_CHANNELS];

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
    memset(dummy_samples,  0, sizeof(dummy_samples));

    sb_init(&signal_buf, sig_storage, sig_timestamps, sig_indices,SIG_CAPACITY, SIG_CHANNELS);
    sb_init(&proc_signal_buf, sig_storage, sig_timestamps, sig_indices,SIG_CAPACITY, SIG_CHANNELS);
    tq_init(&trigger_queue, trig_storage, TRIG_CAPACITY);
    eq_init(&epoch_queue, epoch_storage, EPOCH_CAPACITY, EPOCH_N_FRAMES);
    eq_init(&features_queue, features_storage, EPOCH_CAPACITY, EPOCH_N_FRAMES);
    eq_configure(&epoch_queue, epoch_pool, SIG_CHANNELS);
    eq_configure(&features_queue, features_pool, SIG_CHANNELS);

    inputs.signal     = &signal_buf;
    inputs.triggers   = &trigger_queue;
    inputs.n_channels = SIG_CHANNELS;

    config.paradigm               = TBCI_PARADIGM_P300;
    config.nominal_srate          = TARGET_SRATE;
    config.target_srate           = TARGET_SRATE;
    config.n_channels             = SIG_CHANNELS;
    config.window_length_ms       = 1000;
    config.use_preprocessing      = false;
    config.use_feature_extraction = false;
    config.mode                   = SEG_MODE_TRIGGERED;
    config.pre_stimulus_ms        = 200;
    config.post_stimulus_ms       = 800;
    config.overlap_ms             = 0;
    config.trial_end_code         = 0;

    tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
}

void tearDown(void) {}

/* ============================================================
 * GROUP 1 — in_push_signal
 * ============================================================ */

void test_push_signal_valid_returns_ok(void)
{
    TBCI_Status s = in_push_signal(&inputs, dummy_samples, 1000u, 0u);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
}

void test_push_signal_null_input_returns_invalid_arg(void)
{
    TBCI_Status s = in_push_signal(NULL, dummy_samples, 1000u, 0u);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_push_signal_null_samples_returns_invalid_arg(void)
{
    TBCI_Status s = in_push_signal(&inputs, NULL, 1000u, 0u);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_push_signal_frame_appears_in_buffer(void)
{
    in_push_signal(&inputs, dummy_samples, 1000u, 0u);
    TEST_ASSERT_EQUAL_size_t(1, sb_size(inputs.signal));
}

void test_push_signal_timestamp_stored_correctly(void)
{
    in_push_signal(&inputs, dummy_samples, 99999u, 0u);
    TEST_ASSERT_EQUAL_UINT64(99999u, sig_timestamps[0]);
}

void test_push_signal_multiple_frames(void)
{
    for (size_t i = 0; i < 10; i++)
        in_push_signal(&inputs, dummy_samples, i * 1000u, (uint32_t)i);
    TEST_ASSERT_EQUAL_size_t(10, sb_size(inputs.signal));
}

/* ============================================================
 * GROUP 2 — in_push_trigger: data class [1-191]
 * ============================================================ */

void test_push_trigger_data_class_valid_returns_ok(void)
{
    TBCI_Trigger t = { .timestamp_us = 1000u, .code = 1u };
    TBCI_Status s = in_push_trigger(&inputs, &t, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
}

void test_push_trigger_null_input_returns_invalid_arg(void)
{
    TBCI_Trigger t = { .timestamp_us = 1000u, .code = 1u };
    TBCI_Status s = in_push_trigger(NULL, &t, &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_push_trigger_null_trigger_returns_invalid_arg(void)
{
    TBCI_Status s = in_push_trigger(&inputs, NULL, &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_push_trigger_null_ctx_returns_invalid_arg(void)
{
    TBCI_Trigger t = { .timestamp_us = 1000u, .code = 1u };
    TBCI_Status s = in_push_trigger(&inputs, &t, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_push_trigger_data_class_appears_in_queue(void)
{
    TBCI_Trigger t = { .timestamp_us = 1000u, .code = 42u };
    in_push_trigger(&inputs, &t, &ctx);
    TEST_ASSERT_FALSE(tq_is_empty(inputs.triggers));
}

void test_push_trigger_data_class_code_preserved(void)
{
    TBCI_Trigger t = { .timestamp_us = 1000u, .code = 42u };
    in_push_trigger(&inputs, &t, &ctx);
    TBCI_Trigger out;
    tq_pop(inputs.triggers, &out);
    TEST_ASSERT_EQUAL_UINT16(42u, out.code);
}

void test_push_trigger_max_data_class_goes_to_queue(void)
{
    TBCI_Trigger t = { .timestamp_us = 1000u, .code = 191u };
    TBCI_Status s = in_push_trigger(&inputs, &t, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_FALSE(tq_is_empty(inputs.triggers));
}

/* ============================================================
 * GROUP 3 — in_push_trigger: reserved [0]
 * ============================================================ */

void test_push_trigger_reserved_code_returns_ok(void)
{
    TBCI_Trigger t = { .timestamp_us = 1000u, .code = 0u };
    TBCI_Status s = in_push_trigger(&inputs, &t, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
}

void test_push_trigger_reserved_code_not_pushed_to_queue(void)
{
    TBCI_Trigger t = { .timestamp_us = 1000u, .code = 0u };
    in_push_trigger(&inputs, &t, &ctx);
    TEST_ASSERT_TRUE(tq_is_empty(inputs.triggers));
}

/* ============================================================
 * GROUP 4 — in_push_trigger: commands [192-255]
 * ============================================================ */

void test_push_trigger_command_idle_stops_context(void)
{
    tbci_context_start(&ctx, TBCI_STATE_TRAINING);
    TBCI_Trigger t = { .timestamp_us = 1000u, .code = 192u };
    in_push_trigger(&inputs, &t, &ctx);
    TEST_ASSERT_EQUAL(TBCI_STATE_IDLE, ctx.state);
}

void test_push_trigger_command_inference_starts_inference(void)
{
    TBCI_Trigger t = { .timestamp_us = 1000u, .code = 193u };
    in_push_trigger(&inputs, &t, &ctx);
    TEST_ASSERT_EQUAL(TBCI_STATE_INFERENCE, ctx.state);
}

void test_push_trigger_command_train_starts_training(void)
{
    TBCI_Trigger t = { .timestamp_us = 1000u, .code = 194u };
    in_push_trigger(&inputs, &t, &ctx);
    TEST_ASSERT_EQUAL(TBCI_STATE_TRAINING, ctx.state);
}

void test_push_trigger_command_not_pushed_to_queue(void)
{
    TBCI_Trigger t = { .timestamp_us = 1000u, .code = 193u };
    in_push_trigger(&inputs, &t, &ctx);
    TEST_ASSERT_TRUE(tq_is_empty(inputs.triggers));
}

void test_push_trigger_unknown_command_returns_invalid_arg(void)
{
    TBCI_Trigger t = { .timestamp_us = 1000u, .code = 255u };
    TBCI_Status s = in_push_trigger(&inputs, &t, &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

int main(void)
{
    UNITY_BEGIN();

    // Group 1 — in_push_signal
    RUN_TEST(test_push_signal_valid_returns_ok);
    RUN_TEST(test_push_signal_null_input_returns_invalid_arg);
    RUN_TEST(test_push_signal_null_samples_returns_invalid_arg);
    RUN_TEST(test_push_signal_frame_appears_in_buffer);
    RUN_TEST(test_push_signal_timestamp_stored_correctly);
    RUN_TEST(test_push_signal_multiple_frames);

    // Group 2 — data class triggers
    RUN_TEST(test_push_trigger_data_class_valid_returns_ok);
    RUN_TEST(test_push_trigger_null_input_returns_invalid_arg);
    RUN_TEST(test_push_trigger_null_trigger_returns_invalid_arg);
    RUN_TEST(test_push_trigger_null_ctx_returns_invalid_arg);
    RUN_TEST(test_push_trigger_data_class_appears_in_queue);
    RUN_TEST(test_push_trigger_data_class_code_preserved);
    RUN_TEST(test_push_trigger_max_data_class_goes_to_queue);

    // Group 3 — reserved
    RUN_TEST(test_push_trigger_reserved_code_returns_ok);
    RUN_TEST(test_push_trigger_reserved_code_not_pushed_to_queue);

    // Group 4 — commands
    RUN_TEST(test_push_trigger_command_idle_stops_context);
    RUN_TEST(test_push_trigger_command_inference_starts_inference);
    RUN_TEST(test_push_trigger_command_train_starts_training);
    RUN_TEST(test_push_trigger_command_not_pushed_to_queue);
    RUN_TEST(test_push_trigger_unknown_command_returns_invalid_arg);

    return UNITY_END();
}