/**
* @file test_context.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for tbci_context.c using the Unity test framework.
 *
 * Tests are grouped by functional area:
 *  - Initialisation
 *  - Start
 *  - Stop
 *  - Reset
 *  - Add Node
 *  - Tick
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
#include "../include/nodes/tbci_node.h"
#include <string.h>

/* storage for buffers */
#define SIG_CAPACITY   64
#define SIG_CHANNELS    8
#define TRIG_CAPACITY  16
#define EPOCH_CAPACITY  4
#define EPOCH_N_FRAMES 16

static float           sig_storage[SIG_CAPACITY * SIG_CHANNELS];
static uint64_t        sig_timestamps[SIG_CAPACITY];
static uint32_t        sig_indices[SIG_CAPACITY];
static TBCI_Trigger     trig_storage[TRIG_CAPACITY];
static TBCI_Epoch       epoch_storage[EPOCH_CAPACITY];
static float           epoch_pool[EPOCH_CAPACITY * EPOCH_N_FRAMES * SIG_CHANNELS];
static TBCI_Epoch      features_storage[EPOCH_CAPACITY];
static float           features_pool[EPOCH_CAPACITY * EPOCH_N_FRAMES * SIG_CHANNELS];

static TBCI_SignalBuffer signal_buf;
static TBCI_SignalBuffer proc_signal_buf;
static TBCI_TriggerQueue trigger_queue;
static TBCI_EpochQueue   epoch_queue;
static TBCI_EpochQueue   features_queue;
static TBCI_Input       inputs;
static TBCI_Config       config;
static TBCI_Context      ctx;

void setUp(void)
{
    memset(sig_storage,   0, sizeof(sig_storage));
    memset(sig_timestamps,0, sizeof(sig_timestamps));
    memset(sig_indices,   0, sizeof(sig_indices));
    memset(trig_storage,  0, sizeof(trig_storage));
    memset(epoch_storage, 0, sizeof(epoch_storage));
    memset(epoch_pool,    0, sizeof(epoch_pool));
    memset(features_storage, 0, sizeof(features_storage));
    memset(features_pool,    0, sizeof(features_pool));

    sb_init(&signal_buf, sig_storage, sig_timestamps, sig_indices, SIG_CAPACITY, SIG_CHANNELS);
    sb_init(&proc_signal_buf, sig_storage, sig_timestamps, sig_indices, SIG_CAPACITY, SIG_CHANNELS);
    tq_init(&trigger_queue, trig_storage, TRIG_CAPACITY);
    eq_init(&epoch_queue, epoch_storage, EPOCH_CAPACITY, EPOCH_N_FRAMES);
    eq_configure(&epoch_queue, epoch_pool, SIG_CHANNELS);
    eq_init(&features_queue, features_storage, EPOCH_CAPACITY, EPOCH_N_FRAMES);
    eq_configure(&features_queue, features_pool, SIG_CHANNELS);

    inputs.signal    = &signal_buf;
    inputs.triggers  = &trigger_queue;
    inputs.n_channels = SIG_CHANNELS;

    config.paradigm               = TBCI_PARADIGM_P300;
    config.nominal_srate          = 256;
    config.target_srate           = 256;
    config.n_channels             = SIG_CHANNELS;
    config.window_length_ms       = 1000;
    config.use_preprocessing      = true;
    config.use_feature_extraction = false;
    config.post_stimulus_ms       = 800;

    tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
}

void tearDown(void) {}

/* ============================================================
 * GROUP 1 — Init
 * ============================================================ */

void test_init_valid_returns_ok(void)
{
    TBCI_Status status = tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}

