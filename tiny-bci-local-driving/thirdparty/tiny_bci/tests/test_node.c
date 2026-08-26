/**
 * @file test_node.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for tbci_node.c (node_init / node_process / node_reset)
 *        using the Unity test framework.
 */

#include "unity/unity.h"
#include "../include/nodes/tbci_node.h"
#include "tbci_context.h"

#define N_CHANNELS 4

static TBCI_Context ctx;
static TBCI_Node    node;

static int             init_calls;
static int             process_calls;
static int             reset_calls;
static TBCI_Status     init_return;
static TBCI_Status     reset_return;
static TBCI_NodeResult process_return;
static bool             process_modifies;

static TBCI_Status mock_init(TBCI_Node *self, struct TBCI_Context *c)
{
    (void)self; (void)c;
    init_calls++;
    return init_return;
}

static TBCI_NodeResult mock_process(TBCI_Node *self, void *data, struct TBCI_Context *c)
{
    (void)self; (void)c;
    process_calls++;
    float *samples = (float *)data;
    if (process_modifies) {
        for (size_t i = 0; i < N_CHANNELS; i++)
            samples[i] *= 2.0f;
    }
    return process_return;
}

static TBCI_Status mock_reset(TBCI_Node *self)
{
    (void)self;
    reset_calls++;
    return reset_return;
}

void setUp(void)
{
    memset(&node, 0, sizeof(node));
    memset(&ctx,  0, sizeof(ctx));

    init_calls    = 0;
    process_calls = 0;
    reset_calls   = 0;

    init_return      = TBCI_OK;
    reset_return      = TBCI_OK;
    process_return    = TBCI_NODE_OK;
    process_modifies  = false;

    node.enabled   = true;
    node.init_fn   = mock_init;
    node.process_fn = mock_process;
    node.reset_fn  = mock_reset;
}

void tearDown(void) {}

/* ============================================================
 * GROUP 1 — node_init
 * ============================================================ */

void test_node_init_null_node_returns_invalid_arg(void)
{
    TBCI_Status s = node_init(NULL, &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_node_init_null_ctx_returns_invalid_arg(void)
{
    TBCI_Status s = node_init(&node, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_node_init_null_init_fn_returns_ok_without_calling(void)
{
    node.init_fn = NULL;
    TBCI_Status s = node_init(&node, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL(0, init_calls);
}

void test_node_init_calls_init_fn_and_returns_ok(void)
{
    TBCI_Status s = node_init(&node, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL(1, init_calls);
}

void test_node_init_propagates_init_fn_error(void)
{
    init_return = TBCI_ERR_INVALID_ARG;
    TBCI_Status s = node_init(&node, &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
    TEST_ASSERT_EQUAL(1, init_calls);
}

/* ============================================================
 * GROUP 2 — node_process
 * ============================================================ */

void test_node_process_null_node_returns_error(void)
{
    float samples[N_CHANNELS] = {0};
    TBCI_NodeResult r = node_process(NULL, samples, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
}

void test_node_process_null_samples_returns_error(void)
{
    TBCI_NodeResult r = node_process(&node, NULL, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
}

void test_node_process_null_ctx_returns_error(void)
{
    float samples[N_CHANNELS] = {0};
    TBCI_NodeResult r = node_process(&node, samples, NULL);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
}

void test_node_process_disabled_returns_ok_without_calling(void)
{
    node.enabled = false;
    float samples[N_CHANNELS] = {1.0f, 2.0f, 3.0f, 4.0f};
    float expected[N_CHANNELS] = {1.0f, 2.0f, 3.0f, 4.0f};

    TBCI_NodeResult r = node_process(&node, samples, &ctx);

    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);
    TEST_ASSERT_EQUAL(0, process_calls);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, samples, N_CHANNELS);
}

void test_node_process_null_process_fn_returns_ok_passthrough(void)
{
    node.process_fn = NULL;
    float samples[N_CHANNELS]  = {1.0f, 2.0f, 3.0f, 4.0f};
    float expected[N_CHANNELS] = {1.0f, 2.0f, 3.0f, 4.0f};

    TBCI_NodeResult r = node_process(&node, samples, &ctx);

    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, samples, N_CHANNELS);
}

void test_node_process_calls_process_fn_and_modifies_samples(void)
{
    process_modifies = true;
    float samples[N_CHANNELS]  = {1.0f, 2.0f, 3.0f, 4.0f};
    float expected[N_CHANNELS] = {2.0f, 4.0f, 6.0f, 8.0f};

    TBCI_NodeResult r = node_process(&node, samples, &ctx);

    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);
    TEST_ASSERT_EQUAL(1, process_calls);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, samples, N_CHANNELS);
}

void test_node_process_propagates_pending(void)
{
    process_return = TBCI_NODE_PENDING;
    float samples[N_CHANNELS] = {0};
    TBCI_NodeResult r = node_process(&node, samples, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, r);
}

void test_node_process_propagates_error(void)
{
    process_return = TBCI_NODE_ERROR;
    float samples[N_CHANNELS] = {0};
    TBCI_NodeResult r = node_process(&node, samples, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
}

/* ============================================================
 * GROUP 3 — node_reset
 * ============================================================ */

void test_node_reset_null_returns_invalid_arg(void)
{
    TBCI_Status s = node_reset(NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_node_reset_null_reset_fn_returns_ok_without_calling(void)
{
    node.reset_fn = NULL;
    TBCI_Status s = node_reset(&node);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL(0, reset_calls);
}

void test_node_reset_calls_reset_fn_and_returns_ok(void)
{
    TBCI_Status s = node_reset(&node);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL(1, reset_calls);
}

void test_node_reset_propagates_error(void)
{
    reset_return = TBCI_ERR_INVALID_ARG;
    TBCI_Status s = node_reset(&node);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

int main(void)
{
    UNITY_BEGIN();

    // Group 1 — node_init
    RUN_TEST(test_node_init_null_node_returns_invalid_arg);
    RUN_TEST(test_node_init_null_ctx_returns_invalid_arg);
    RUN_TEST(test_node_init_null_init_fn_returns_ok_without_calling);
    RUN_TEST(test_node_init_calls_init_fn_and_returns_ok);
    RUN_TEST(test_node_init_propagates_init_fn_error);

    // Group 2 — node_process
    RUN_TEST(test_node_process_null_node_returns_error);
    RUN_TEST(test_node_process_null_samples_returns_error);
    RUN_TEST(test_node_process_null_ctx_returns_error);
    RUN_TEST(test_node_process_disabled_returns_ok_without_calling);
    RUN_TEST(test_node_process_null_process_fn_returns_ok_passthrough);
    RUN_TEST(test_node_process_calls_process_fn_and_modifies_samples);
    RUN_TEST(test_node_process_propagates_pending);
    RUN_TEST(test_node_process_propagates_error);

    // Group 3 — node_reset
    RUN_TEST(test_node_reset_null_returns_invalid_arg);
    RUN_TEST(test_node_reset_null_reset_fn_returns_ok_without_calling);
    RUN_TEST(test_node_reset_calls_reset_fn_and_returns_ok);
    RUN_TEST(test_node_reset_propagates_error);

    return UNITY_END();
}