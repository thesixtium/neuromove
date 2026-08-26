/**
 * @file test_raw_out.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for TBCI_RawOutNode.
 */

#include "unity/unity.h"
#include "nodes/core/tbci_raw_out.h"
#include "tbci_context.h"
#include <string.h>
#include <stdio.h>

#define N_CHANNELS   4
#define SIG_CAPACITY 64

/* --------------------------------------------------------------------------
 * Static storage
 * -------------------------------------------------------------------------- */

static TBCI_RawOutNode   node;
static TBCI_CoreConfig   config;
static TBCI_Context      ctx;
static TBCI_Input        inputs;
static TBCI_SignalBuffer signal_buf;

static float        sig_storage   [SIG_CAPACITY * N_CHANNELS];
static uint64_t     sig_timestamps[SIG_CAPACITY];
static uint32_t     sig_indices   [SIG_CAPACITY];

/* tap tracking */
static size_t    tap_call_count;
static uint16_t  tap_last_trigger;
static uint64_t  tap_last_ts;

static void test_tap_cb(const float *samples, size_t n_samples,
                         const TBCI_Frame *frame, uint16_t trigger_val,
                         void *user_data)
{
    (void)samples; (void)n_samples; (void)user_data;
    tap_call_count++;
    tap_last_trigger = trigger_val;
    tap_last_ts      = frame->timestamp_us;
}

static TBCI_SyncResult make_sync(uint16_t code, uint64_t ts_us)
{
    TBCI_SyncResult s = {0};
    s.trigger.code         = code;
    s.trigger.timestamp_us = ts_us;
    s.new_trigger = true;
    return s;
}

static void push_frame(uint64_t ts_us, uint32_t idx)
{
    float samples[N_CHANNELS] = {0.1f, 0.2f, 0.3f, 0.4f};
    sb_put(&signal_buf, samples, ts_us, idx);
}

/* --------------------------------------------------------------------------
 * setUp / tearDown
 * -------------------------------------------------------------------------- */

void setUp(void)
{
    memset(&node,        0, sizeof(node));
    memset(&config,      0, sizeof(config));
    memset(&ctx,         0, sizeof(ctx));
    memset(&inputs,      0, sizeof(inputs));
    memset(sig_storage,  0, sizeof(sig_storage));
    memset(sig_timestamps, 0, sizeof(sig_timestamps));
    memset(sig_indices,  0, sizeof(sig_indices));

    tap_call_count   = 0;
    tap_last_trigger = 0;
    tap_last_ts      = 0;

    sb_init(&signal_buf, sig_storage, sig_timestamps, sig_indices,
            SIG_CAPACITY, N_CHANNELS);

    inputs.signal     = &signal_buf;
    inputs.n_channels = N_CHANNELS;
    ctx.inputs        = &inputs;
    ctx.config.n_channels = N_CHANNELS;

    config.log_enabled  = false;  /* no file by default */
    config.log_commands = false;
    strncpy(config.log_subject, "test",    sizeof(config.log_subject) - 1);
    strncpy(config.log_session, "session", sizeof(config.log_session) - 1);
}

void tearDown(void)
{
    ro_close(&node);
    /* remove any test CSV files */
    if (node.filepath[0] != '\0')
        remove(node.filepath);
}

/* ============================================================
 * GROUP 1 — ro_init
 * ============================================================ */

void test_ro_init_ok_logging_disabled(void)
{
    TEST_ASSERT_EQUAL(TBCI_OK, ro_init(&node, &config, &ctx));
    TEST_ASSERT_NULL(node.file);
    TEST_ASSERT_EQUAL_UINT64(0, node.sample_index);
    TEST_ASSERT_EQUAL_UINT64(0, node.last_written_ts);
    TEST_ASSERT_FALSE(node.header_written);
}

void test_ro_init_ok_logging_enabled(void)
{
    config.log_enabled = true;
    TEST_ASSERT_EQUAL(TBCI_OK, ro_init(&node, &config, &ctx));
    TEST_ASSERT_NOT_NULL(node.file);
    TEST_ASSERT_TRUE(node.filepath[0] != '\0');
}