void test_init_null_ctx_returns_invalid_arg(void)
{
    TBCI_Status status = tbci_context_init(NULL, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_null_config_returns_invalid_arg(void)
{
    TBCI_Status status = tbci_context_init(&ctx, NULL, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_null_inputs_returns_invalid_arg(void)
{
    TBCI_Status status = tbci_context_init(&ctx, &config, NULL, &proc_signal_buf,  &epoch_queue, &features_queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_null_proc_signal_returns_invalid_arg(void)
{
    TBCI_Status status = tbci_context_init(&ctx, &config, &inputs, NULL,  &epoch_queue, &features_queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_null_epoch_queue_returns_invalid_arg(void)
{
    TBCI_Status status = tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, NULL, &features_queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}
void test_init_null_features_queue_returns_invalid_arg(void)
{
    TBCI_Status status = tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, NULL, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_zero_srate_returns_invalid_arg(void)
{
    config.nominal_srate = 0;
    TBCI_Status status = tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
    config.nominal_srate = 256;
}

void test_init_zero_target_srate_returns_invalid_arg(void)
{
    config.target_srate = 0;
    TBCI_Status status = tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
    config.target_srate = 256;
}

void test_init_zero_channels_returns_invalid_arg(void)
{
    config.n_channels = 0;
    TBCI_Status status = tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
    config.n_channels = 8;
}

void test_init_zero_window_returns_invalid_arg(void)
{
    config.window_length_ms = 0;
    TBCI_Status status = tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
    config.window_length_ms = 1000;
}

void test_init_state_is_idle(void)
{
    TBCI_Status status = tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(ctx.state, TBCI_STATE_IDLE);
}

void test_init_n_nodes(void)
{
    TBCI_Status status = tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(4, ctx.n_nodes);
}

/* ============================================================
 * GROUP 2 — Start
 * ============================================================ */

void test_start_idle_to_training_returns_ok(void)
{
    TBCI_Status status = tbci_context_start(&ctx, TBCI_STATE_TRAINING);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(ctx.state, TBCI_STATE_TRAINING);
}
void test_start_idle_to_inference_returns_ok(void)
{
    TBCI_Status status = tbci_context_start(&ctx, TBCI_STATE_INFERENCE);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(ctx.state, TBCI_STATE_INFERENCE);
}
void test_start_idle_to_idle_returns_invalid_state(void)
{
    TBCI_Status status = tbci_context_start(&ctx, TBCI_STATE_IDLE);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_STATE, status);
}
void test_start_training_to_training_returns_invalid_state(void)
{
    ctx.state = TBCI_STATE_TRAINING;
    TBCI_Status status = tbci_context_start(&ctx, TBCI_STATE_TRAINING);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_STATE, status);
    ctx.state = TBCI_STATE_IDLE;
}
void test_start_null_ctx_returns_invalid_arg(void)
{
    TBCI_Status status = tbci_context_start(NULL, TBCI_STATE_TRAINING);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

/* ============================================================
 * GROUP 3 — Stop
 * ============================================================ */

void test_stop_training_to_idle_returns_ok(void)
{
    ctx.state = TBCI_STATE_TRAINING;
    TBCI_Status status = tbci_context_stop(&ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(ctx.state, TBCI_STATE_IDLE);
}
void test_stop_inference_to_idle_returns_ok(void)
{
    ctx.state = TBCI_STATE_INFERENCE;
    TBCI_Status status = tbci_context_stop(&ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(ctx.state, TBCI_STATE_IDLE);
}
void test_stop_idle_returns_invalid_state(void)
{
    ctx.state = TBCI_STATE_IDLE;
    TBCI_Status status = tbci_context_stop(&ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_STATE, status);
}
void test_stop_null_ctx_returns_invalid_arg(void)
{
    ctx.state = TBCI_STATE_IDLE;
    TBCI_Status status = tbci_context_stop(NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

/* ============================================================
 * GROUP 4 — Reset
 * ============================================================ */

void test_reset_state_is_idle(void)
{
    TBCI_Status status = tbci_context_reset(&ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(ctx.state, TBCI_STATE_IDLE);
}

void test_reset_signal_buffer_is_empty(void)
{
    TBCI_Status status = tbci_context_reset(&ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT(sb_is_empty(ctx.inputs->signal));
}
void test_reset_trigger_queue_is_empty(void)
{
    TBCI_Status status = tbci_context_reset(&ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT(tq_is_empty(ctx.inputs->triggers));
}

void test_reset_epoch_queue_is_empty(void)
{
    TBCI_Status status = tbci_context_reset(&ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT(eq_is_empty(ctx.epoch_queue));
}

void test_reset_null_ctx_returns_invalid_arg(void)
{
    TBCI_Status status = tbci_context_reset(NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}



/* ============================================================
 * GROUP 6 — Tick
 * ============================================================ */

void test_tick_null_ctx_returns_invalid_arg(void)
{
    TBCI_Status status = tbci_context_tick(NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_tick_idle_returns_ok(void)
{
    TBCI_Status status = tbci_context_tick(&ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}

void test_tick_training_returns_ok(void)
{
    ctx.state = TBCI_STATE_TRAINING;
    TBCI_Status status = tbci_context_tick(&ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}

int main(void)
{
    UNITY_BEGIN();

    // Group 1
    RUN_TEST(test_init_valid_returns_ok);
    RUN_TEST(test_init_null_config_returns_invalid_arg);
    RUN_TEST(test_init_null_inputs_returns_invalid_arg);
    RUN_TEST(test_init_null_proc_signal_returns_invalid_arg);
    RUN_TEST(test_init_null_epoch_queue_returns_invalid_arg);
    RUN_TEST(test_init_null_features_queue_returns_invalid_arg);
    RUN_TEST(test_init_zero_srate_returns_invalid_arg);
    RUN_TEST(test_init_zero_target_srate_returns_invalid_arg);
    RUN_TEST(test_init_zero_channels_returns_invalid_arg);
    RUN_TEST(test_init_zero_window_returns_invalid_arg);
    RUN_TEST(test_init_state_is_idle);
    RUN_TEST(test_init_n_nodes);
    RUN_TEST(test_init_null_ctx_returns_invalid_arg);

    // Group 2
    RUN_TEST(test_start_idle_to_training_returns_ok);
    RUN_TEST(test_start_idle_to_inference_returns_ok);
    RUN_TEST(test_start_idle_to_idle_returns_invalid_state);
    RUN_TEST(test_start_training_to_training_returns_invalid_state);
    RUN_TEST(test_start_null_ctx_returns_invalid_arg);

    // Group 3
    RUN_TEST(test_stop_training_to_idle_returns_ok);
    RUN_TEST(test_stop_inference_to_idle_returns_ok);
    RUN_TEST(test_stop_idle_returns_invalid_state);
    RUN_TEST(test_stop_null_ctx_returns_invalid_arg);

    // Group 4
    RUN_TEST(test_reset_state_is_idle);
    RUN_TEST(test_reset_signal_buffer_is_empty);
    RUN_TEST(test_reset_trigger_queue_is_empty);
    RUN_TEST(test_reset_epoch_queue_is_empty);
    RUN_TEST(test_reset_null_ctx_returns_invalid_arg);


    // Group 6
    RUN_TEST(test_tick_null_ctx_returns_invalid_arg);
    RUN_TEST(test_tick_idle_returns_ok);
    RUN_TEST(test_tick_training_returns_ok);

    return UNITY_END();
}