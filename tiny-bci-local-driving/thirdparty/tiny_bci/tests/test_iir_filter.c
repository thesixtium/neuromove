/**
 * @file test_iir_filter.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for IIR filter state and bandpass node.
 */

#include "unity/unity.h"
#include "tbci_context.h"
#include "nodes/preprocessing/tbci_iir_filter_state.h"
#include "nodes/preprocessing/tbci_bandpass_node.h"
#include "nodes/preprocessing/tbci_notch_node.h"

/* --------------------------------------------------------------------------
 * Test dimensions
 * -------------------------------------------------------------------------- */
#define N_CHANNELS      4
#define TARGET_SRATE    256.0f
#define SETTLE_SAMPLES  256     /* samples to run before checking output */
#define FLOAT_TOL       0.1f    /* loose tolerance — filter response not exact */

/* --------------------------------------------------------------------------
 * Static storage
 * -------------------------------------------------------------------------- */
static float sig_storage[512 * N_CHANNELS];
static uint64_t sig_timestamps[512];
static uint32_t sig_indices[512];
static float proc_storage[512 * N_CHANNELS];
static uint64_t proc_timestamps[512];
static uint32_t proc_indices[512];
static TBCI_Trigger trig_storage[8];
static TBCI_Epoch epoch_storage[4];
static float epoch_pool[4 * 256 * N_CHANNELS];
static TBCI_Epoch features_storage[4];
static float features_pool[4 * 256 * N_CHANNELS];
static TBCI_Epoch output_storage[4];
static float output_pool[4 * 256 * N_CHANNELS];

static TBCI_SignalBuffer sig_buf;
static TBCI_SignalBuffer proc_buf;
static TBCI_TriggerQueue trig_queue;
static TBCI_EpochQueue epoch_queue;
static TBCI_EpochQueue features_queue;
static TBCI_EpochQueue output_queue;
static TBCI_Input inputs;
static TBCI_Config config;
static TBCI_Context ctx;

static TBCI_IIRFilterState iir_state;
static TBCI_IIRFilterConfig iir_config;
static TBCI_BandpassNode bp_node;
static TBCI_BandpassConfig bp_config;
static TBCI_NotchNode notch_node;
static TBCI_NotchConfig notch_config;

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

/**
 * @brief Run n_samples of a constant DC value through the filter state.
 * Returns the last output sample on channel 0.
 */
static float run_dc_through_iir(TBCI_IIRFilterState* state, float dc_value, int n_samples)
{
    float samples[N_CHANNELS];
    float last_out = 0.0f;
    for (int i = 0; i < n_samples; i++)
    {
        for (int ch = 0; ch < N_CHANNELS; ch++)
            samples[ch] = dc_value;
        iir_process(state, samples, &ctx);
        last_out = samples[0];
    }
    return last_out;
}

/**
 * @brief Run n_samples of a sine wave at freq_hz through the filter state.
 * Returns the RMS of the last 64 output samples on channel 0.
 */
static float run_sine_through_iir(TBCI_IIRFilterState* state, float freq_hz, int n_samples)
{
    float samples[N_CHANNELS];
    float rms = 0.0f;
    int rms_count = 64;
    int rms_start = n_samples - rms_count;

    for (int i = 0; i < n_samples; i++)
    {
        float val = sinf(2.0f * 3.14159265f * freq_hz * (float)i / TARGET_SRATE);
        for (int ch = 0; ch < N_CHANNELS; ch++)
            samples[ch] = val;
        iir_process(state, samples, &ctx);
        if (i >= rms_start)
            rms += samples[0] * samples[0];
    }
    return sqrtf(rms / (float)rms_count);
}

/**
 * @brief Run n_samples of a sine wave through the bandpass node.
 * Returns the RMS of the last 64 output samples on channel 0.
 */
