/**
 * @file test_decoder.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for tbci_decoder.c (cg_init / cg_process / cg_reset)
 *        using the Unity test framework.
 */

#include "unity/unity.h"
#include "../include/nodes/decoder/tbci_decoder.h"
#include "tbci_context.h"
#include "tbci_config.h"

/* --------------------------------------------------------------------------
 * Test dimensions
 * -------------------------------------------------------------------------- */
#define N_CHANNELS      8
#define N_CLASSES       6
#define N_FREQS         6
#define PRE_MS          0
#define POST_MS         1000
#define TARGET_SRATE    250.0f
#define N_FRAMES        250
#define SIG_CAPACITY    512
#define TRIG_CAPACITY   8
#define EPOCH_CAPACITY  4

/* --------------------------------------------------------------------------
 * Static storage
 * -------------------------------------------------------------------------- */
static float     raw_storage    [SIG_CAPACITY * N_CHANNELS];
static uint64_t  raw_timestamps [SIG_CAPACITY];
static uint32_t  raw_indices    [SIG_CAPACITY];

static float     proc_storage   [SIG_CAPACITY * N_CHANNELS];
static uint64_t  proc_timestamps[SIG_CAPACITY];
static uint32_t  proc_indices   [SIG_CAPACITY];

static TBCI_Epoch epoch_storage    [EPOCH_CAPACITY];
static float      epoch_pool       [EPOCH_CAPACITY * N_FRAMES * N_CHANNELS];
static TBCI_Epoch features_storage [EPOCH_CAPACITY];
static float      features_pool    [EPOCH_CAPACITY * N_FRAMES * N_CHANNELS];
static TBCI_Epoch decoder_storage[EPOCH_CAPACITY];
static float      decoder_pool  [EPOCH_CAPACITY * N_FRAMES * N_CHANNELS];

static TBCI_Trigger trig_storage[TRIG_CAPACITY];

static TBCI_SignalBuffer raw_buf;
static TBCI_SignalBuffer proc_buf;
static TBCI_EpochQueue   epoch_queue;
static TBCI_EpochQueue   features_queue;
static TBCI_EpochQueue   output_queue;
static TBCI_TriggerQueue trigger_queue;
static TBCI_Input        inputs;
static TBCI_Config       config;
static TBCI_Context      ctx;

static TBCI_Decoder dc;
static TBCI_Model      model;

/* --------------------------------------------------------------------------
 * Mock decoder tracking
 * -------------------------------------------------------------------------- */
static int train_calls;
static int eval_calls;
static int infer_calls;
static int reset_calls;
static int init_calls;

/* --------------------------------------------------------------------------
 * Mock decoder function pointers
 * -------------------------------------------------------------------------- */
static TBCI_Status mock_train(TBCI_Model *self, TBCI_Epoch *epoch)
{
    (void)self; (void)epoch;
    train_calls++;
    return TBCI_OK;
}

static TBCI_Status mock_eval(TBCI_Model *self, float *accuracy_out)
{
    (void)self;
    *accuracy_out = 1.0f;
    eval_calls++;
    return TBCI_OK;
}

static TBCI_Status mock_infer(TBCI_Model *self, TBCI_Epoch *epoch)
{
    (void)self;
    /* write uniform probabilities */
    for (size_t i = 0; i < epoch->n_channels; i++)
        epoch->samples[i] = 1.0f / (float)epoch->n_channels;
    infer_calls++;
    return TBCI_OK;
}

static TBCI_Status mock_init(TBCI_Node *self, struct TBCI_Context *ctx)
{
    (void)self; (void)ctx;
    init_calls++;
    return TBCI_OK;
}

static TBCI_Status mock_reset(TBCI_Node *self)
{
    (void)self;
    reset_calls++;
    return TBCI_OK;
}

static TBCI_NodeResult mock_process(TBCI_Node *self, void *data, struct TBCI_Context *ctx)
{
    TBCI_Model *model   = (TBCI_Model *)self;
    TBCI_Epoch *epoch   = (TBCI_Epoch *)data;

    switch (ctx->state) {
    case TBCI_STATE_TRAINING:  model->train(model, epoch); return TBCI_NODE_OK;
    case TBCI_STATE_INFERENCE: model->infer(model, epoch); return TBCI_NODE_OK;
    case TBCI_STATE_IDLE:      return TBCI_NODE_PENDING;
    }
    return TBCI_NODE_ERROR;
}

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */
static void push_feature_epoch(float value, uint16_t label, uint64_t ts_us)
{
    float *slot = eq_next_slot(&features_queue);
    TEST_ASSERT_NOT_NULL(slot);
    for (size_t i = 0; i < N_CLASSES; i++)
        slot[i] = value;
    TBCI_Epoch e = {0};
    e.samples      = slot;
    e.n_frames     = 1;
    e.n_channels   = N_CLASSES;
    e.label        = label;
    e.timestamp_us = ts_us;
    eq_push(&features_queue, &e);
}

/* --------------------------------------------------------------------------
 * setUp / tearDown
 * -------------------------------------------------------------------------- */
