/**
 * @file test_node_group.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for tbci_node_group.c (group_init / group_process / group_reset)
 *        using the Unity test framework.
 */

#include "unity/unity.h"
#include "../include/nodes/tbci_node_group.h"
#include "tbci_context.h"
#include <string.h>

#define N_CHANNELS 4

typedef struct {
    int             init_calls;
    int             process_calls;
    int             reset_calls;
    TBCI_Status     init_return;
    TBCI_Status     reset_return;
    TBCI_NodeResult process_return;
    float           multiplier;
} MockState;

static MockState       states[TBCI_MAX_GROUP_NODES];
static TBCI_Node        nodes_arr[TBCI_MAX_GROUP_NODES];
static TBCI_NodeGroup   group;
static TBCI_Context     ctx;

static TBCI_Status mock_init(TBCI_Node *self, struct TBCI_Context *c)
{
    (void)c;
    MockState *s = (MockState *)self->state;
    s->init_calls++;
    return s->init_return;
}

static TBCI_NodeResult mock_process(TBCI_Node *self, void *data, struct TBCI_Context *c)
{
    (void)c;
    MockState *s = (MockState *)self->state;
    float *samples = (float *)data;

    s->process_calls++;
    for (size_t i = 0; i < N_CHANNELS; i++)
        samples[i] *= s->multiplier;
    return s->process_return;
}

static TBCI_Status mock_reset(TBCI_Node *self)
{
    MockState *s = (MockState *)self->state;
    s->reset_calls++;
    return s->reset_return;
}

static void setup_node(size_t i, bool enabled, float multiplier)
{
    memset(&states[i], 0, sizeof(MockState));
    states[i].init_return    = TBCI_OK;
    states[i].reset_return    = TBCI_OK;
    states[i].process_return  = TBCI_NODE_OK;
    states[i].multiplier      = multiplier;

    memset(&nodes_arr[i], 0, sizeof(TBCI_Node));
    nodes_arr[i].enabled    = enabled;
    nodes_arr[i].state      = &states[i];
    nodes_arr[i].init_fn    = mock_init;
    nodes_arr[i].process_fn = mock_process;
    nodes_arr[i].reset_fn   = mock_reset;

    group.nodes[i] = &nodes_arr[i];
}

void setUp(void)
{
    memset(&group, 0, sizeof(group));
    memset(&ctx,   0, sizeof(ctx));
    group.base.enabled = true;
    group.n_nodes      = 0;
}

void tearDown(void) {}

/* ============================================================
 * GROUP 1 — group_init
 * ============================================================ */

void test_group_init_null_group_returns_invalid_arg(void)
{
    TBCI_Status s = group_init(NULL, &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_group_init_null_ctx_returns_invalid_arg(void)
{
    TBCI_Status s = group_init(&group, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_group_init_empty_group_returns_ok(void)
{
    TBCI_Status s = group_init(&group, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
}

void test_group_init_calls_init_on_all_nodes(void)
{
    setup_node(0, true, 1.0f);
    setup_node(1, true, 1.0f);
    group.n_nodes = 2;

    TBCI_Status s = group_init(&group, &ctx);

    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL(1, states[0].init_calls);
    TEST_ASSERT_EQUAL(1, states[1].init_calls);
}

void test_group_init_stops_on_first_error(void)
{
    setup_node(0, true, 1.0f);
    setup_node(1, true, 1.0f);
    states[0].init_return = TBCI_ERR_INVALID_ARG;
    group.n_nodes = 2;

    TBCI_Status s = group_init(&group, &ctx);

    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
    TEST_ASSERT_EQUAL(1, states[0].init_calls);
    TEST_ASSERT_EQUAL(0, states[1].init_calls);
}

/* ============================================================
 * GROUP 2 — group_process
 * ============================================================ */

void test_group_process_null_group_returns_error(void)
{
    float samples[N_CHANNELS] = {0};
    TBCI_NodeResult r = group_process(NULL, samples, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
}

void test_group_process_null_samples_returns_error(void)
{
    TBCI_NodeResult r = group_process(&group, NULL, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
}

void test_group_process_null_ctx_returns_error(void)
{
    float samples[N_CHANNELS] = {0};
    TBCI_NodeResult r = group_process(&group, samples, NULL);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
}

void test_group_process_disabled_group_passthrough(void)
{
    group.base.enabled = false;
    setup_node(0, true, 2.0f);
    group.n_nodes = 1;

    float samples[N_CHANNELS]  = {1.0f, 2.0f, 3.0f, 4.0f};
    float expected[N_CHANNELS] = {1.0f, 2.0f, 3.0f, 4.0f};

    TBCI_NodeResult r = group_process(&group, samples, &ctx);

    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);
    TEST_ASSERT_EQUAL(0, states[0].process_calls);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, samples, N_CHANNELS);
}

void test_group_process_empty_group_passthrough(void)
{
    float samples[N_CHANNELS]  = {1.0f, 2.0f, 3.0f, 4.0f};
    float expected[N_CHANNELS] = {1.0f, 2.0f, 3.0f, 4.0f};

    TBCI_NodeResult r = group_process(&group, samples, &ctx);

    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, samples, N_CHANNELS);
}

void test_group_process_single_node_modifies_samples(void)
{
    setup_node(0, true, 2.0f);
    group.n_nodes = 1;

    float samples[N_CHANNELS]  = {1.0f, 2.0f, 3.0f, 4.0f};
    float expected[N_CHANNELS] = {2.0f, 4.0f, 6.0f, 8.0f};

    TBCI_NodeResult r = group_process(&group, samples, &ctx);

    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);
    TEST_ASSERT_EQUAL(1, states[0].process_calls);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, samples, N_CHANNELS);
}

void test_group_process_chains_multiple_nodes_in_order(void)
{
    setup_node(0, true, 2.0f);
    setup_node(1, true, 3.0f);
    group.n_nodes = 2;

    float samples[N_CHANNELS]  = {1.0f, 1.0f, 1.0f, 1.0f};
    float expected[N_CHANNELS] = {6.0f, 6.0f, 6.0f, 6.0f}; /* x2 then x3 */

    TBCI_NodeResult r = group_process(&group, samples, &ctx);

    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);
    TEST_ASSERT_EQUAL(1, states[0].process_calls);
    TEST_ASSERT_EQUAL(1, states[1].process_calls);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, samples, N_CHANNELS);
}