void test_ro_init_null_node(void)
{
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, ro_init(NULL, &config, &ctx));
}

void test_ro_init_null_config(void)
{
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, ro_init(&node, NULL, &ctx));
}

void test_ro_init_null_ctx(void)
{
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, ro_init(&node, &config, NULL));
}

/* ============================================================
 * GROUP 2 — ro_write: basic
 * ============================================================ */

void test_ro_write_null_node_returns_error(void)
{
    TBCI_SyncResult sync = make_sync(0, 0);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, ro_write(NULL, &sync, &ctx));
}

void test_ro_write_null_sync_returns_error(void)
{
    ro_init(&node, &config, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, ro_write(&node, NULL, &ctx));
}

void test_ro_write_empty_buffer_returns_ok(void)
{
    ro_init(&node, &config, &ctx);
    TBCI_SyncResult sync = make_sync(0, 0);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, ro_write(&node, &sync, &ctx));
    TEST_ASSERT_EQUAL_size_t(0, node.sample_index);
}

/* ============================================================
 * GROUP 3 — ro_write: frame counting
 * ============================================================ */

void test_ro_write_increments_sample_index(void)
{
    ro_init(&node, &config, &ctx);
    push_frame(1000, 0);
    TBCI_SyncResult sync = make_sync(0, 0);
    ro_write(&node, &sync, &ctx);
    TEST_ASSERT_EQUAL_UINT64(1, node.sample_index);
}

void test_ro_write_all_frames_written(void)
{
    ro_init(&node, &config, &ctx);

    /* push 5 frames */
    for (size_t i = 0; i < 5; i++)
        push_frame(1000 + i * 4000, (uint32_t)i);

    TBCI_SyncResult sync = make_sync(0, 0);
    ro_write(&node, &sync, &ctx);
    TEST_ASSERT_EQUAL_UINT64(5, node.sample_index);
}

void test_ro_write_no_duplicate_frames(void)
{
    ro_init(&node, &config, &ctx);
    push_frame(1000, 0);

    TBCI_SyncResult sync = make_sync(0, 0);
    ro_write(&node, &sync, &ctx);
    ro_write(&node, &sync, &ctx);  /* second call — no new frames */

    TEST_ASSERT_EQUAL_UINT64(1, node.sample_index);
}

void test_ro_write_updates_last_written_ts(void)
{
    ro_init(&node, &config, &ctx);
    push_frame(1000, 0);
    push_frame(5000, 1);

    TBCI_SyncResult sync = make_sync(0, 0);
    ro_write(&node, &sync, &ctx);
    TEST_ASSERT_EQUAL_UINT64(5000, node.last_written_ts);
}

/* ============================================================
 * GROUP 4 — ro_write: trigger placement
 * ============================================================ */

void test_ro_write_trigger_attached_to_correct_frame(void)
{
    ro_init(&node, &config, &ctx);
    node.on_frame = test_tap_cb;

    push_frame(1000, 0);
    push_frame(5000, 1);
    push_frame(9000, 2);

    /* trigger at ts 5000 — should only fire on frame 1 */
    TBCI_SyncResult sync = make_sync(1u, 5000);
    ro_write(&node, &sync, &ctx);

    /* tap fired 3 times total */
    TEST_ASSERT_EQUAL_size_t(3, tap_call_count);
    /* last trigger seen should be 0 (frame at 9000 has no trigger) */
    TEST_ASSERT_EQUAL_UINT16(0, tap_last_trigger);
}

void test_ro_write_no_trigger_when_code_zero(void)
{
    ro_init(&node, &config, &ctx);
    node.on_frame = test_tap_cb;

    push_frame(1000, 0);
    TBCI_SyncResult sync = make_sync(0, 0);
    sync.new_trigger = true;
    ro_write(&node, &sync, &ctx);

    TEST_ASSERT_EQUAL_UINT16(0, tap_last_trigger);
}

void test_ro_write_command_not_logged_when_disabled(void)
{
    ro_init(&node, &config, &ctx);
    node.on_frame = test_tap_cb;
    config.log_commands = false;

    push_frame(1000, 0);
    TBCI_SyncResult sync = make_sync(192u, 1000);  /* command code */
    ro_write(&node, &sync, &ctx);

    TEST_ASSERT_EQUAL_UINT16(0, tap_last_trigger);
}