void setUp(void)
{
    memset(raw_storage,       0, sizeof(raw_storage));
    memset(raw_timestamps,    0, sizeof(raw_timestamps));
    memset(raw_indices,       0, sizeof(raw_indices));
    memset(proc_storage,      0, sizeof(proc_storage));
    memset(proc_timestamps,   0, sizeof(proc_timestamps));
    memset(proc_indices,      0, sizeof(proc_indices));
    memset(epoch_storage,     0, sizeof(epoch_storage));
    memset(epoch_pool,        0, sizeof(epoch_pool));
    memset(features_storage,  0, sizeof(features_storage));
    memset(features_pool,     0, sizeof(features_pool));
    memset(decoder_storage,0, sizeof(decoder_storage));
    memset(decoder_pool,   0, sizeof(decoder_pool));
    memset(trig_storage,      0, sizeof(trig_storage));
    memset(&dc,               0, sizeof(dc));
    memset(&model,  0, sizeof(model));

    train_calls = 0;
    eval_calls  = 0;
    infer_calls = 0;
    reset_calls = 0;
    init_calls  = 0;

    sb_init(&raw_buf,  raw_storage,  raw_timestamps,  raw_indices,  SIG_CAPACITY, N_CHANNELS);
    sb_init(&proc_buf, proc_storage, proc_timestamps, proc_indices, SIG_CAPACITY, N_CHANNELS);
    eq_init(&epoch_queue,      epoch_storage,      EPOCH_CAPACITY, N_FRAMES);
    eq_init(&features_queue,   features_storage,   EPOCH_CAPACITY, N_FRAMES);
    eq_init(&output_queue, decoder_storage, EPOCH_CAPACITY, N_FRAMES);
    eq_configure(&epoch_queue,      epoch_pool,      N_CHANNELS);
    eq_configure(&features_queue,   features_pool,   N_CHANNELS);
    eq_configure(&output_queue, decoder_pool, N_CHANNELS);
    tq_init(&trigger_queue, trig_storage, TRIG_CAPACITY);

    inputs.signal     = &raw_buf;
    inputs.triggers   = &trigger_queue;
    inputs.n_channels = N_CHANNELS;

    config.paradigm               = TBCI_PARADIGM_SSVEP;
    config.nominal_srate          = TARGET_SRATE;
    config.target_srate           = TARGET_SRATE;
    config.n_channels             = N_CHANNELS;
    config.n_classes              = N_CLASSES;
    config.window_length_ms       = POST_MS;
    config.use_preprocessing      = false;
    config.use_feature_extraction = true;
    config.use_decoder         = true;
    config.mode                   = SEG_MODE_SLIDING;
    config.pre_stimulus_ms        = PRE_MS;
    config.post_stimulus_ms       = POST_MS;
    config.overlap_ms             = 0;
    config.trial_end_code         = 0;

    tbci_context_init(&ctx, &config, &inputs, &proc_buf, &epoch_queue, &features_queue, &output_queue);
    ctx.output_queue = &output_queue;

    /* wire mock decoder */
    model.base.enabled    = true;
    model.base.init_fn    = mock_init;
    model.base.process_fn = mock_process;
    model.base.reset_fn   = mock_reset;
    model.train           = mock_train;
    model.eval            = mock_eval;
    model.infer           = mock_infer;
    model.type            = TBCI_CCA_MODEL;
}

void tearDown(void) {}

/* ============================================================
 * GROUP 1 — dc_init
 * ============================================================ */

