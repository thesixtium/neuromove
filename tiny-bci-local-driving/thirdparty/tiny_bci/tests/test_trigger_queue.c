/**
 * @file test_trigger_queue.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for test_trigger_queue using the Unity test framework.
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
#include "../include/containers/tbci_trigger_queue.h"
#include <string.h>

#define TEST_CAPACITY 8

static TBCI_Trigger storage[TEST_CAPACITY];
static TBCI_TriggerQueue queue;

/* a few triggers to use in tests */
static const TBCI_Trigger trigger_a = {.timestamp_us = 1000u, .code = 1u};
static const TBCI_Trigger trigger_b = {.timestamp_us = 2000u, .code = 2u};
static const TBCI_Trigger trigger_c = {.timestamp_us = 3000u, .code = 3u};

void setUp(void)
{
    memset(storage, 0, sizeof(storage));
    memset(&queue, 0, sizeof(queue));

    tq_init(&queue, storage, TEST_CAPACITY);
}

void tearDown(void)
{
}

static void fill_n(const TBCI_Trigger *trigger, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        tq_push(&queue, trigger);
    }
}

void test_init_valid_returns_ok(void)
{
    TBCI_TriggerQueue t;
    TBCI_Status status = tq_init(&t, storage, TEST_CAPACITY);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}