static float run_sine_through_bp(TBCI_BandpassNode* node, float freq_hz, int n_samples)
{
    float samples[N_CHANNELS];
    float rms = 0.0f;
    int rms_count = 64;
    int rms_start = n_samples - rms_count;

    for (int i = 0; i < n_samples; i++)
    {
        float val = sinf(2.0f * 3.14159265f * freq_hz * (float)i / TARGET_SRATE);
        for (int ch = 0; ch < N_CHANNELS; ch++)
            samples[ch] = val;
        bp_process(node, samples, &ctx);
        if (i >= rms_start)
            rms += samples[0] * samples[0];
    }
    return sqrtf(rms / (float)rms_count);
}

static float run_sine_through_notch(TBCI_NotchNode* node, float freq_hz, int n_samples)
{
    float samples[N_CHANNELS];
    float rms = 0.0f;
    int rms_count = 64;
    int rms_start = n_samples - rms_count;

    for (int i = 0; i < n_samples; i++)
    {
        float val = sinf(2.0f * 3.14159265f * freq_hz * (float)i / TARGET_SRATE);
        for (int ch = 0; ch < N_CHANNELS; ch++)
            samples[ch] = val;
        notch_process(node, samples, &ctx);
        if (i >= rms_start)
            rms += samples[0] * samples[0];
    }
    return sqrtf(rms / (float)rms_count);
}

/* --------------------------------------------------------------------------
 * setUp / tearDown
 * -------------------------------------------------------------------------- */

void setUp(void)
{
    memset(sig_storage, 0, sizeof(sig_storage));
    memset(sig_timestamps, 0, sizeof(sig_timestamps));
    memset(sig_indices, 0, sizeof(sig_indices));
    memset(proc_storage, 0, sizeof(proc_storage));
    memset(proc_timestamps, 0, sizeof(proc_timestamps));
    memset(proc_indices, 0, sizeof(proc_indices));
    memset(trig_storage, 0, sizeof(trig_storage));
    memset(epoch_storage, 0, sizeof(epoch_storage));
    memset(epoch_pool, 0, sizeof(epoch_pool));
    memset(features_storage, 0, sizeof(features_storage));
    memset(features_pool, 0, sizeof(features_pool));
    memset(output_storage, 0, sizeof(output_storage));
    memset(output_pool, 0, sizeof(output_pool));
    memset(&iir_state, 0, sizeof(iir_state));
    memset(&iir_config, 0, sizeof(iir_config));
    memset(&bp_node, 0, sizeof(bp_node));
    memset(&bp_config, 0, sizeof(bp_config));
    memset(&notch_node, 0, sizeof(notch_node));
    memset(&notch_config, 0, sizeof(notch_config));

    sb_init(&sig_buf, sig_storage, sig_timestamps, sig_indices, 512, N_CHANNELS);
    sb_init(&proc_buf, proc_storage, proc_timestamps, proc_indices, 512, N_CHANNELS);
    tq_init(&trig_queue, trig_storage, 8);
    eq_init(&epoch_queue, epoch_storage, 4, 256);
    eq_init(&features_queue, features_storage, 4, 256);
    eq_init(&output_queue, output_storage, 4, 256);
    eq_configure(&epoch_queue, epoch_pool, N_CHANNELS);
    eq_configure(&features_queue, features_pool, N_CHANNELS);
    eq_configure(&output_queue, output_pool, N_CHANNELS);

    inputs.signal = &sig_buf;
    inputs.triggers = &trig_queue;
    inputs.n_channels = N_CHANNELS;

    config.paradigm = TBCI_PARADIGM_P300;
    config.nominal_srate = TARGET_SRATE;
    config.target_srate = TARGET_SRATE;
    config.n_channels = N_CHANNELS;
    config.window_length_ms = 1000;
    config.use_preprocessing = false;
    config.use_feature_extraction = false;
    config.use_decoder = false;
    config.mode = SEG_MODE_TRIGGERED;
    config.pre_stimulus_ms = 0;
    config.post_stimulus_ms = 1000;
    config.overlap_ms = 0;
    config.trial_end_code = 0;

    tbci_context_init(&ctx, &config, &inputs, &proc_buf,
                      &epoch_queue, &features_queue, &output_queue);

    /* default lowpass config — 40Hz cutoff */
    iir_config.b[0] = 0.0f;
    iir_config.b[1] = 0.0f;
    iir_config.b[2] = 0.0f;
    iir_config.a[0] = 1.0f;
    iir_config.a[1] = 0.0f;
    iir_config.a[2] = 0.0f;
    iir_config.zi[0] = 0.0f;
    iir_config.zi[1] = 0.0f;

    /* default bandpass config */
    bp_config.low_hz = 1.0f;
    bp_config.high_hz = 40.0f;

    /* default notch config */
    notch_config.freq_hz = 50.0f;
    notch_config.q_factor = 30.0f;
    notch_config.n_harmonics = 2;
}