void test_cg_init_null_node_returns_invalid_arg(void)
{
    TBCI_Status status = dc_init(NULL, true, &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}
void test_cg_init_null_ctx_returns_invalid_arg(void)
{
    TBCI_Status status = dc_init(&dc, true, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}
void test_cg_init_enabled_no_queue_returns_invalid_state(void)
{
    tbci_context_init(&ctx, &config, &inputs, &proc_buf, &epoch_queue, &features_queue, NULL);
    TBCI_Status status = dc_init(&dc, true, &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_STATE, status);
}
void test_cg_init_disabled_no_queue_returns_ok(void)
{
    tbci_context_init(&ctx, &config, &inputs, &proc_buf, &epoch_queue, &features_queue, NULL);
    TBCI_Status status = dc_init(&dc, false, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}
void test_cg_init_valid_returns_ok(void)
{
    TBCI_Status status = dc_init(&dc, true, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
}
void test_cg_init_sets_type(void)
{
    TBCI_Status status = dc_init(&dc, true, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_INT(TBCI_NODE_TYPE_DECODER, dc.group.base.type);
}
void test_cg_init_sets_enabled_flag(void)
{
    TBCI_Status status = dc_init(&dc, true, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL(true, dc.decoding_enabled);
}
void test_cg_init_n_nodes_zero_by_default(void)
{
    TBCI_Status status = dc_init(&dc, true, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_INT(0, dc.group.n_nodes);
}

/* ============================================================
 * GROUP 2 — dc_process argument validation
 * ============================================================ */

void test_cg_process_null_node_returns_error(void)
{
    TBCI_NodeResult status = dc_process(NULL, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, status);
}
void test_cg_process_null_ctx_returns_error(void)
{
    TBCI_NodeResult status = dc_process(&dc, NULL);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, status);
}

/* ============================================================
 * GROUP 3 — dc_process state-driven dispatch
 * ============================================================ */

void test_cg_process_empty_features_queue_returns_pending(void)
{
    TBCI_NodeResult status = dc_process(&dc, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, status);
}
void test_cg_process_idle_state_returns_pending(void)
{
    ctx.state = TBCI_STATE_IDLE;
    push_feature_epoch(1.5f, 1u, 1000u);
    TBCI_NodeResult status = dc_process(&dc, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_PENDING, status);
}
void test_cg_process_inference_moves_epoch_to_output_queue(void)
{
    group_add_node(&dc.group, (TBCI_Node *)&model);
    dc_init(&dc, true, &ctx);
    ctx.state = TBCI_STATE_INFERENCE;
    push_feature_epoch(1.5f, 1u, 1000u);

    TBCI_NodeResult status = dc_process(&dc, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, status);
    TEST_ASSERT_EQUAL_INT(1, eq_size(&output_queue));
}
void test_cg_process_inference_disabled_passthrough_to_output_queue(void)
{
    dc_init(&dc, false, &ctx);  /* disabled */
    ctx.state = TBCI_STATE_INFERENCE;
    push_feature_epoch(1.5f, 1u, 1000u);

    TBCI_NodeResult status = dc_process(&dc, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, status);

    /* disabled — epoch passed through unchanged */
    TEST_ASSERT_FALSE(eq_is_empty(&output_queue));
    TBCI_Epoch out;
    eq_pop(&output_queue, &out);
    TEST_ASSERT_EQUAL_UINT16(1u, out.label);
    TEST_ASSERT_EQUAL_UINT64(1000u, out.timestamp_us);
}
void test_cg_process_training_calls_train_on_inner_node(void)
{
    group_add_node(&dc.group, (TBCI_Node *)&model);
    dc_init(&dc, true, &ctx);
    ctx.state = TBCI_STATE_TRAINING;
    push_feature_epoch(1.5f, 1u, 1000u);

    TBCI_NodeResult status = dc_process(&dc, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, status);
    TEST_ASSERT_EQUAL_INT(1, train_calls);
}
void test_cg_process_training_does_not_push_to_output_queue(void)
{
    group_add_node(&dc.group, (TBCI_Node *)&model);
    dc_init(&dc, true, &ctx);
    ctx.state = TBCI_STATE_TRAINING;
    push_feature_epoch(1.5f, 1u, 1000u);

    TBCI_NodeResult status = dc_process(&dc, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, status);
    TEST_ASSERT_TRUE(eq_is_empty(&output_queue));
}

/* ============================================================
 * GROUP 4 — dc_reset
 * ============================================================ */

void test_cg_reset_null_returns_invalid_arg(void)
{
    TBCI_Status status = dc_reset(NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, status);
}
void test_cg_reset_calls_inner_node_reset(void)
{
    group_add_node(&dc.group, (TBCI_Node *)&model);
    dc_init(&dc, true, &ctx);

    TBCI_Status status = dc_reset(&dc);
    TEST_ASSERT_EQUAL(TBCI_OK, status);
    TEST_ASSERT_EQUAL_INT(1, reset_calls);
}

/* --------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();

    // Group 1 — dc_init
    RUN_TEST(test_cg_init_null_node_returns_invalid_arg);
    RUN_TEST(test_cg_init_null_ctx_returns_invalid_arg);
    RUN_TEST(test_cg_init_enabled_no_queue_returns_invalid_state);
    RUN_TEST(test_cg_init_disabled_no_queue_returns_ok);
    RUN_TEST(test_cg_init_valid_returns_ok);
    RUN_TEST(test_cg_init_sets_type);
    RUN_TEST(test_cg_init_sets_enabled_flag);
    RUN_TEST(test_cg_init_n_nodes_zero_by_default);

    // Group 2 — dc_process validation
    RUN_TEST(test_cg_process_null_node_returns_error);
    RUN_TEST(test_cg_process_null_ctx_returns_error);

    // Group 3 — state-driven dispatch
    RUN_TEST(test_cg_process_empty_features_queue_returns_pending);
    RUN_TEST(test_cg_process_idle_state_returns_pending);
    RUN_TEST(test_cg_process_inference_moves_epoch_to_output_queue);
    RUN_TEST(test_cg_process_inference_disabled_passthrough_to_output_queue);
    RUN_TEST(test_cg_process_training_calls_train_on_inner_node);
    RUN_TEST(test_cg_process_training_does_not_push_to_output_queue);

    // Group 4 — dc_reset
    RUN_TEST(test_cg_reset_null_returns_invalid_arg);
    RUN_TEST(test_cg_reset_calls_inner_node_reset);

    return UNITY_END();
}