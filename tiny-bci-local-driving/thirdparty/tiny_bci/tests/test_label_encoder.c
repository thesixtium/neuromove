/**
 * @file test_label_encoder.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for TBCI_LabelEncoderNode.
 */

#include "tbci_context.h"
#include "unity/unity.h"
#include "nodes/decoder/tbci_label_encoder_node.h"


#define SIG_CAPACITY      512
#define SIG_CHANNELS        8
#define TRIG_CAPACITY      16
#define EPOCH_CAPACITY      4
#define TARGET_SRATE     256.0f
#define PRE_STIMULUS_MS   200
#define POST_STIMULUS_MS  800
#define TOTAL_FRAMES      256
#define PRE_FRAMES         51
// all static storage
static float           sig_storage[SIG_CAPACITY * SIG_CHANNELS];
static uint64_t        sig_timestamps[SIG_CAPACITY];
static uint32_t        sig_indices[SIG_CAPACITY];
static TBCI_Trigger    trig_storage[TRIG_CAPACITY];
static TBCI_Epoch      epoch_storage[EPOCH_CAPACITY];
static float           epoch_pool[EPOCH_CAPACITY * TOTAL_FRAMES * SIG_CHANNELS];
static TBCI_Epoch      features_storage[EPOCH_CAPACITY];
static float           features_pool[EPOCH_CAPACITY * TOTAL_FRAMES * SIG_CHANNELS];
static TBCI_Epoch      output_storage[EPOCH_CAPACITY];
static float           output_pool[EPOCH_CAPACITY * TOTAL_FRAMES * SIG_CHANNELS];

static TBCI_SignalBuffer       signal_buf;
static TBCI_SignalBuffer       proc_signal_buf;
static TBCI_TriggerQueue       trigger_queue;
static TBCI_EpochQueue         epoch_queue;
static TBCI_EpochQueue         features_queue;
static TBCI_EpochQueue         output_queue;
static TBCI_Input              inputs;
static TBCI_Config config;
static TBCI_Context ctx;
static TBCI_LabelEncoderConfig le_cfg;
static TBCI_LabelEncoderNode encoder;
static TBCI_Epoch            epoch;
static float                 samples[8];

void setUp(void)
{
    sb_init(&signal_buf, sig_storage, sig_timestamps, sig_indices,SIG_CAPACITY, SIG_CHANNELS);
    sb_init(&proc_signal_buf, sig_storage, sig_timestamps, sig_indices,SIG_CAPACITY, SIG_CHANNELS);
    tq_init(&trigger_queue, trig_storage, TRIG_CAPACITY);
    eq_init(&epoch_queue, epoch_storage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_init(&features_queue, features_storage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_init(&output_queue, output_storage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&epoch_queue, epoch_pool, SIG_CHANNELS);
    eq_configure(&features_queue, features_pool, SIG_CHANNELS);
    eq_configure(&output_queue, output_pool, SIG_CHANNELS);

    inputs.signal     = &signal_buf;
    inputs.triggers   = &trigger_queue;
    inputs.n_channels = SIG_CHANNELS;

    config.paradigm               = TBCI_PARADIGM_P300;
    config.nominal_srate          = TARGET_SRATE;
    config.target_srate           = TARGET_SRATE;
    config.n_channels             = SIG_CHANNELS;
    config.window_length_ms       = PRE_STIMULUS_MS + POST_STIMULUS_MS;
    config.use_preprocessing      = false;
    config.use_feature_extraction = false;

    config.mode             = SEG_MODE_TRIGGERED;
    config.pre_stimulus_ms  = PRE_STIMULUS_MS;
    config.post_stimulus_ms = POST_STIMULUS_MS;
    config.overlap_ms       = 0;
    config.trial_end_code   = 0;

    memset(&encoder, 0, sizeof(encoder));
    memset(&epoch,   0, sizeof(epoch));
    memset(samples,  0, sizeof(samples));
    epoch.samples   = samples;
    epoch.n_frames  = 1;
    epoch.n_channels = 8;
    le_cfg.binarize_target = false;
    le_init(&encoder, &le_cfg);

    tbci_context_init(&ctx, &config, &inputs, &proc_signal_buf, &epoch_queue, &features_queue, &output_queue);

}

void tearDown(void) {}

/* ============================================================
 * GROUP 1 — le_init
 * ============================================================ */

void test_le_init_ok(void)
{
    TBCI_Status s = le_init(&encoder, &le_cfg);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
}

void test_le_init_null_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, le_init(NULL, &le_cfg));
}