void test_ro_write_command_logged_when_enabled(void)
{
    config.log_commands = true;
    ro_init(&node, &config, &ctx);
    node.on_frame = test_tap_cb;

    push_frame(1000, 0);
    TBCI_SyncResult sync = make_sync(192u, 1000);
    ro_write(&node, &sync, &ctx);

    TEST_ASSERT_EQUAL_UINT16(192u, tap_last_trigger);
}

/* ============================================================
 * GROUP 5 — tap
 * ============================================================ */

void test_ro_tap_fires_per_frame(void)
{
    ro_init(&node, &config, &ctx);
    node.on_frame = test_tap_cb;

    for (size_t i = 0; i < 4; i++)
        push_frame(1000 + i * 4000, (uint32_t)i);

    TBCI_SyncResult sync = make_sync(0, 0);
    ro_write(&node, &sync, &ctx);

    TEST_ASSERT_EQUAL_size_t(4, tap_call_count);
}

void test_ro_tap_not_called_when_null(void)
{
    ro_init(&node, &config, &ctx);
    node.on_frame = NULL;

    push_frame(1000, 0);
    TBCI_SyncResult sync = make_sync(0, 0);
    ro_write(&node, &sync, &ctx);

    TEST_ASSERT_EQUAL_size_t(0, tap_call_count);
}

void test_ro_tap_fires_even_when_logging_disabled(void)
{
    config.log_enabled = false;
    ro_init(&node, &config, &ctx);
    node.on_frame = test_tap_cb;

    push_frame(1000, 0);
    TBCI_SyncResult sync = make_sync(0, 0);
    ro_write(&node, &sync, &ctx);

    TEST_ASSERT_EQUAL_size_t(1, tap_call_count);
}

/* ============================================================
 * GROUP 6 — ro_close
 * ============================================================ */

void test_ro_close_ok(void)
{
    config.log_enabled = true;
    ro_init(&node, &config, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, ro_close(&node));
    TEST_ASSERT_NULL(node.file);
}

void test_ro_close_null_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, ro_close(NULL));
}

void test_ro_close_safe_when_already_closed(void)
{
    config.log_enabled = true;
    ro_init(&node, &config, &ctx);
    ro_close(&node);
    TEST_ASSERT_EQUAL(TBCI_OK, ro_close(&node));  /* second close safe */
}

/* --------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    // Group 1 — init
    RUN_TEST(test_ro_init_ok_logging_disabled);
    RUN_TEST(test_ro_init_ok_logging_enabled);
    RUN_TEST(test_ro_init_null_node);
    RUN_TEST(test_ro_init_null_config);
    RUN_TEST(test_ro_init_null_ctx);

    // Group 2 — write basic
    RUN_TEST(test_ro_write_null_node_returns_error);
    RUN_TEST(test_ro_write_null_sync_returns_error);
    RUN_TEST(test_ro_write_empty_buffer_returns_ok);

    // Group 3 — frame counting
    RUN_TEST(test_ro_write_increments_sample_index);
    RUN_TEST(test_ro_write_all_frames_written);
    RUN_TEST(test_ro_write_no_duplicate_frames);
    RUN_TEST(test_ro_write_updates_last_written_ts);

    // Group 4 — trigger placement
    RUN_TEST(test_ro_write_trigger_attached_to_correct_frame);
    RUN_TEST(test_ro_write_no_trigger_when_code_zero);
    RUN_TEST(test_ro_write_command_not_logged_when_disabled);
    RUN_TEST(test_ro_write_command_logged_when_enabled);

    // Group 5 — tap
    RUN_TEST(test_ro_tap_fires_per_frame);
    RUN_TEST(test_ro_tap_not_called_when_null);
    RUN_TEST(test_ro_tap_fires_even_when_logging_disabled);

    // Group 6 — close
    RUN_TEST(test_ro_close_ok);
    RUN_TEST(test_ro_close_null_returns_invalid_arg);
    RUN_TEST(test_ro_close_safe_when_already_closed);

    return UNITY_END();
}