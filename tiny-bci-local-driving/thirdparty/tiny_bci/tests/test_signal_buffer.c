/**
 * @file test_signal_buffer.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for tbci_signal_buffer using the Unity test framework.
 *
 * Tests are grouped by functional area:
 *  - Initialisation
 *  - Write / Read (put / get)
 *  - Overflow behaviour
 *  - Reset
 *  - Introspection (size, capacity, is_empty, is_full, overflow_count)
 *
 * Build and run via CTest:
 *  cmake --build . && ctest --verbose
 *
 * @note Unity (https://github.com/ThrowTheSwitch/Unity) must be present at
 *       tests/unity/unity.c and tests/unity/unity.h.
 * @note This test file does NOT test thread safety.
 */

#include "unity/unity.h"
#include "../include/containers/tbci_signal_buffer.h"

#include <string.h>  /* memset */

/* --------------------------------------------------------------------------
 * Test fixtures
 * -------------------------------------------------------------------------- */

#define TEST_CAPACITY   8
#define TEST_CHANNELS   4

/* Backing storage and handle shared across tests, reset in setUp() */
static float storage[TEST_CAPACITY * TEST_CHANNELS];
static uint64_t timestamps[TEST_CAPACITY];
static uint32_t sample_indices[TEST_CAPACITY];
static TBCI_SignalBuffer buf;

/* A few sample frames for use in tests */
static const float frame_a[TEST_CHANNELS] = {1.0f, 2.0f, 3.0f, 4.0f};
static const float frame_b[TEST_CHANNELS] = {5.0f, 6.0f, 7.0f, 8.0f};
static const float frame_c[TEST_CHANNELS] = {9.0f, 10.0f, 11.0f, 12.0f};

/* --------------------------------------------------------------------------
 * Unity mandatory callbacks
 * -------------------------------------------------------------------------- */

void setUp(void)
{
    memset(storage, 0, sizeof(storage));
    memset(sample_indices, 0, sizeof(sample_indices));
    memset(timestamps, 0, sizeof(timestamps));
    memset(&buf, 0, sizeof(buf));
    /* Each test starts with a clean, initialised buffer */
    sb_init(&buf, storage, timestamps, sample_indices, TEST_CAPACITY, TEST_CHANNELS);
}

void tearDown(void)
{
    /* nothing to release — static allocation */
}

/* --------------------------------------------------------------------------
 * Helper: write n identical frames into the buffer
 * -------------------------------------------------------------------------- */
static void fill_n(const float* samples, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        sb_put(&buf, samples, (uint64_t)i * 1000u, i);
    }
}

void test_init_valid_returns_ok(void)
{
    TBCI_SignalBuffer b;
    TBCI_Status status = sb_init(&b, storage, timestamps, sample_indices, TEST_CAPACITY, TEST_CHANNELS);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}