void tearDown(void)
{
}

/* ============================================================
 * GROUP 1 — iir_init
 * ============================================================ */

void test_iir_init_null_state_returns_invalid_arg(void)
{
    TBCI_Status s = iir_init(NULL, &iir_config);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_iir_init_null_config_returns_invalid_arg(void)
{
    TBCI_Status s = iir_init(&iir_state, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_iir_init_copies_config(void)
{
    iir_config.b[0] = 0.5f;
    iir_init(&iir_state, &iir_config);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, iir_state.config.b[0]);
}

void test_iir_init_state_zeroed(void)
{
    iir_init(&iir_state, &iir_config);
    for (size_t ch = 0; ch < N_CHANNELS; ch++)
    {
        TEST_ASSERT_EQUAL_FLOAT(0.0f, iir_state.w[ch][0]);
        TEST_ASSERT_EQUAL_FLOAT(0.0f, iir_state.w[ch][1]);
    }
}

void test_iir_init_initialized_flags_false(void)
{
    iir_init(&iir_state, &iir_config);
    for (size_t ch = 0; ch < N_CHANNELS; ch++)
        TEST_ASSERT_FALSE(iir_state.initialized[ch]);
}

/* ============================================================
 * GROUP 2 — iir_process
 * ============================================================ */

void test_iir_process_null_state_returns_error(void)
{
    float samples[N_CHANNELS] = {1.0f};
    TBCI_NodeResult r = iir_process(NULL, samples, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
}

void test_iir_process_null_data_returns_error(void)
{
    iir_init(&iir_state, &iir_config);
    TBCI_NodeResult r = iir_process(&iir_state, NULL, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
}

void test_iir_process_null_ctx_returns_error(void)
{
    iir_init(&iir_state, &iir_config);
    float samples[N_CHANNELS] = {1.0f};
    TBCI_NodeResult r = iir_process(&iir_state, samples, NULL);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, r);
}

void test_iir_process_dc_through_highpass_attenuates(void)
{
    /* highpass at 10Hz should block DC */
    TBCI_IIRFilterConfig hp_cfg;
    float w0 = 2.0f * 3.14159265f * 10.0f / TARGET_SRATE;
    float alpha = sinf(w0) / (2.0f * 0.707107f);
    float cos_w0 = cosf(w0);
    float a0 = 1.0f + alpha;
    hp_cfg.b[0] = (1.0f + cos_w0) / 2.0f / a0;
    hp_cfg.b[1] = -(1.0f + cos_w0) / a0;
    hp_cfg.b[2] = (1.0f + cos_w0) / 2.0f / a0;
    hp_cfg.a[0] = 1.0f;
    hp_cfg.a[1] = -2.0f * cos_w0 / a0;
    hp_cfg.a[2] = (1.0f - alpha) / a0;
    hp_cfg.zi[0] = 0.0f;
    hp_cfg.zi[1] = 0.0f;

    iir_init(&iir_state, &hp_cfg);
    float out = run_dc_through_iir(&iir_state, 1.0f, SETTLE_SAMPLES);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, out);
}

void test_iir_process_dc_through_lowpass_passes(void)
{
    /* lowpass at 40Hz should pass DC */
    TBCI_IIRFilterConfig lp_cfg;
    float w0 = 2.0f * 3.14159265f * 40.0f / TARGET_SRATE;
    float alpha = sinf(w0) / (2.0f * 0.707107f);
    float cos_w0 = cosf(w0);
    float a0 = 1.0f + alpha;
    lp_cfg.b[0] = (1.0f - cos_w0) / 2.0f / a0;
    lp_cfg.b[1] = (1.0f - cos_w0) / a0;
    lp_cfg.b[2] = (1.0f - cos_w0) / 2.0f / a0;
    lp_cfg.a[0] = 1.0f;
    lp_cfg.a[1] = -2.0f * cos_w0 / a0;
    lp_cfg.a[2] = (1.0f - alpha) / a0;
    lp_cfg.zi[0] = 0.0f;
    lp_cfg.zi[1] = 0.0f;

    iir_init(&iir_state, &lp_cfg);
    float out = run_dc_through_iir(&iir_state, 1.0f, SETTLE_SAMPLES);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.5f, out);
}

void test_iir_process_high_freq_through_lowpass_attenuates(void)
{
    /* lowpass at 10Hz should attenuate 100Hz sine */
    TBCI_IIRFilterConfig lp_cfg;
    float w0 = 2.0f * 3.14159265f * 10.0f / TARGET_SRATE;
    float alpha = sinf(w0) / (2.0f * 0.707107f);
    float cos_w0 = cosf(w0);
    float a0 = 1.0f + alpha;
    lp_cfg.b[0] = (1.0f - cos_w0) / 2.0f / a0;
    lp_cfg.b[1] = (1.0f - cos_w0) / a0;
    lp_cfg.b[2] = (1.0f - cos_w0) / 2.0f / a0;
    lp_cfg.a[0] = 1.0f;
    lp_cfg.a[1] = -2.0f * cos_w0 / a0;
    lp_cfg.a[2] = (1.0f - alpha) / a0;
    lp_cfg.zi[0] = 0.0f;
    lp_cfg.zi[1] = 0.0f;

    iir_init(&iir_state, &lp_cfg);
    float rms = run_sine_through_iir(&iir_state, 100.0f, SETTLE_SAMPLES);
    TEST_ASSERT_LESS_THAN_FLOAT(0.1f, rms);
}

void test_iir_process_high_freq_through_highpass_passes(void)
{
    /* highpass at 1Hz should pass 50Hz sine */
    TBCI_IIRFilterConfig hp_cfg;
    float w0 = 2.0f * 3.14159265f * 1.0f / TARGET_SRATE;
    float alpha = sinf(w0) / (2.0f * 0.707107f);
    float cos_w0 = cosf(w0);
    float a0 = 1.0f + alpha;
    hp_cfg.b[0] = (1.0f + cos_w0) / 2.0f / a0;
    hp_cfg.b[1] = -(1.0f + cos_w0) / a0;
    hp_cfg.b[2] = (1.0f + cos_w0) / 2.0f / a0;
    hp_cfg.a[0] = 1.0f;
    hp_cfg.a[1] = -2.0f * cos_w0 / a0;
    hp_cfg.a[2] = (1.0f - alpha) / a0;
    hp_cfg.zi[0] = 0.0f;
    hp_cfg.zi[1] = 0.0f;

    iir_init(&iir_state, &hp_cfg);
    float rms = run_sine_through_iir(&iir_state, 50.0f, SETTLE_SAMPLES);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.3f, rms);
}

