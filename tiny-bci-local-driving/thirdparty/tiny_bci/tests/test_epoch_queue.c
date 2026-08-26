/**
 * @file test_epoch_queue.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for tbci_epoch_queue using the Unity test framework.
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

#include <string.h>

#include "unity/unity.h"
#include "../include/containers/tbci_epoch_queue.h"

#define EPOCH_CAPACITY  4
#define EPOCH_N_FRAMES  16
#define EPOCH_N_CHANNELS 8

static TBCI_Epoch        epoch_storage[EPOCH_CAPACITY];
static float            sample_pool[EPOCH_CAPACITY * EPOCH_N_FRAMES * EPOCH_N_CHANNELS];
static TBCI_EpochQueue   queue;

/* a few synthetic epochs */
static float samples_a[EPOCH_N_FRAMES * EPOCH_N_CHANNELS];
static float samples_b[EPOCH_N_FRAMES * EPOCH_N_CHANNELS];
static float samples_c[EPOCH_N_FRAMES * EPOCH_N_CHANNELS];

static const TBCI_Epoch epoch_a = {
    .samples      = samples_a,
    .n_frames     = EPOCH_N_FRAMES,
    .n_channels   = EPOCH_N_CHANNELS,
    .timestamp_us = 1000u,
    .label        = 1u
};

static const TBCI_Epoch epoch_b = {
    .samples      = samples_b,
    .n_frames     = EPOCH_N_FRAMES,
    .n_channels   = EPOCH_N_CHANNELS,
    .timestamp_us = 2000u,
    .label        = 2u
};

static const TBCI_Epoch epoch_c = {
    .samples      = samples_c,
    .n_frames     = EPOCH_N_FRAMES,
    .n_channels   = EPOCH_N_CHANNELS,
    .timestamp_us = 2000u,
    .label        = 3u
};

void setUp(void)
{
    memset(epoch_storage, 0, sizeof(epoch_storage));
    memset(sample_pool,   0, sizeof(sample_pool));
    memset(samples_a,     1, sizeof(samples_a));  // fill with non-zero so we can verify copies
    memset(samples_b,     2, sizeof(samples_b));
    memset(samples_c,     3, sizeof(samples_c));
    memset(&queue,        0, sizeof(queue));

    eq_init(&queue, epoch_storage, EPOCH_CAPACITY, EPOCH_N_FRAMES);
    eq_configure(&queue, sample_pool, EPOCH_N_CHANNELS);
}
void tearDown(void) {}

static void fill_n(const TBCI_Epoch *epoch, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        eq_push(&queue, epoch);
    }
}

/* ============================================================
 * INIT
 * ============================================================ */


void test_init_valid_returns_ok(void)
{
    TBCI_EpochQueue e;
    TBCI_Status status = eq_init(&e, epoch_storage, EPOCH_CAPACITY, EPOCH_N_FRAMES);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}