void test_init_null_buf_returns_invalid_arg(void)
{
    TBCI_Status status = sb_init(NULL, storage, timestamps, sample_indices, TEST_CAPACITY, TEST_CHANNELS);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_null_storage_returns_invalid_arg(void)
{
    TBCI_SignalBuffer b;
    TBCI_Status status = sb_init(&b, NULL, timestamps, sample_indices, TEST_CAPACITY, TEST_CHANNELS);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_zero_capacity_returns_invalid_arg(void)
{
    float s[TEST_CHANNELS];
    TBCI_SignalBuffer b;
    TBCI_Status status = sb_init(&b, s, timestamps, sample_indices, 0, TEST_CHANNELS);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_zero_channels_returns_invalid_arg(void)
{
    float s[TEST_CAPACITY];
    TBCI_SignalBuffer b;
    TBCI_Status status = sb_init(&b, s, timestamps, sample_indices, TEST_CAPACITY, 0);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_buffer_is_empty(void)
{
    TEST_ASSERT_TRUE(sb_is_empty(&buf));
}

void test_init_size_is_zero(void)
{
    TEST_ASSERT_EQUAL_size_t(0, sb_size(&buf));
}

void test_init_capacity_matches(void)
{
    TEST_ASSERT_EQUAL_size_t(TEST_CAPACITY, sb_capacity(&buf));
}

void test_init_overflow_count_is_zero(void)
{
    TEST_ASSERT_EQUAL_UINT32(0, sb_overflow_count(&buf));
}

/* ============================================================
 * GROUP 2 — Put / Get round-trip
 * ============================================================ */

void test_put_single_frame_returns_ok(void)
{
    TBCI_Status status = sb_put(&buf, frame_a, 1000u, 0u);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}

void test_put_null_buf_returns_invalid_arg(void)
{
    TBCI_Status status = sb_put(NULL, frame_a, 0u, 0u);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_put_null_samples_returns_invalid_arg(void)
{
    TBCI_Status status = sb_put(&buf, NULL, 0u, 0u);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_get_single_frame_samples_match(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    sb_put(&buf, frame_a, 1000u, 42u);
    TBCI_Status status = sb_pop(&buf, out, &frame);

    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_a, out, TEST_CHANNELS);
}

void test_get_single_frame_timestamp_matches(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    sb_put(&buf, frame_a, 9999u, 0u);
    sb_pop(&buf, out, &frame);

    TEST_ASSERT_EQUAL_UINT64(9999u, frame.timestamp_us);
}

void test_get_single_frame_index_matches(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    sb_put(&buf, frame_a, 0u, 77u);
    sb_pop(&buf, out, &frame);

    TEST_ASSERT_EQUAL_UINT32(77u, frame.sample_index);
}

void test_get_on_empty_returns_empty(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;
    TBCI_Status status = sb_pop(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_ERR_EMPTY, status);
}

void test_get_null_buf_returns_invalid_arg(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;
    TBCI_Status status = sb_pop(NULL, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_get_null_samples_out_returns_invalid_arg(void)
{
    TBCI_Frame frame;
    sb_put(&buf, frame_a, 0u, 0u);
    TBCI_Status status = sb_pop(&buf, NULL, &frame);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_get_null_frame_out_returns_invalid_arg(void)
{
    float out[TEST_CHANNELS];
    sb_put(&buf, frame_a, 0u, 0u);
    TBCI_Status status = sb_pop(&buf, out, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_fifo_order_preserved(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);
    sb_put(&buf, frame_c, 3000u, 2u);

    sb_pop(&buf, out, &frame);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_a, out, TEST_CHANNELS);

    sb_pop(&buf, out, &frame);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_b, out, TEST_CHANNELS);

    sb_pop(&buf, out, &frame);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_c, out, TEST_CHANNELS);
}

void test_fill_to_capacity_all_frames_recoverable(void)
{
    /* Write TEST_CAPACITY distinct frames */
    for (size_t i = 0; i < TEST_CAPACITY; i++)
    {
        float f[TEST_CHANNELS];
        for (size_t ch = 0; ch < TEST_CHANNELS; ch++)
        {
            f[ch] = (float)(i * TEST_CHANNELS + ch);
        }
        sb_put(&buf, f, (uint64_t)i, (uint32_t)i);
    }

    TEST_ASSERT_TRUE(sb_is_full(&buf));

    /* Read them back and verify order and values */
    for (size_t i = 0; i < TEST_CAPACITY; i++)
    {
        float out[TEST_CHANNELS];
        TBCI_Frame frame;
        sb_pop(&buf, out, &frame);

        for (size_t ch = 0; ch < TEST_CHANNELS; ch++)
        {
            TEST_ASSERT_EQUAL_FLOAT((float)(i * TEST_CHANNELS + ch), out[ch]);
        }
        TEST_ASSERT_EQUAL_UINT32((uint32_t)i, frame.sample_index);
    }
}

/* ============================================================
 * GROUP 3 — Overflow behaviour
 * ============================================================ */

void test_overflow_returns_overflow_status(void)
{
    fill_n(frame_a, TEST_CAPACITY); /* fill exactly */
    TBCI_Status status = sb_put(&buf, frame_b, 9999u, 99u);
    TEST_ASSERT_EQUAL(TBCI_ERR_OVERFLOW, status);
}

void test_overflow_increments_counter(void)
{
    fill_n(frame_a, TEST_CAPACITY);
    sb_put(&buf, frame_b, 0u, 0u);
    TEST_ASSERT_EQUAL_UINT32(1, sb_overflow_count(&buf));
}

void test_overflow_multiple_increments_counter(void)
{
    fill_n(frame_a, TEST_CAPACITY);
    sb_put(&buf, frame_b, 0u, 0u);
    sb_put(&buf, frame_b, 0u, 1u);
    sb_put(&buf, frame_b, 0u, 2u);
    TEST_ASSERT_EQUAL_UINT32(3, sb_overflow_count(&buf));
}

void test_overflow_oldest_frame_is_overwritten(void)
{
    /* Fill buffer with frame_a frames indexed 0..N-1 */
    fill_n(frame_a, TEST_CAPACITY);

    /* Overwrite: push frame_b with index 99 — oldest frame_a[0] should be lost */
    sb_put(&buf, frame_b, 9999u, 99u);

    /* First frame out should now be frame_a[1], not frame_a[0] */
    float out[TEST_CHANNELS];
    TBCI_Frame frame;
    sb_pop(&buf, out, &frame);

    TEST_ASSERT_EQUAL_UINT32(1u, frame.sample_index); /* index 0 was overwritten */
}

void test_overflow_newest_frame_is_readable(void)
{
    fill_n(frame_a, TEST_CAPACITY);
    sb_put(&buf, frame_b, 9999u, 99u);

    TEST_ASSERT_EQUAL_size_t(TEST_CAPACITY, sb_size(&buf));

    /* Drain all frames; the last one should be frame_b */
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    for (size_t i = 0; i < TEST_CAPACITY - 1; i++)
    {
        TBCI_Status status = sb_pop(&buf, out, &frame);
        TEST_ASSERT_EQUAL(TBCI_OK, status);
    }
    TBCI_Status status = sb_pop(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_b, out, TEST_CHANNELS);
}

/* ============================================================
 * GROUP 4 — Reset
 * ============================================================ */

void test_reset_buffer_is_empty(void)
{
    fill_n(frame_a, 3);
    sb_reset(&buf);
    TEST_ASSERT_TRUE(sb_is_empty(&buf));
}

void test_reset_size_is_zero(void)
{
    fill_n(frame_a, 3);
    sb_reset(&buf);
    TEST_ASSERT_EQUAL_size_t(0, sb_size(&buf));
}

void test_reset_overflow_count_is_zero(void)
{
    fill_n(frame_a, TEST_CAPACITY);
    sb_put(&buf, frame_b, 0u, 0u); /* trigger overflow */
    sb_reset(&buf);
    TEST_ASSERT_EQUAL_UINT32(0, sb_overflow_count(&buf));
}

void test_reset_buffer_is_usable_after_reset(void)
{
    fill_n(frame_a, TEST_CAPACITY);
    sb_reset(&buf);

    TBCI_Status status = sb_put(&buf, frame_b, 1000u, 0u);
    TEST_ASSERT_EQUAL(TBCI_OK, status);

    float out[TEST_CHANNELS];
    TBCI_Frame frame;
    sb_pop(&buf, out, &frame);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_b, out, TEST_CHANNELS);
}

void test_reset_null_buf_returns_invalid_arg(void)
{
    TBCI_Status status = sb_reset(NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

/* ============================================================
 * GROUP 5 — Introspection
 * ============================================================ */

void test_size_increases_on_put(void)
{
    sb_put(&buf, frame_a, 0u, 0u);
    TEST_ASSERT_EQUAL_size_t(1, sb_size(&buf));

    sb_put(&buf, frame_b, 0u, 1u);
    TEST_ASSERT_EQUAL_size_t(2, sb_size(&buf));
}

void test_size_decreases_on_get(void)
{
    fill_n(frame_a, 3);
    float out[TEST_CHANNELS];
    TBCI_Frame frame;
    sb_pop(&buf, out, &frame);
    TEST_ASSERT_EQUAL_size_t(2, sb_size(&buf));
}

void test_size_at_capacity(void)
{
    fill_n(frame_a, TEST_CAPACITY);
    TEST_ASSERT_EQUAL_size_t(TEST_CAPACITY, sb_size(&buf));
}

void test_is_empty_true_on_init(void)
{
    TEST_ASSERT_TRUE(sb_is_empty(&buf));
}

void test_is_empty_false_after_put(void)
{
    sb_put(&buf, frame_a, 0u, 0u);
    TEST_ASSERT_FALSE(sb_is_empty(&buf));
}

void test_is_empty_true_after_drain(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;
    sb_put(&buf, frame_a, 0u, 0u);
    sb_pop(&buf, out, &frame);
    TEST_ASSERT_TRUE(sb_is_empty(&buf));
}

void test_is_full_false_on_init(void)
{
    TEST_ASSERT_FALSE(sb_is_full(&buf));
}

void test_is_full_true_at_capacity(void)
{
    fill_n(frame_a, TEST_CAPACITY);
    TEST_ASSERT_TRUE(sb_is_full(&buf));
}

void test_is_full_false_after_get(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;
    fill_n(frame_a, TEST_CAPACITY);
    sb_pop(&buf, out, &frame);
    TEST_ASSERT_FALSE(sb_is_full(&buf));
}

void test_capacity_unchanged_after_operations(void)
{
    fill_n(frame_a, TEST_CAPACITY);
    float out[TEST_CHANNELS];
    TBCI_Frame frame;
    sb_pop(&buf, out, &frame);
    TEST_ASSERT_EQUAL_size_t(TEST_CAPACITY, sb_capacity(&buf));
}

/* --------------------------------------------------------------------------
 * GROUP 6 — sb_find_timestamp
 * -------------------------------------------------------------------------- */

void test_find_timestamp_null_buf_returns_invalid_arg(void)
{
    size_t index;
    TBCI_MatchType match;
    TBCI_Status status = sb_find_timestamp(NULL, 1000u, &index, &match);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_find_timestamp_null_index_returns_invalid_arg(void)
{
    TBCI_MatchType match;
    TBCI_Status status = sb_find_timestamp(&buf, 1000u, NULL, &match);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_find_timestamp_null_match_returns_invalid_arg(void)
{
    size_t index;
    TBCI_Status status = sb_find_timestamp(&buf, 1000u, &index, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_find_timestamp_empty_buffer_returns_empty(void)
{
    size_t index;
    TBCI_MatchType match;
    TBCI_Status status = sb_find_timestamp(&buf, 1000u, &index, &match);
    TEST_ASSERT_EQUAL(TBCI_ERR_EMPTY, status);
}

void test_find_timestamp_exact_match(void)
{
    size_t index;
    TBCI_MatchType match;

    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);
    sb_put(&buf, frame_c, 3000u, 2u);

    TBCI_Status status = sb_find_timestamp(&buf, 2000u, &index, &match);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(TBCI_MATCH_EXACT, match);
    TEST_ASSERT_EQUAL_size_t(1, index); /* second frame, logical index 1 */
}

void test_find_timestamp_nearest_after(void)
{
    size_t index;
    TBCI_MatchType match;

    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);
    sb_put(&buf, frame_c, 3000u, 2u);

    /* 1500u falls between frame_a and frame_b — nearest after is frame_b */
    TBCI_Status status = sb_find_timestamp(&buf, 1500u, &index, &match);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(TBCI_MATCH_NEAREST, match);
    TEST_ASSERT_EQUAL_size_t(1, index); /* frame_b at logical index 1 */
}

void test_find_timestamp_not_yet(void)
{
    size_t index;
    TBCI_MatchType match;

    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);

    /* 9999u is beyond newest frame */
    TBCI_Status status = sb_find_timestamp(&buf, 9999u, &index, &match);
    TEST_ASSERT_EQUAL(TBCI_ERR_NOT_YET, status);
}

void test_find_timestamp_exact_first_frame(void)
{
    size_t index;
    TBCI_MatchType match;

    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);

    TBCI_Status status = sb_find_timestamp(&buf, 1000u, &index, &match);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(TBCI_MATCH_EXACT, match);
    TEST_ASSERT_EQUAL_size_t(0, index);
}

void test_find_timestamp_exact_last_frame(void)
{
    size_t index;
    TBCI_MatchType match;

    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);
    sb_put(&buf, frame_c, 3000u, 2u);

    TBCI_Status status = sb_find_timestamp(&buf, 3000u, &index, &match);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(TBCI_MATCH_EXACT, match);
    TEST_ASSERT_EQUAL_size_t(2, index);
}

void test_find_timestamp_nearest_after_wrapped_buffer(void)
{
    /* Fill buffer, consume some frames, then add new ones to cause wrap */
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    fill_n(frame_a, TEST_CAPACITY); /* fill completely    */
    sb_pop(&buf, out, &frame); /* pop oldest         */
    sb_pop(&buf, out, &frame); /* pop second oldest  */
    sb_put(&buf, frame_b, 9000u, 8u); /* push — head wraps  */
    sb_put(&buf, frame_c, 10000u, 9u);

    size_t index;
    TBCI_MatchType match;
    TBCI_Status status = sb_find_timestamp(&buf, 9500u, &index, &match);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(TBCI_MATCH_NEAREST, match);
}

/* --------------------------------------------------------------------------
 * GROUP 7 — sb_frames_available_from
 * -------------------------------------------------------------------------- */

void test_frames_available_null_buf_returns_invalid_arg(void)
{
    size_t count;
    TBCI_Status status = sb_frames_available_from(NULL, 0, &count);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_frames_available_null_count_returns_invalid_arg(void)
{
    TBCI_Status status = sb_frames_available_from(&buf, 0, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_frames_available_from_start(void)
{
    size_t count;
    fill_n(frame_a, 4);
    sb_frames_available_from(&buf, 0, &count);
    TEST_ASSERT_EQUAL_size_t(4, count);
}

void test_frames_available_from_middle(void)
{
    size_t count;
    fill_n(frame_a, 4);
    sb_frames_available_from(&buf, 2, &count);
    TEST_ASSERT_EQUAL_size_t(2, count);
}

void test_frames_available_from_last(void)
{
    size_t count;
    fill_n(frame_a, 4);
    sb_frames_available_from(&buf, 3, &count);
    TEST_ASSERT_EQUAL_size_t(1, count);
}

/* --------------------------------------------------------------------------
 * GROUP 8 — sb_read_from
 * -------------------------------------------------------------------------- */

void test_read_from_null_buf_returns_invalid_arg(void)
{
    float out[TEST_CHANNELS * 2];
    TBCI_Status status = sb_read_from(NULL, 0, 2, out, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_read_from_null_samples_returns_invalid_arg(void)
{
    TBCI_Status status = sb_read_from(&buf, 0, 2, NULL, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_read_from_zero_frames_returns_invalid_arg(void)
{
    float out[TEST_CHANNELS];
    TBCI_Status status = sb_read_from(&buf, 0, 0, out, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_read_from_empty_returns_empty(void)
{
    float out[TEST_CHANNELS];
    TBCI_Status status = sb_read_from(&buf, 0, 1, out, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_EMPTY, status);
}

void test_read_from_does_not_consume_frames(void)
{
    fill_n(frame_a, 3);
    float out[TEST_CHANNELS * 3];
    sb_read_from(&buf, 0, 3, out, NULL);
    TEST_ASSERT_EQUAL_size_t(3, sb_size(&buf));
}

void test_read_from_samples_match(void)
{
    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);
    sb_put(&buf, frame_c, 3000u, 2u);

    float out[TEST_CHANNELS * 3];
    TBCI_Status status = sb_read_from(&buf, 0, 3, out, NULL);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_a, &out[0 * TEST_CHANNELS], TEST_CHANNELS);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_b, &out[1 * TEST_CHANNELS], TEST_CHANNELS);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_c, &out[2 * TEST_CHANNELS], TEST_CHANNELS);
}

void test_read_from_middle_samples_match(void)
{
    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);
    sb_put(&buf, frame_c, 3000u, 2u);

    float out[TEST_CHANNELS * 2];
    TBCI_Status status = sb_read_from(&buf, 1, 2, out, NULL);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_b, &out[0 * TEST_CHANNELS], TEST_CHANNELS);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_c, &out[1 * TEST_CHANNELS], TEST_CHANNELS);
}

void test_read_from_metadata_populated(void)
{
    sb_put(&buf, frame_a, 1000u, 42u);
    sb_put(&buf, frame_b, 2000u, 43u);

    float out[TEST_CHANNELS * 2];
    TBCI_Frame frames[2];
    sb_read_from(&buf, 0, 2, out, frames);

    TEST_ASSERT_EQUAL_UINT64(1000u, frames[0].timestamp_us);
    TEST_ASSERT_EQUAL_UINT32(42u, frames[0].sample_index);
    TEST_ASSERT_EQUAL_UINT64(2000u, frames[1].timestamp_us);
    TEST_ASSERT_EQUAL_UINT32(43u, frames[1].sample_index);
}

/* ============================================================
 * GROUP 9 - Peek Latest
 * ============================================================ */

void test_sb_peek_latest_returns_ok_when_not_empty(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    sb_put(&buf, frame_a, 1000u, 0u);

    TBCI_Status status = sb_peek_latest(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}

void test_sb_peek_latest_returns_empty_when_buffer_empty(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    TBCI_Status status = sb_peek_latest(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_ERR_EMPTY, status);
}

void test_sb_peek_latest_null_buf_returns_invalid_arg(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    TBCI_Status status = sb_peek_latest(NULL, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_sb_peek_latest_null_samples_out_returns_invalid_arg(void)
{
    TBCI_Frame frame;

    TBCI_Status status = sb_peek_latest(&buf, NULL, &frame);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_sb_peek_latest_null_frame_out_returns_invalid_arg(void)
{
    float out[TEST_CHANNELS];

    TBCI_Status status = sb_peek_latest(&buf, out, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_sb_peek_latest_returns_most_recently_written_frame(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);
    sb_put(&buf, frame_c, 3000u, 2u);

    sb_peek_latest(&buf, out, &frame);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_c, out, TEST_CHANNELS);
}

void test_sb_peek_latest_does_not_consume_frame(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    sb_put(&buf, frame_a, 1000u, 0u);

    TBCI_Status status = sb_peek_latest(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_a, out, TEST_CHANNELS);
}

void test_sb_peek_latest_size_unchanged_after_call(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;
    size_t size;

    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);
    sb_put(&buf, frame_c, 3000u, 2u);

    TBCI_Status status = sb_peek_latest(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_OK, status);

    size = sb_size(&buf);
    TEST_ASSERT_EQUAL_UINT32(3u, size);
}

void test_sb_peek_latest_head_tail_unchanged_after_call(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);
    sb_put(&buf, frame_c, 3000u, 2u);

    size_t head = buf.head;
    size_t tail = buf.tail;

    TBCI_Status status = sb_peek_latest(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_OK, status);

    TEST_ASSERT_EQUAL_UINT32(head, buf.head);
    TEST_ASSERT_EQUAL_UINT32(tail, buf.tail);
}

void test_sb_peek_latest_samples_match_last_put(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);
    sb_put(&buf, frame_c, 3000u, 2u);

    TBCI_Status status = sb_peek_latest(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_OK, status);

    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_c, out, TEST_CHANNELS);
}

void test_sb_peek_latest_timestamp_matches_last_put(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);
    sb_put(&buf, frame_c, 3000u, 2u);

    TBCI_Status status = sb_peek_latest(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_OK, status);

    TEST_ASSERT_EQUAL(3000u, frame.timestamp_us);
}

void test_sb_peek_latest_sample_index_matches_last_put(void)
{
    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);
    sb_put(&buf, frame_c, 3000u, 2u);

    TBCI_Status status = sb_peek_latest(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_OK, status);

    TEST_ASSERT_EQUAL(2u, frame.sample_index);
}

void test_sb_peek_latest_after_wraparound_returns_correct_frame(void)
{
    float f[TEST_CHANNELS];

    /* fill buffer past capacity by exactly one frame, forcing one wrap */
    for (size_t i = 0; i <= TEST_CAPACITY; i++)
    {
        for (size_t ch = 0; ch < TEST_CHANNELS; ch++)
            f[ch] = (float)(i * TEST_CHANNELS + ch);

        sb_put(&buf, f, (uint64_t)(1000u + i), (uint32_t)i);
    }

    /* the last frame pushed has index TEST_CAPACITY */
    float expected[TEST_CHANNELS];
    for (size_t ch = 0; ch < TEST_CHANNELS; ch++)
        expected[ch] = (float)(TEST_CAPACITY * TEST_CHANNELS + ch);

    float out[TEST_CHANNELS];
    TBCI_Frame frame;

    TBCI_Status status = sb_peek_latest(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_OK, status);

    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, out, TEST_CHANNELS);
    TEST_ASSERT_EQUAL_UINT64(1000u + TEST_CAPACITY, frame.timestamp_us);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)TEST_CAPACITY, frame.sample_index);
}

void test_sb_peek_latest_repeated_calls_return_same_frame(void)
{
    sb_put(&buf, frame_a, 1000u, 0u);
    sb_put(&buf, frame_b, 2000u, 1u);
    sb_put(&buf, frame_c, 3000u, 2u);

    float out[TEST_CHANNELS];
    TBCI_Frame frame;
    TBCI_Status status = sb_peek_latest(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_c, out, TEST_CHANNELS);

    status = sb_peek_latest(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_c, out, TEST_CHANNELS);

    status = sb_peek_latest(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_c, out, TEST_CHANNELS);
}

void test_sb_peek_latest_after_overflow_returns_newest_not_overwritten(void)

{
    for (size_t i = 0; i < TEST_CAPACITY; i++)
    {
        float f[TEST_CHANNELS];
        for (size_t ch = 0; ch < TEST_CHANNELS; ch++)
        {
            f[ch] = (float)(i * TEST_CHANNELS + ch);
        }
        sb_put(&buf, f, (uint64_t)i, (uint32_t)i);
    }

    float out[TEST_CHANNELS];
    TBCI_Frame frame;
    sb_put(&buf, frame_a, 1000u, 0u);

    TBCI_Status status = sb_peek_latest(&buf, out, &frame);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(frame_a, out, TEST_CHANNELS);
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void)
{
    UNITY_BEGIN();

    /* Init */
    RUN_TEST(test_init_valid_returns_ok);
    RUN_TEST(test_init_null_buf_returns_invalid_arg);
    RUN_TEST(test_init_null_storage_returns_invalid_arg);
    RUN_TEST(test_init_zero_capacity_returns_invalid_arg);
    RUN_TEST(test_init_zero_channels_returns_invalid_arg);
    RUN_TEST(test_init_buffer_is_empty);
    RUN_TEST(test_init_size_is_zero);
    RUN_TEST(test_init_capacity_matches);
    RUN_TEST(test_init_overflow_count_is_zero);

    /* Put / Get */
    RUN_TEST(test_put_single_frame_returns_ok);
    RUN_TEST(test_put_null_buf_returns_invalid_arg);
    RUN_TEST(test_put_null_samples_returns_invalid_arg);
    RUN_TEST(test_get_single_frame_samples_match);
    RUN_TEST(test_get_single_frame_timestamp_matches);
    RUN_TEST(test_get_single_frame_index_matches);
    RUN_TEST(test_get_on_empty_returns_empty);
    RUN_TEST(test_get_null_buf_returns_invalid_arg);
    RUN_TEST(test_get_null_samples_out_returns_invalid_arg);
    RUN_TEST(test_get_null_frame_out_returns_invalid_arg);
    RUN_TEST(test_fifo_order_preserved);
    RUN_TEST(test_fill_to_capacity_all_frames_recoverable);

    /* Overflow */
    RUN_TEST(test_overflow_returns_overflow_status);
    RUN_TEST(test_overflow_increments_counter);
    RUN_TEST(test_overflow_multiple_increments_counter);
    RUN_TEST(test_overflow_oldest_frame_is_overwritten);
    RUN_TEST(test_overflow_newest_frame_is_readable);

    /* Reset */
    RUN_TEST(test_reset_buffer_is_empty);
    RUN_TEST(test_reset_size_is_zero);
    RUN_TEST(test_reset_overflow_count_is_zero);
    RUN_TEST(test_reset_buffer_is_usable_after_reset);
    RUN_TEST(test_reset_null_buf_returns_invalid_arg);

    /* Introspection */
    RUN_TEST(test_size_increases_on_put);
    RUN_TEST(test_size_decreases_on_get);
    RUN_TEST(test_size_at_capacity);
    RUN_TEST(test_is_empty_true_on_init);
    RUN_TEST(test_is_empty_false_after_put);
    RUN_TEST(test_is_empty_true_after_drain);
    RUN_TEST(test_is_full_false_on_init);
    RUN_TEST(test_is_full_true_at_capacity);
    RUN_TEST(test_is_full_false_after_get);
    RUN_TEST(test_capacity_unchanged_after_operations);
    RUN_TEST(test_find_timestamp_null_buf_returns_invalid_arg);
    // Test Find Timestamp in Buffer
    RUN_TEST(test_find_timestamp_null_index_returns_invalid_arg);
    RUN_TEST(test_find_timestamp_null_index_returns_invalid_arg);
    RUN_TEST(test_find_timestamp_null_match_returns_invalid_arg);
    RUN_TEST(test_find_timestamp_empty_buffer_returns_empty);
    RUN_TEST(test_find_timestamp_exact_match);
    RUN_TEST(test_find_timestamp_nearest_after);
    RUN_TEST(test_find_timestamp_not_yet);
    RUN_TEST(test_find_timestamp_exact_first_frame);
    RUN_TEST(test_find_timestamp_exact_last_frame);
    RUN_TEST(test_find_timestamp_nearest_after_wrapped_buffer);
    // Test Available Frames
    RUN_TEST(test_frames_available_null_buf_returns_invalid_arg);
    RUN_TEST(test_frames_available_null_count_returns_invalid_arg);
    RUN_TEST(test_frames_available_from_start);
    RUN_TEST(test_frames_available_from_middle);
    RUN_TEST(test_frames_available_from_last);
    // Test Read Buffer from Frame
    RUN_TEST(test_read_from_null_buf_returns_invalid_arg);
    RUN_TEST(test_read_from_null_samples_returns_invalid_arg);
    RUN_TEST(test_read_from_zero_frames_returns_invalid_arg);
    RUN_TEST(test_read_from_empty_returns_empty);
    RUN_TEST(test_read_from_does_not_consume_frames);
    RUN_TEST(test_read_from_samples_match);
    RUN_TEST(test_read_from_middle_samples_match);
    RUN_TEST(test_read_from_metadata_populated);
    // Test Peek Latest
    RUN_TEST(test_sb_peek_latest_returns_ok_when_not_empty);
    RUN_TEST(test_sb_peek_latest_returns_empty_when_buffer_empty);
    RUN_TEST(test_sb_peek_latest_null_buf_returns_invalid_arg);
    RUN_TEST(test_sb_peek_latest_null_samples_out_returns_invalid_arg);
    RUN_TEST(test_sb_peek_latest_null_frame_out_returns_invalid_arg);
    RUN_TEST(test_sb_peek_latest_returns_most_recently_written_frame);
    RUN_TEST(test_sb_peek_latest_does_not_consume_frame);
    RUN_TEST(test_sb_peek_latest_size_unchanged_after_call);
    RUN_TEST(test_sb_peek_latest_head_tail_unchanged_after_call);
    RUN_TEST(test_sb_peek_latest_samples_match_last_put);
    RUN_TEST(test_sb_peek_latest_timestamp_matches_last_put);
    RUN_TEST(test_sb_peek_latest_sample_index_matches_last_put);
    RUN_TEST(test_sb_peek_latest_after_wraparound_returns_correct_frame);
    RUN_TEST(test_sb_peek_latest_repeated_calls_return_same_frame);
    RUN_TEST(test_sb_peek_latest_after_overflow_returns_newest_not_overwritten);

    return UNITY_END();
}