/* ============================================================
 * GROUP 3 — iir_reset
 * ============================================================ */

void test_iir_reset_null_returns_invalid_arg(void)
{
    TBCI_Status s = iir_reset(NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_iir_reset_clears_state(void)
{
    iir_init(&iir_state, &iir_config);
    /* dirty the state */
    iir_state.w[0][0] = 99.0f;
    iir_state.w[0][1] = 99.0f;
    iir_reset(&iir_state);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, iir_state.w[0][0]);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, iir_state.w[0][1]);
}

void test_iir_reset_clears_initialized_flags(void)
{
    iir_init(&iir_state, &iir_config);
    iir_state.initialized[0] = true;
    iir_reset(&iir_state);
    TEST_ASSERT_FALSE(iir_state.initialized[0]);
}

/* ============================================================
 * GROUP 4 — bp_init / bp_process
 * ============================================================ */

void test_bp_init_null_node_returns_invalid_arg(void)
{
    TBCI_Status s = bp_init(NULL, &bp_config);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_bp_init_null_config_returns_invalid_arg(void)
{
    TBCI_Status s = bp_init(&bp_node, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_bp_init_valid_returns_ok(void)
{
    TBCI_Status s = bp_init(&bp_node, &bp_config);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
}

void test_bp_process_dc_signal_attenuated(void)
{
    /* bandpass 1-40Hz should block DC after settling */
    bp_init(&bp_node, &bp_config);
    node_init((TBCI_Node*)&bp_node, &ctx);

    float rms = run_sine_through_bp(&bp_node, 0.0f, SETTLE_SAMPLES);
    TEST_ASSERT_LESS_THAN_FLOAT(FLOAT_TOL, rms);
}

void test_bp_process_inband_signal_passes(void)
{
    /* 10Hz is within 1-40Hz band — should pass */
    bp_init(&bp_node, &bp_config);
    node_init((TBCI_Node*)&bp_node, &ctx);

    float rms = run_sine_through_bp(&bp_node, 10.0f, SETTLE_SAMPLES);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.3f, rms);
}

void test_bp_process_high_freq_attenuated(void)
{
    /* 100Hz is above 40Hz cutoff — should be attenuated */
    bp_init(&bp_node, &bp_config);
    node_init((TBCI_Node*)&bp_node, &ctx);

    float rms = run_sine_through_bp(&bp_node, 100.0f, SETTLE_SAMPLES);
    TEST_ASSERT_LESS_THAN_FLOAT(FLOAT_TOL, rms);
}

/* ============================================================
 * GROUP 5 — notch_init / notch_process
 * ============================================================ */
void test_notch_init_null_node_returns_invalid_arg(void)
{
    TBCI_Status s = notch_init(NULL, &notch_config);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_notch_init_null_config_returns_invalid_arg(void)
{
    TBCI_Status s = notch_init(&notch_node, NULL);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_notch_init_zero_harmonics_returns_invalid_arg(void)
{
    notch_config.n_harmonics = 0;
    TBCI_Status s = notch_init(&notch_node, &notch_config);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_notch_init_too_many_harmonics_returns_invalid_arg(void)
{
    notch_config.n_harmonics = TBCI_MAX_NOTCH_HARMONICS + 1;
    TBCI_Status s = notch_init(&notch_node, &notch_config);
    TEST_ASSERT_EQUAL(TBCI_ERR_INVALID_ARG, s);
}

void test_notch_init_valid_returns_ok(void)
{
    TBCI_Status s = notch_init(&notch_node, &notch_config);
    TEST_ASSERT_EQUAL(TBCI_OK, s);
}

void test_notch_process_null_node_returns_error(void)
{
    TBCI_Status s = notch_init(&notch_node, &notch_config);
    TEST_ASSERT_EQUAL(TBCI_OK, s);

    float fake_data[SETTLE_SAMPLES];
    for (int i = 0; i < SETTLE_SAMPLES; i++)
        fake_data[i] = 1.0f;

    TBCI_NodeResult res = notch_process(NULL, fake_data, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, res);
}

void test_notch_process_null_data_returns_error(void)
{
    TBCI_Status s = notch_init(&notch_node, &notch_config);
    TEST_ASSERT_EQUAL(TBCI_OK, s);

    TBCI_NodeResult res = notch_process(&notch_node, NULL, &ctx);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, res);
}

void test_notch_process_null_ctx_returns_error(void)
{
    TBCI_Status s = notch_init(&notch_node, &notch_config);
    TEST_ASSERT_EQUAL(TBCI_OK, s);

    float fake_data[SETTLE_SAMPLES];
    for (int i = 0; i < SETTLE_SAMPLES; i++)
        fake_data[i] = 1.0f;

    TBCI_NodeResult res = notch_process(&notch_node, fake_data, NULL);
    TEST_ASSERT_EQUAL(TBCI_NODE_ERROR, res);
}

void test_notch_process_notch_freq_attenuated(void)
{
    notch_init(&notch_node, &notch_config);
    node_init((TBCI_Node*)&notch_node, &ctx);

    float rms = run_sine_through_notch(&notch_node, 50.0f, SETTLE_SAMPLES);
    TEST_ASSERT_LESS_THAN_FLOAT(FLOAT_TOL, rms);
}

void test_notch_process_inband_signal_passes(void)
{
    notch_init(&notch_node, &notch_config);
    node_init((TBCI_Node*)&notch_node, &ctx);

    float rms = run_sine_through_notch(&notch_node, 10.0f, SETTLE_SAMPLES);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.3f, rms);
}

void test_notch_process_harmonic_attenuated(void)
{
    notch_init(&notch_node, &notch_config);
    node_init((TBCI_Node*)&notch_node, &ctx);

    float rms = run_sine_through_notch(&notch_node, 100.0f, SETTLE_SAMPLES);
    TEST_ASSERT_LESS_THAN_FLOAT(FLOAT_TOL, rms);
}

/* --------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    // Group 1 — iir_init
    RUN_TEST(test_iir_init_null_state_returns_invalid_arg);
    RUN_TEST(test_iir_init_null_config_returns_invalid_arg);
    RUN_TEST(test_iir_init_copies_config);
    RUN_TEST(test_iir_init_state_zeroed);
    RUN_TEST(test_iir_init_initialized_flags_false);

    // Group 2 — iir_process
    RUN_TEST(test_iir_process_null_state_returns_error);
    RUN_TEST(test_iir_process_null_data_returns_error);
    RUN_TEST(test_iir_process_null_ctx_returns_error);
    RUN_TEST(test_iir_process_dc_through_highpass_attenuates);
    RUN_TEST(test_iir_process_dc_through_lowpass_passes);
    RUN_TEST(test_iir_process_high_freq_through_lowpass_attenuates);
    RUN_TEST(test_iir_process_high_freq_through_highpass_passes);

    // Group 3 — iir_reset
    RUN_TEST(test_iir_reset_null_returns_invalid_arg);
    RUN_TEST(test_iir_reset_clears_state);
    RUN_TEST(test_iir_reset_clears_initialized_flags);

    // Group 4 — bp_init / bp_process
    RUN_TEST(test_bp_init_null_node_returns_invalid_arg);
    RUN_TEST(test_bp_init_null_config_returns_invalid_arg);
    RUN_TEST(test_bp_init_valid_returns_ok);
    RUN_TEST(test_bp_process_dc_signal_attenuated);
    RUN_TEST(test_bp_process_inband_signal_passes);
    RUN_TEST(test_bp_process_high_freq_attenuated);

    // Group 5 - notch_init / notch_process
    RUN_TEST(test_notch_init_null_node_returns_invalid_arg);
    RUN_TEST(test_notch_init_null_config_returns_invalid_arg);
    RUN_TEST(test_notch_init_zero_harmonics_returns_invalid_arg);
    RUN_TEST(test_notch_init_too_many_harmonics_returns_invalid_arg);
    RUN_TEST(test_notch_init_valid_returns_ok);

    RUN_TEST(test_notch_process_null_node_returns_error);
    RUN_TEST(test_notch_process_null_data_returns_error);
    RUN_TEST(test_notch_process_null_ctx_returns_error);
    RUN_TEST(test_notch_process_notch_freq_attenuated);
    RUN_TEST(test_notch_process_inband_signal_passes);
    RUN_TEST(test_notch_process_harmonic_attenuated);

    return UNITY_END();
}