void test_init_null_buf_returns_invalid_arg(void)
{
    TBCI_Status status = tq_init(NULL, storage, TEST_CAPACITY);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_null_storage_returns_invalid_arg(void)
{
    TBCI_TriggerQueue t;
    TBCI_Status status = tq_init(&t, NULL, TEST_CAPACITY);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_zero_capacity_returns_invalid_arg(void)
{
    TBCI_TriggerQueue t;
    TBCI_Status status = tq_init(&t, storage, 0);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_init_queue_is_empty(void)
{
    TEST_ASSERT_TRUE(tq_is_empty(&queue));
}

void test_init_size_is_zero(void)
{
    TEST_ASSERT_EQUAL_size_t(0, tq_size(&queue));
}

void test_init_capacity_matches(void)
{
    TEST_ASSERT_EQUAL_size_t(TEST_CAPACITY, tq_capacity(&queue));
}

// PUSH / POP
void test_push_null_queue_returns_invalid_arg(void)
{
    TBCI_Status status = tq_push(NULL, &trigger_a);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_push_null_trigger_returns_invalid_arg(void)
{
    TBCI_Status status = tq_push(&queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_push_single_returns_ok(void)
{
    TBCI_Status status = tq_push(&queue, &trigger_a);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}

void test_pop_null_queue_returns_invalid_arg(void)
{
    TBCI_Trigger out;
    TBCI_Status status = tq_pop(NULL, &out);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_pop_null_out_returns_invalid_arg(void)
{
    tq_push(&queue, &trigger_a);
    TBCI_Status status = tq_pop(&queue, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

void test_pop_on_empty_returns_empty(void)
{
    TBCI_Trigger out;
    TBCI_Status status = tq_pop(&queue, &out);
    TEST_ASSERT_EQUAL(TBCI_ERR_EMPTY, status);
}

void test_pop_single_code_matches(void)
{
    TBCI_Trigger out;
    tq_push(&queue, &trigger_a);
    tq_pop(&queue, &out);
    TEST_ASSERT_EQUAL_UINT16(trigger_a.code, out.code);
}

void test_pop_single_timestamp_matches(void)
{
    TBCI_Trigger out;
    tq_push(&queue, &trigger_a);
    tq_pop(&queue, &out);
    TEST_ASSERT_EQUAL_UINT64(trigger_a.timestamp_us, out.timestamp_us);
}

void test_fifo_order_preserved(void)
{
    TBCI_Trigger out;

    tq_push(&queue, &trigger_a);
    tq_push(&queue, &trigger_b);
    tq_push(&queue, &trigger_c);

    tq_pop(&queue, &out);
    TEST_ASSERT_EQUAL_UINT16(trigger_a.code, out.code);

    tq_pop(&queue, &out);
    TEST_ASSERT_EQUAL_UINT16(trigger_b.code, out.code);

    tq_pop(&queue, &out);
    TEST_ASSERT_EQUAL_UINT16(trigger_c.code, out.code);
}


void test_overflow_returns_overflow_status(void)
{
    fill_n(&trigger_a, TEST_CAPACITY); /* fill exactly */
    TBCI_Status status = tq_push(&queue, &trigger_b);
    TEST_ASSERT_EQUAL(TBCI_ERR_FULL, status);
}

void test_fill_to_capacity_all_recoverable(void)
{
    for (uint32_t i = 0; i < TEST_CAPACITY; i++) {
        TBCI_Trigger t = { .timestamp_us = (uint64_t)i * 1000u, .code = (uint16_t)i };
        tq_push(&queue, &t);
    }

    TEST_ASSERT_TRUE(tq_is_full(&queue));

    for (uint32_t i = 0; i < TEST_CAPACITY; i++) {
        TBCI_Trigger out;
        tq_pop(&queue, &out);
        TEST_ASSERT_EQUAL_UINT16((uint16_t)i, out.code);
    }
}


/* ============================================================
 * GROUP 4 — Reset
 * ============================================================ */

void test_reset_buffer_is_empty(void)
{
    fill_n(&trigger_a, 3);
    tq_reset(&queue);
    TEST_ASSERT_TRUE(tq_is_empty(&queue));
}

void test_reset_size_is_zero(void)
{
    fill_n(&trigger_a, 3);
    tq_reset(&queue);
    TEST_ASSERT_EQUAL_size_t(0, tq_size(&queue));
}


void test_reset_buffer_is_usable_after_reset(void)
{
    fill_n(&trigger_a, TEST_CAPACITY);
    tq_reset(&queue);

    TBCI_Status status = tq_push(&queue, &trigger_b);
    TEST_ASSERT_EQUAL(TBCI_OK, status);

    TBCI_Trigger out;
    TBCI_Status pop_status = tq_pop(&queue, &out);
    TEST_ASSERT_EQUAL(TBCI_OK, pop_status);
    TEST_ASSERT_EQUAL_UINT16(trigger_b.code, out.code);
}

void test_reset_null_buf_returns_invalid_arg(void)
{
    TBCI_Status status = tq_reset(NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

/* ============================================================
 * GROUP 5 — Introspection
 * ============================================================ */

void test_size_increases_on_put(void)
{
    tq_push(&queue, &trigger_a);
    TEST_ASSERT_EQUAL_size_t(1, tq_size(&queue));

    tq_push(&queue, &trigger_b);
    TEST_ASSERT_EQUAL_size_t(2, tq_size(&queue));
}

void test_size_decreases_on_get(void)
{
    fill_n(&trigger_a, 3);
    TBCI_Trigger out;
    TBCI_Status pop_status = tq_pop(&queue, &out);
    TEST_ASSERT_EQUAL(TBCI_OK, pop_status);
    TEST_ASSERT_EQUAL_size_t(2, tq_size(&queue));
}

void test_size_at_capacity(void)
{
    fill_n(&trigger_a, TEST_CAPACITY);
    TEST_ASSERT_EQUAL_size_t(TEST_CAPACITY, tq_size(&queue));
}

void test_is_empty_true_on_init(void)
{
    TEST_ASSERT_TRUE(tq_is_empty(&queue));
}

void test_is_empty_false_after_put(void)
{
    tq_push(&queue, &trigger_a);
    TEST_ASSERT_FALSE(tq_is_empty(&queue));
}

void test_is_empty_true_after_drain(void)
{
    TBCI_Trigger out;
    tq_push(&queue, &trigger_a);
    TBCI_Status pop_status = tq_pop(&queue, &out);
    TEST_ASSERT_EQUAL(TBCI_OK, pop_status);
    TEST_ASSERT_TRUE(tq_is_empty(&queue));
}

void test_is_full_false_on_init(void)
{
    TEST_ASSERT_FALSE(tq_is_full(&queue));
}

void test_is_full_true_at_capacity(void)
{
    fill_n(&trigger_a, TEST_CAPACITY);
    TEST_ASSERT_TRUE(tq_is_full(&queue));
}

void test_is_full_false_after_get(void)
{
    TBCI_Trigger out;
    fill_n(&trigger_a, TEST_CAPACITY);
    TBCI_Status pop_status = tq_pop(&queue, &out);
    TEST_ASSERT_EQUAL(TBCI_OK, pop_status);
    TEST_ASSERT_FALSE(tq_is_full(&queue));
}

void test_capacity_unchanged_after_operations(void)
{
    fill_n(&trigger_a, TEST_CAPACITY);
    TBCI_Trigger out;
    TBCI_Status pop_status = tq_pop(&queue, &out);
    TEST_ASSERT_EQUAL(TBCI_OK, pop_status);
    TEST_ASSERT_EQUAL_size_t(TEST_CAPACITY, tq_capacity(&queue));
}

void test_tq_peek_returns_front_without_consuming(void)
{
    tq_push(&queue, &trigger_a);
    tq_push(&queue, &trigger_b);

    TBCI_Trigger out;
    TBCI_Status status = tq_peek(&queue, &out);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_UINT16(trigger_a.code, out.code);
    TEST_ASSERT_EQUAL_size_t(2, tq_size(&queue));  // queue unchanged
}

void test_tq_peek_empty_returns_empty(void)
{
    TBCI_Trigger out;
    TBCI_Status status = tq_peek(&queue, &out);
    TEST_ASSERT_EQUAL(TBCI_ERR_EMPTY, status);
}

void test_tq_peek_null_returns_invalid_arg(void)
{
    TBCI_Trigger out;
    TBCI_Status status = tq_peek(NULL, &out);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}

int main(void)
{
    UNITY_BEGIN();

    /* Init */
    RUN_TEST(test_init_valid_returns_ok);
    RUN_TEST(test_init_null_buf_returns_invalid_arg);
    RUN_TEST(test_init_null_storage_returns_invalid_arg);
    RUN_TEST(test_init_zero_capacity_returns_invalid_arg);
    RUN_TEST(test_init_size_is_zero);
    RUN_TEST(test_init_capacity_matches);
    RUN_TEST(test_init_queue_is_empty);

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
    RUN_TEST(test_tq_peek_returns_front_without_consuming);
    RUN_TEST(test_tq_peek_empty_returns_empty);
    RUN_TEST(test_tq_peek_null_returns_invalid_arg);

    return UNITY_END();
}
