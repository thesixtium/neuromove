/**
 * @file test_trial_averaging.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for TBCI_TrialAveragingNode.
 */

#include "unity/unity.h"
#include "nodes/decoder/tbci_trial_averaging_node.h"
#include "tbci_context.h"

#define N_REPS      3
#define N_TARGETS   3
#define N_CLASSES   3

static TBCI_TrialAveragingNode node;
static TBCI_TrialAveragingConfig config;
static TBCI_Epoch  epoch;
static float       samples[TBCI_MAX_CLASSES];
static TBCI_Context ctx;

void setUp(void)
{
    memset(&node,    0, sizeof(node));
    memset(&config,  0, sizeof(config));
    memset(&epoch,   0, sizeof(epoch));
    memset(&ctx,     0, sizeof(ctx));
    memset(samples,  0, sizeof(samples));

    config.n_reps         = N_REPS;
    config.min_confidence = 0.4f;
    config.min_margin     = 0.2f;
    config.early_stopping = false;

    epoch.samples    = samples;
    epoch.n_channels = N_CLASSES;
    epoch.n_frames   = 1;
    epoch.label      = 1u;

    ta_init(&node, &config);
}

void tearDown(void) {}

/* ============================================================
 * GROUP 1 — ta_init
 * ============================================================ */

void test_ta_init_ok(void)
{
    TEST_ASSERT_EQUAL(TBCI_OK, ta_init(&node, &config));
}

void test_ta_init_null_node(void)
{
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, ta_init(NULL, &config));
}

void test_ta_init_null_config(void)
{
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, ta_init(&node, NULL));
}

void test_ta_init_zero_reps(void)
{
    config.n_reps = 0;
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, ta_init(&node, &config));
}

void test_ta_init_zero_targets(void)
{
    config.n_reps = 0;
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, ta_init(&node, &config));
}

void test_ta_init_reps_exceed_max(void)
{
    config.n_reps = TBCI_MAX_TRIAL_REPS + 1;
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, ta_init(&node, &config));
}

void test_ta_init_name_set(void)
{
    TEST_ASSERT_EQUAL_STRING("trial_averaging", node.base.name);
}

void test_ta_init_counters_zero(void)
{
    TEST_ASSERT_EQUAL_size_t(0, node.current_epochs);
}

/* ============================================================
 * GROUP 2 — ta_process pending
 * ============================================================ */

void test_ta_process_pending_before_reps_complete(void)
{
    samples[0] = 0.8f; samples[1] = 0.1f; samples[2] = 0.1f;
    TBCI_NodeResult r = ta_process(&node, &epoch, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, r);
}



void test_ta_process_increments_epoch_count(void)
{
    samples[0] = 0.5f;
    ta_process(&node, &epoch, &ctx);
    TEST_ASSERT_EQUAL_size_t(1, node.current_epochs);
}

void test_ta_process_pending_after_partial_epochs(void)
{
    samples[0] = 0.5f;
    TBCI_NodeResult r = TBCI_NODE_PENDING;
    for (size_t i = 0; i < N_REPS - 1; i++)
        r = ta_process(&node, &epoch, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, r);
    TEST_ASSERT_EQUAL_size_t(N_REPS - 1, node.current_epochs);
}

/* ============================================================
 * GROUP 3 — ta_process output
 * ============================================================ */

static void push_n_epochs(float *probs, size_t n)
{
    memcpy(samples, probs, N_CLASSES * sizeof(float));
    for (size_t i = 0; i < n; i++)
        ta_process(&node, &epoch, &ctx);
}

void test_ta_process_ok_after_all_reps(void)
{
    float probs[N_CLASSES] = {0.7f, 0.2f, 0.1f};
    TBCI_NodeResult r = TBCI_NODE_PENDING;
    memcpy(samples, probs, sizeof(probs));
    for (size_t i = 0; i < N_REPS * N_TARGETS; i++)
        r = ta_process(&node, &epoch, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);
}

void test_ta_process_output_argmax_correct(void)
{
    /* class 1 always wins */
    float probs[N_CLASSES] = {0.1f, 0.8f, 0.1f};
    memcpy(samples, probs, sizeof(probs));
    for (size_t i = 0; i < N_REPS * N_TARGETS; i++)
        ta_process(&node, &epoch, &ctx);
    TEST_ASSERT_EQUAL_INT(1, epoch.predicted_label);
}

void test_ta_process_output_confidence_set(void)
{
    float probs[N_CLASSES] = {0.1f, 0.8f, 0.1f};
    memcpy(samples, probs, sizeof(probs));
    for (size_t i = 0; i < N_REPS * N_TARGETS; i++)
        ta_process(&node, &epoch, &ctx);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.8f, epoch.confidence);
}

void test_ta_process_resets_after_output(void)
{
    float probs[N_CLASSES] = {0.7f, 0.2f, 0.1f};
    memcpy(samples, probs, sizeof(probs));
    for (size_t i = 0; i < N_REPS * N_TARGETS; i++)
        ta_process(&node, &epoch, &ctx);
    /* after output, counters should be reset */
    TEST_ASSERT_EQUAL_size_t(0, node.current_epochs);
}