void test_init_null_buf_returns_invalid_arg(void)
{
    TBCI_Status status = eq_init(NULL, epoch_storage, EPOCH_CAPACITY, EPOCH_N_FRAMES);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_null_epoch_storage_returns_invalid_arg(void)
{
    TBCI_EpochQueue e;
    TBCI_Status status = eq_init(&e, NULL, EPOCH_CAPACITY, EPOCH_N_FRAMES);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_zero_capacity_returns_invalid_arg(void)
{
    TBCI_EpochQueue e;
    TBCI_Status status = eq_init(&e, epoch_storage, 0, EPOCH_N_FRAMES);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_queue_is_empty(void)
{
    TEST_ASSERT_TRUE(eq_is_empty(&queue));
}

void test_init_size_is_zero(void)
{
    TEST_ASSERT_EQUAL_size_t(0, eq_size(&queue));
}

void test_init_capacity_matches(void)
{
    TEST_ASSERT_EQUAL_size_t(EPOCH_CAPACITY, eq_capacity(&queue));
}

void test_init_zero_n_frames_returns_invalid_arg(void)
{
    TBCI_EpochQueue e;
    TBCI_Status status = eq_init(&e, epoch_storage, EPOCH_CAPACITY, 0);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

/* ============================================================
 * PUSH / POP
 * ============================================================ */

void test_push_null_queue_returns_invalid_arg(void)
{
    TBCI_Status status = eq_push(NULL, &epoch_a);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_push_null_trigger_returns_invalid_arg(void)
{
    TBCI_Status status = eq_push(&queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_push_single_returns_ok(void)
{
    TBCI_Status status = eq_push(&queue, &epoch_a);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}

void test_pop_null_queue_returns_invalid_arg(void)
{
    TBCI_Epoch out;
    TBCI_Status status = eq_pop(NULL, &out);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_pop_null_out_returns_invalid_arg(void)
{
    eq_push(&queue, &epoch_a);
    TBCI_Status status = eq_pop(&queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_pop_on_empty_returns_empty(void)
{
    TBCI_Epoch out;
    TBCI_Status status = eq_pop(&queue, &out);
    TEST_ASSERT_EQUAL(TBCI_ERR_EMPTY, status);
}

void test_pop_single_code_matches(void)
{
    TBCI_Epoch out;
    eq_push(&queue, &epoch_a);
    eq_pop(&queue, &out);
    TEST_ASSERT_EQUAL_INT(epoch_a.label, out.label);
}

void test_pop_single_timestamp_matches(void)
{
    TBCI_Epoch out;
    eq_push(&queue, &epoch_a);
    eq_pop(&queue, &out);
    TEST_ASSERT_EQUAL_UINT64(epoch_a.timestamp_us, out.timestamp_us);
}

void test_fifo_order_preserved(void)
{
    TBCI_Epoch out;

    eq_push(&queue, &epoch_a);
    eq_push(&queue, &epoch_b);
    eq_push(&queue, &epoch_c);

    eq_pop(&queue, &out);
    TEST_ASSERT_EQUAL_UINT16(epoch_a.label, out.label);

    eq_pop(&queue, &out);
    TEST_ASSERT_EQUAL_UINT16(epoch_b.label, out.label);

    eq_pop(&queue, &out);
    TEST_ASSERT_EQUAL_UINT16(epoch_c.label, out.label);
}


void test_overflow_returns_overflow_status(void)
{
    fill_n(&epoch_a, EPOCH_CAPACITY); /* fill exactly */
    TBCI_Status status = eq_push(&queue, &epoch_b);
    TEST_ASSERT_EQUAL(TBCI_ERR_FULL, status);
}

void test_fill_to_capacity_all_recoverable(void)
{
    for (uint32_t i = 0; i < EPOCH_CAPACITY; i++) {
        TBCI_Epoch e = {
            .samples      = samples_a,   // valid pointer
            .n_frames     = EPOCH_N_FRAMES,
            .n_channels   = EPOCH_N_CHANNELS,
            .timestamp_us = (uint64_t)i * 1000u,
            .label        = (uint16_t)i
        };
        eq_push(&queue, &e);
    }

    TEST_ASSERT_TRUE(eq_is_full(&queue));

    for (uint32_t i = 0; i < EPOCH_CAPACITY; i++) {
        TBCI_Epoch out;
        eq_pop(&queue, &out);
        TEST_ASSERT_EQUAL_UINT16((uint16_t)i, out.label);
    }
}

void test_push_before_configure_returns_invalid_state(void)
{
    TBCI_EpochQueue unconfigured;
    TBCI_Epoch storage[EPOCH_CAPACITY];
    unconfigured.sample_pool = NULL;  // On MSVC, it must be explicitly set to NULL
    eq_init(&unconfigured, storage, EPOCH_CAPACITY, EPOCH_N_FRAMES);
    // deliberately skip eq_configure
    TBCI_Status status = eq_push(&unconfigured, &epoch_a);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_STATE, status);
}

void test_samples_copied_correctly(void)
{
    TBCI_Epoch out;
    eq_push(&queue, &epoch_a);
    eq_pop(&queue, &out);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(samples_a, out.samples, EPOCH_N_FRAMES * EPOCH_N_CHANNELS);
}

/* ============================================================
 * RESET
 * ============================================================ */

void test_reset_buffer_is_empty(void)
{
    fill_n(&epoch_a, 3);
    eq_reset(&queue);
    TEST_ASSERT_TRUE(eq_is_empty(&queue));
}

void test_reset_size_is_zero(void)
{
    fill_n(&epoch_a, 3);
    eq_reset(&queue);
    TEST_ASSERT_EQUAL_size_t(0, eq_size(&queue));
}


void test_reset_buffer_is_usable_after_reset(void)
{
    fill_n(&epoch_a, EPOCH_CAPACITY);
    eq_reset(&queue);

    TBCI_Status status = eq_push(&queue, &epoch_b);
    TEST_ASSERT_EQUAL(TBCI_OK, status);

    TBCI_Epoch out;
    TBCI_Status pop_status = eq_pop(&queue, &out);
    TEST_ASSERT_EQUAL(TBCI_OK, pop_status);
    TEST_ASSERT_EQUAL_UINT16(epoch_b.label, out.label);
}

void test_reset_null_buf_returns_invalid_arg(void)
{
    TBCI_Status status = eq_reset(NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

/* ============================================================
 * INTROSPECTION
 * ============================================================ */

void test_size_increases_on_put(void)
{
    eq_push(&queue, &epoch_a);
    TEST_ASSERT_EQUAL_size_t(1, eq_size(&queue));

    eq_push(&queue, &epoch_b);
    TEST_ASSERT_EQUAL_size_t(2, eq_size(&queue));
}

void test_size_decreases_on_get(void)
{
    fill_n(&epoch_a, 3);
    TBCI_Epoch out;
    TBCI_Status pop_status = eq_pop(&queue, &out);
    TEST_ASSERT_EQUAL(TBCI_OK, pop_status);
    TEST_ASSERT_EQUAL_size_t(2, eq_size(&queue));
}

void test_size_at_capacity(void)
{
    fill_n(&epoch_a, EPOCH_CAPACITY);
    TEST_ASSERT_EQUAL_size_t(EPOCH_CAPACITY, eq_size(&queue));
}

void test_is_empty_true_on_init(void)
{
    TEST_ASSERT_TRUE(eq_is_empty(&queue));
}

void test_is_empty_false_after_put(void)
{
    eq_push(&queue, &epoch_a);
    TEST_ASSERT_FALSE(eq_is_empty(&queue));
}

void test_is_empty_true_after_drain(void)
{
    TBCI_Epoch out;
    eq_push(&queue, &epoch_a);
    TBCI_Status pop_status = eq_pop(&queue, &out);
    TEST_ASSERT_EQUAL(TBCI_OK, pop_status);
    TEST_ASSERT_TRUE(eq_is_empty(&queue));
}

void test_is_full_false_on_init(void)
{
    TEST_ASSERT_FALSE(eq_is_full(&queue));
}

void test_is_full_true_at_capacity(void)
{
    fill_n(&epoch_a, EPOCH_CAPACITY);
    TEST_ASSERT_TRUE(eq_is_full(&queue));
}

void test_is_full_false_after_get(void)
{
    TBCI_Epoch out;
    fill_n(&epoch_a, EPOCH_CAPACITY);
    TBCI_Status pop_status = eq_pop(&queue, &out);
    TEST_ASSERT_EQUAL(TBCI_OK, pop_status);
    TEST_ASSERT_FALSE(eq_is_full(&queue));
}

void test_capacity_unchanged_after_operations(void)
{
    fill_n(&epoch_a, EPOCH_CAPACITY);
    TBCI_Epoch out;
    TBCI_Status pop_status = eq_pop(&queue, &out);
    TEST_ASSERT_EQUAL(TBCI_OK, pop_status);
    TEST_ASSERT_EQUAL_size_t(EPOCH_CAPACITY, eq_capacity(&queue));
}

void test_eq_next_slot_returns_valid_pointer(void)
{
    TEST_ASSERT_NOT_NULL(eq_next_slot(&queue));
}

void test_eq_next_slot_returns_null_when_full(void)
{
    fill_n(&epoch_a, EPOCH_CAPACITY);
    TEST_ASSERT_NULL(eq_next_slot(&queue));
}

void test_eq_next_slot_direct_write_matches_push(void)
{
    float *slot = eq_next_slot(&queue);
    TEST_ASSERT_NOT_NULL(slot);

    // write directly into slot
    for (size_t i = 0; i < EPOCH_N_FRAMES * EPOCH_N_CHANNELS; i++) {
        slot[i] = (float)i;
    }

    TBCI_Epoch epoch = {
        .samples      = slot,
        .n_frames     = EPOCH_N_FRAMES,
        .n_channels   = EPOCH_N_CHANNELS,
        .timestamp_us = 1000u,
        .label        = 1u
    };
    eq_push(&queue, &epoch);

    TBCI_Epoch out;
    eq_pop(&queue, &out);

    for (size_t i = 0; i < EPOCH_N_FRAMES * EPOCH_N_CHANNELS; i++) {
        TEST_ASSERT_EQUAL_FLOAT((float)i, out.samples[i]);
    }
}

int main(void) {
    UNITY_BEGIN();

    /* Init */
    RUN_TEST(test_init_valid_returns_ok);
    RUN_TEST(test_init_null_buf_returns_invalid_arg);
    RUN_TEST(test_init_null_epoch_storage_returns_invalid_arg);
    RUN_TEST(test_init_zero_capacity_returns_invalid_arg);
    RUN_TEST(test_init_size_is_zero);
    RUN_TEST(test_init_capacity_matches);
    RUN_TEST(test_init_queue_is_empty);
    RUN_TEST(test_init_zero_n_frames_returns_invalid_arg);

    /* Put / Get */
    RUN_TEST(test_push_null_queue_returns_invalid_arg);
    RUN_TEST(test_push_null_trigger_returns_invalid_arg);
    RUN_TEST(test_push_single_returns_ok);
    RUN_TEST(test_pop_null_queue_returns_invalid_arg);
    RUN_TEST(test_pop_null_out_returns_invalid_arg);
    RUN_TEST(test_pop_on_empty_returns_empty);
    RUN_TEST(test_pop_single_code_matches);
    RUN_TEST(test_pop_single_timestamp_matches);
    RUN_TEST(test_fifo_order_preserved);
    RUN_TEST(test_push_before_configure_returns_invalid_state);
    RUN_TEST(test_samples_copied_correctly);

    /* Overflow */
    RUN_TEST(test_overflow_returns_overflow_status);
    RUN_TEST(test_fill_to_capacity_all_recoverable);

    /* Reset */
    RUN_TEST(test_reset_buffer_is_empty);
    RUN_TEST(test_reset_size_is_zero);
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
    RUN_TEST(test_eq_next_slot_returns_valid_pointer);
    RUN_TEST(test_eq_next_slot_returns_null_when_full);
    RUN_TEST(test_eq_next_slot_direct_write_matches_push);

    return UNITY_END();
}