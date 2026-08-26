/**
 * @file test_onnx_model.c
 * @brief Unit tests for TBCI_ONNXModel using the Unity test framework.
 */
#ifdef TBCI_WITH_ONNX

#include "unity/unity.h"
#include "nodes/decoder/tbci_onnx_model.h"
#include "tbci_context.h"

#define N_CHANNELS   8
#define N_FRAMES     200
#define INPUT_SIZE   (N_CHANNELS * N_FRAMES)
#define N_CLASSES    1
#define MAX_TRIALS   16
#define N_FOLDS      4

/* --------------------------------------------------------------------------
 * Static storage
 * -------------------------------------------------------------------------- */
static TBCI_ONNXModel      model;
static TBCI_ONNXModelConfig config;
static float               train_trials[MAX_TRIALS * INPUT_SIZE];
static uint16_t            train_labels[MAX_TRIALS];
static float               epoch_samples[INPUT_SIZE];
static TBCI_Epoch          epoch;

/* minimal ctx — onnx_model_init needs it but doesn't use much */
static TBCI_Context        ctx;

void setUp(void)
{
    memset(&model,        0, sizeof(model));
    memset(&config,       0, sizeof(config));
    memset(train_trials,  0, sizeof(train_trials));
    memset(train_labels,  0, sizeof(train_labels));
    memset(epoch_samples, 0, sizeof(epoch_samples));
    memset(&ctx,          0, sizeof(ctx));

    strncpy(config.model_path, "model.onnx", TBCI_ONNX_MAX_PATH_LEN - 1);
    config.train_trials   = train_trials;
    config.train_labels   = train_labels;
    config.train_capacity = MAX_TRIALS;
    config.n_folds        = N_FOLDS;
    config.output_mode    = TBCI_OUTPUT_SIGMOID;
    config.sigmoid_threshold = 0.5f;
    config.temperature = 1.0f;

    epoch.samples    = epoch_samples;
    epoch.n_channels = N_CHANNELS;
    epoch.n_frames   = N_FRAMES;
    epoch.label      = 1;
    epoch.confidence = 0.0f;
    epoch.eval_score = 0.0f;
}

void tearDown(void)
{
    onnx_model_close(&model);
}

/* --------------------------------------------------------------------------
 * Tests
 * -------------------------------------------------------------------------- */

void test_onnx_model_init_ok(void)
{
    TBCI_Status s = onnx_model_init(&model, &config, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_NOT_NULL(model.session);
    TEST_ASSERT_NOT_NULL(model.env);
    TEST_ASSERT_EQUAL_size_t(INPUT_SIZE, model.input_size);
    TEST_ASSERT_EQUAL_size_t(N_CLASSES,  model.output_size);
}

void test_onnx_model_init_null_args(void)
{
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, onnx_model_init(NULL,   &config, &ctx));
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, onnx_model_init(&model, NULL,    &ctx));
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, onnx_model_init(&model, &config, NULL));
}

void test_onnx_model_init_bad_path(void)
{
    strncpy(config.model_path, "nonexistent.onnx", TBCI_ONNX_MAX_PATH_LEN - 1);
    TBCI_Status s = onnx_model_init(&model, &config, &ctx);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_STATE, s);
}

void test_onnx_model_infer_ok(void)
{
    TBCI_Status s = onnx_model_init(&model, &config, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);

    /* fill with non-zero data */
    for (size_t i = 0; i < INPUT_SIZE; i++)
        epoch_samples[i] = 0.1f * (float)i;

    s = onnx_model_infer(&model.base_model, &epoch);
    int predicted = model.base_model.predicted_class;
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_TRUE(epoch.confidence >= 0.0f && epoch.confidence <= 1.0f);
    TEST_ASSERT_TRUE(predicted >= 0);
}

void test_onnx_model_infer_wrong_size(void)
{
    TBCI_Status s = onnx_model_init(&model, &config, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);

    epoch.n_channels = 4;  /* wrong — model expects 8 */
    s = onnx_model_infer(&model.base_model, &epoch);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_STATE, s);
}

void test_onnx_model_train_accumulates(void)
{
    TBCI_Status s = onnx_model_init(&model, &config, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);

    s = onnx_model_train(&model.base_model, &epoch);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_EQUAL_size_t(1, model.train_count);
}

void test_onnx_model_train_full_warns(void)
{
    TBCI_Status s = onnx_model_init(&model, &config, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);

    for (size_t i = 0; i < MAX_TRIALS; i++)
        onnx_model_train(&model.base_model, &epoch);

    s = onnx_model_train(&model.base_model, &epoch);
    TEST_ASSERT_EQUAL(TBCI_WARN_FULL_TRIALS, s);
    TEST_ASSERT_EQUAL_size_t(MAX_TRIALS, model.train_count);  /* did not increment */
}

void test_onnx_model_eval_not_enough_trials(void)
{
    TBCI_Status s = onnx_model_init(&model, &config, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);

    /* fewer trials than folds */
    onnx_model_train(&model.base_model, &epoch);

    float accuracy = 0.0f;
    s = onnx_model_eval(&model.base_model, &accuracy);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_STATE, s);
}

void test_onnx_model_eval_ok(void)
{
    TBCI_Status s = onnx_model_init(&model, &config, &ctx);
    TEST_ASSERT_EQUAL(TBCI_OK, s);

    /* accumulate enough trials */
    for (size_t i = 0; i < N_FOLDS * 2; i++) {
        epoch.label = (uint16_t)(i % 2);
        onnx_model_train(&model.base_model, &epoch);
    }

    float accuracy = 0.0f;
    s = onnx_model_eval(&model.base_model, &accuracy);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
    TEST_ASSERT_TRUE(accuracy >= 0.0f && accuracy <= 1.0f);
    TEST_ASSERT_TRUE(model.base_model.eval_score >= 0.0f);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_onnx_model_init_ok);
    RUN_TEST(test_onnx_model_init_null_args);
    RUN_TEST(test_onnx_model_init_bad_path);
    RUN_TEST(test_onnx_model_infer_ok);
    RUN_TEST(test_onnx_model_infer_wrong_size);
    RUN_TEST(test_onnx_model_train_accumulates);
    RUN_TEST(test_onnx_model_train_full_warns);
    RUN_TEST(test_onnx_model_eval_not_enough_trials);
    RUN_TEST(test_onnx_model_eval_ok);
    return UNITY_END();
}

#endif /* TBCI_WITH_ONNX */