void test_ta_process_averaging_correct(void)
{
    /* rep 1: [0.9, 0.05, 0.05], rep 2: [0.5, 0.3, 0.2], rep 3: [0.7, 0.2, 0.1] */
    /* avg:   [0.7, 0.183, 0.117] → class 0 wins */
    float rep1[N_CLASSES] = {0.9f, 0.05f, 0.05f};
    float rep2[N_CLASSES] = {0.5f, 0.3f,  0.2f};
    float rep3[N_CLASSES] = {0.7f, 0.2f,  0.1f};

    for (size_t t = 0; t < N_TARGETS; t++) { memcpy(samples, rep1, sizeof(rep1)); ta_process(&node, &epoch, &ctx); }
    for (size_t t = 0; t < N_TARGETS; t++) { memcpy(samples, rep2, sizeof(rep2)); ta_process(&node, &epoch, &ctx); }
    for (size_t t = 0; t < N_TARGETS; t++) { memcpy(samples, rep3, sizeof(rep3)); ta_process(&node, &epoch, &ctx); }

    TEST_ASSERT_EQUAL_INT(0, epoch.predicted_label);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.7f, epoch.samples[0]);
}

/* ============================================================
 * GROUP 4 — early stopping
 * ============================================================ */

void test_ta_early_stopping_fires_on_confident_rep(void)
{
    config.early_stopping = true;
    ta_init(&node, &config);

    /* very confident after rep 1 */
    float probs[N_CLASSES] = {0.95f, 0.03f, 0.02f};
    memcpy(samples, probs, sizeof(probs));

    TBCI_NodeResult r = TBCI_NODE_PENDING;
    for (size_t i = 0; i < N_TARGETS; i++)
        r = ta_process(&node, &epoch, &ctx);

    TEST_ASSERT_EQUAL(TBCI_NODE_OK, r);
    TEST_ASSERT_EQUAL_INT(0, epoch.predicted_label);
}

void test_ta_early_stopping_does_not_fire_on_low_confidence(void)
{
    config.early_stopping = true;
    config.min_confidence = 0.9f;
    config.min_margin     = 0.5f;
    config.n_reps       = 5;  /* more than we push */
    ta_init(&node, &config);

    float probs[N_CLASSES] = {0.4f, 0.35f, 0.25f};
    memcpy(samples, probs, sizeof(probs));

    TBCI_NodeResult r = TBCI_NODE_PENDING;
    for (size_t i = 0; i < 3; i++)
        r = ta_process(&node, &epoch, &ctx);

    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, r);
}

/* ============================================================
 * GROUP 5 — ta_reset
 * ============================================================ */

void test_ta_reset_clears_counters(void)
{
    float probs[N_CLASSES] = {0.5f, 0.3f, 0.2f};
    memcpy(samples, probs, sizeof(probs));
    ta_process(&node, &epoch, &ctx);
    ta_reset(&node);
    TEST_ASSERT_EQUAL_size_t(0, node.current_epochs);
}

void test_ta_reset_clears_probs(void)
{
    float probs[N_CLASSES] = {0.5f, 0.3f, 0.2f};
    memcpy(samples, probs, sizeof(probs));
    ta_process(&node, &epoch, &ctx);
    ta_reset(&node);
    for (size_t c = 0; c < TBCI_MAX_CLASSES; c++)
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, node.avg_probs[c]);
}

void test_ta_reset_null_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, ta_reset(NULL));
}

/* --------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    // Group 1 — init
    RUN_TEST(test_ta_init_ok);
    RUN_TEST(test_ta_init_null_node);
    RUN_TEST(test_ta_init_null_config);
    RUN_TEST(test_ta_init_zero_reps);
    RUN_TEST(test_ta_init_zero_targets);
    RUN_TEST(test_ta_init_reps_exceed_max);
    RUN_TEST(test_ta_init_name_set);
    RUN_TEST(test_ta_init_counters_zero);

    // Group 2 — pending
    RUN_TEST(test_ta_process_pending_before_reps_complete);
    RUN_TEST(test_ta_process_increments_epoch_count);
    RUN_TEST(test_ta_process_pending_after_partial_epochs);

    // Group 3 — output
    RUN_TEST(test_ta_process_ok_after_all_reps);
    RUN_TEST(test_ta_process_output_argmax_correct);
    RUN_TEST(test_ta_process_output_confidence_set);
    RUN_TEST(test_ta_process_resets_after_output);
    RUN_TEST(test_ta_process_averaging_correct);

    // Group 4 — early stopping
    RUN_TEST(test_ta_early_stopping_fires_on_confident_rep);
    RUN_TEST(test_ta_early_stopping_does_not_fire_on_low_confidence);

    // Group 5 — reset
    RUN_TEST(test_ta_reset_clears_counters);
    RUN_TEST(test_ta_reset_clears_probs);
    RUN_TEST(test_ta_reset_null_returns_invalid_arg);

    return UNITY_END();
}