void test_group_process_skips_disabled_inner_node(void)
{
    setup_node(0, false, 2.0f); /* disabled — skipped */
    setup_node(1, true,  3.0f);
    group.n_nodes = 2;

    float samples[N_CHANNELS]  = {1.0f, 1.0f, 1.0f, 1.0f};
    float expected[N_CHANNELS] = {3.0f, 3.0f, 3.0f, 3.0f}; /* only x3 */

    TBCI_NodeResult r = group_process(&group, samples, &ctx);

    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);
    TEST_ASSERT_EQUAL(0, states[0].process_calls);
    TEST_ASSERT_EQUAL(1, states[1].process_calls);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, samples, N_CHANNELS);
}

void test_group_process_stops_on_pending(void)
{
    setup_node(0, true, 2.0f);
    setup_node(1, true, 3.0f);
    states[0].process_return = TBCI_NODE_PENDING;
    group.n_nodes = 2;

    float samples[N_CHANNELS] = {1.0f, 1.0f, 1.0f, 1.0f};

    TBCI_NodeResult r = group_process(&group, samples, &ctx);

    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, r);
    TEST_ASSERT_EQUAL(1, states[0].process_calls);
    TEST_ASSERT_EQUAL(0, states[1].process_calls);
}

void test_group_process_stops_on_error(void)
{
    setup_node(0, true, 2.0f);
    setup_node(1, true, 3.0f);
    states[0].process_return = TBCI_NODE_ERROR;
    group.n_nodes = 2;

    float samples[N_CHANNELS] = {1.0f, 1.0f, 1.0f, 1.0f};

    TBCI_NodeResult r = group_process(&group, samples, &ctx);

    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
    TEST_ASSERT_EQUAL(0, states[1].process_calls);
}

/* ============================================================
 * GROUP 3 — group_reset
 * ============================================================ */

void test_group_reset_null_returns_invalid_arg(void)
{
    TBCI_Status s = group_reset(NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_group_reset_empty_group_returns_ok(void)
{
    TBCI_Status s = group_reset(&group);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
}

void test_group_reset_calls_reset_on_all_nodes(void)
{
    setup_node(0, true, 1.0f);
    setup_node(1, true, 1.0f);
    group.n_nodes = 2;

    TBCI_Status s = group_reset(&group);

    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL(1, states[0].reset_calls);
    TEST_ASSERT_EQUAL(1, states[1].reset_calls);
}

void test_group_reset_stops_on_first_error(void)
{
    setup_node(0, true, 1.0f);
    setup_node(1, true, 1.0f);
    states[0].reset_return = TBCI_ERR_INVALID_ARG;
    group.n_nodes = 2;

    TBCI_Status s = group_reset(&group);

    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
    TEST_ASSERT_EQUAL(0, states[1].reset_calls);
}

int main(void)
{
    UNITY_BEGIN();

    // Group 1 — group_init
    RUN_TEST(test_group_init_null_group_returns_invalid_arg);
    RUN_TEST(test_group_init_null_ctx_returns_invalid_arg);
    RUN_TEST(test_group_init_empty_group_returns_ok);
    RUN_TEST(test_group_init_calls_init_on_all_nodes);
    RUN_TEST(test_group_init_stops_on_first_error);

    // Group 2 — group_process
    RUN_TEST(test_group_process_null_group_returns_error);
    RUN_TEST(test_group_process_null_samples_returns_error);
    RUN_TEST(test_group_process_null_ctx_returns_error);
    RUN_TEST(test_group_process_disabled_group_passthrough);
    RUN_TEST(test_group_process_empty_group_passthrough);
    RUN_TEST(test_group_process_single_node_modifies_samples);
    RUN_TEST(test_group_process_chains_multiple_nodes_in_order);
    RUN_TEST(test_group_process_skips_disabled_inner_node);
    RUN_TEST(test_group_process_stops_on_pending);
    RUN_TEST(test_group_process_stops_on_error);

    // Group 3 — group_reset
    RUN_TEST(test_group_reset_null_returns_invalid_arg);
    RUN_TEST(test_group_reset_empty_group_returns_ok);
    RUN_TEST(test_group_reset_calls_reset_on_all_nodes);
    RUN_TEST(test_group_reset_stops_on_first_error);

    return UNITY_END();
}