void test_le_init_name_set(void)
{
    TEST_ASSERT_EQUAL_STRING("label_encoder", encoder.base.name);
}

void test_le_init_enabled(void)
{
    TEST_ASSERT_TRUE(encoder.base.enabled);
}

/* ============================================================
 * GROUP 2 — le_encode
 * ============================================================ */

void test_le_encode_maps_1_to_0(void)
{
    epoch.label = 1u;
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, le_encode(&encoder, &epoch, &ctx));
    TEST_ASSERT_EQUAL_UINT16(0u, epoch.encoded_label);
}

void test_le_encode_maps_127_to_126(void)
{
    epoch.label = 127u;
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, le_encode(&encoder, &epoch, &ctx));
    TEST_ASSERT_EQUAL_UINT16(126u, epoch.encoded_label);
}

void test_le_encode_maps_arbitrary(void)
{
    epoch.label = 42u;
    TEST_ASSERT_EQUAL(TBCI_NODE_OK, le_encode(&encoder, &epoch, &ctx));
    TEST_ASSERT_EQUAL_UINT16(41u, epoch.encoded_label);
}

void test_le_encode_label_zero_returns_error(void)
{
    epoch.label = 0u;
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, le_encode(&encoder, &epoch, &ctx));
}

void test_le_encode_label_above_127_returns_error(void)
{
    epoch.label = 128u;
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, le_encode(&encoder, &epoch, &ctx));
}

void test_le_encode_null_node_returns_error(void)
{
    epoch.label = 1u;
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, le_encode(NULL, &epoch, &ctx));
}

void test_le_encode_null_epoch_returns_error(void)
{
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, le_encode(&encoder, NULL, &ctx));
}

void test_le_encode_does_not_modify_label(void)
{
    epoch.label = 5u;
    le_encode(&encoder, &epoch, &ctx);
    TEST_ASSERT_EQUAL_UINT16(5u, epoch.label);  /* original label untouched */
}

/* ============================================================
 * GROUP 3 — le_decode
 * ============================================================ */

void test_le_decode_maps_0_to_1(void)
{
    epoch.encoded_label = 0u;
    epoch.label = 1u;
    TEST_ASSERT_EQUAL(TBCI_OK, le_decode(&encoder, &epoch));
    TEST_ASSERT_EQUAL_UINT16(1u, epoch.label);
}

void test_le_decode_maps_126_to_127(void)
{
    epoch.encoded_label = 126u;
    epoch.label = 127u;
    epoch.predicted_label = -1;

    TEST_ASSERT_EQUAL(TBCI_OK, le_decode(&encoder, &epoch));
    TEST_ASSERT_EQUAL_UINT16(127u, epoch.label);
}

void test_le_decode_roundtrip(void)
{
    epoch.label = 42u;
    le_encode(&encoder, &epoch, &ctx);
    /* simulate model overwriting label with predicted class */
    // TODO: verify alls stale tests
    // epoch.label = epoch.encoded_label;
    le_decode(&encoder, &epoch);
    TEST_ASSERT_EQUAL_UINT16(42u, epoch.label);
}

void test_le_decode_null_node_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, le_decode(NULL, &epoch));
}

void test_le_decode_null_epoch_returns_invalid_arg(void)
{
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, le_decode(&encoder, NULL));
}

/* --------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    // Group 1 — init
    RUN_TEST(test_le_init_ok);
    RUN_TEST(test_le_init_null_returns_invalid_arg);
    RUN_TEST(test_le_init_name_set);
    RUN_TEST(test_le_init_enabled);

    // Group 2 — encode
    RUN_TEST(test_le_encode_maps_1_to_0);
    RUN_TEST(test_le_encode_maps_127_to_126);
    RUN_TEST(test_le_encode_maps_arbitrary);
    RUN_TEST(test_le_encode_label_zero_returns_error);
    RUN_TEST(test_le_encode_label_above_127_returns_error);
    RUN_TEST(test_le_encode_null_node_returns_error);
    RUN_TEST(test_le_encode_null_epoch_returns_error);
    RUN_TEST(test_le_encode_does_not_modify_label);

    // Group 3 — decode
    RUN_TEST(test_le_decode_maps_0_to_1);
    RUN_TEST(test_le_decode_maps_126_to_127);
    RUN_TEST(test_le_decode_roundtrip);
    RUN_TEST(test_le_decode_null_node_returns_invalid_arg);
    RUN_TEST(test_le_decode_null_epoch_returns_invalid_arg);

    return UNITY_END();
}