/**
 * @file tbci_bandpass_node.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Bandpass filter node implementation.
 */

#include "tbci_context.h"

#include "nodes/preprocessing/tbci_bandpass_node.h"


/* --------------------------------------------------------------------------
 * Internal helpers — coefficient computation
 * -------------------------------------------------------------------------- */

/**
 * @brief Compute lowpass biquad coefficients.
 *
 * Butterworth 2nd order lowpass at cutoff fc, sampling rate fs.
 * BUTTERWORTH_Q = 0.707 for maximally flat response.
 *
 * @param[out] cfg  Output config. Must not be NULL.
 * @param[in]  fc   Cutoff frequency in Hz.
 * @param[in]  fs   Sampling rate in Hz.
 * @param[in]  q    Q value
 */
static void compute_lowpass(TBCI_IIRFilterConfig *cfg, float fc, float fs, float q)
{
    float w0   = 2 * TBCI_M_PI * fc / fs;
    float alpha =  sinf(w0) / (2 * q);
    float cos_w0 = cosf(w0);

    float b0 = (1 - cos_w0) / 2;
    float b1 =  1 - cos_w0;
    float b2 = (1 - cos_w0) / 2;
    float a0 = 1 + alpha;
    float a1 = -2 * cos_w0;
    float a2 = 1 - alpha;

    /* normalize by a0 */
    cfg->b[0] = b0 / a0;
    cfg->b[1] = b1 / a0;
    cfg->b[2] = b2 / a0;
    cfg->a[0] = 1.0f;
    cfg->a[1] = a1 / a0;
    cfg->a[2] = a2 / a0;
    cfg->zi[0] = 0.0f;
    cfg->zi[1] = 0.0f;
}

/**
 * @brief Compute highpass biquad coefficients.
 *
 * Butterworth 2nd order highpass at cutoff fc, sampling rate fs.
 *
 * @param[out] cfg  Output config. Must not be NULL.
 * @param[in]  fc   Cutoff frequency in Hz.
 * @param[in]  fs   Sampling rate in Hz.
 * @param[in]  q    Q value
 */
static void compute_highpass(TBCI_IIRFilterConfig *cfg, float fc, float fs, float q)
{
    float w0    =  2 * TBCI_M_PI * fc / fs;
    float alpha =  sinf(w0) / (2 * q);
    float cos_w0 = cosf(w0);

    float b0 = (1 + cos_w0) / 2;
    float b1 = -(1 + cos_w0);
    float b2 = (1 + cos_w0) / 2;
    float a0 = 1 + alpha;
    float a1 = -2 * cos_w0;
    float a2 = 1 - alpha;

    /* normalize by a0 */
    cfg->b[0] = b0 / a0;
    cfg->b[1] = b1 / a0;
    cfg->b[2] = b2 / a0;
    cfg->a[0] = 1.0f;
    cfg->a[1] = a1 / a0;
    cfg->a[2] = a2 / a0;
    cfg->zi[0] = 0.0f;
    cfg->zi[1] = 0.0f;
}

/* --------------------------------------------------------------------------
 * Node wiring
 * -------------------------------------------------------------------------- */

static TBCI_Status bp_init_fn(TBCI_Node *self, struct TBCI_Context *ctx)
{
    TBCI_BandpassNode *node = (TBCI_BandpassNode *)self;
    if (ctx == NULL) return TBCI_ERR_INVALID_ARG;

    float fs = ctx->config.target_srate;
    node->n_stages = (node->config.n_stages == 0) ? 1 : node->config.n_stages;

    if (node->n_stages > TBCI_MAX_FILTER_STAGES) {
        fprintf(stderr, "bp_init_fn: n_stages=%zu exceeds TBCI_MAX_FILTER_STAGES=%d, clamping\n",
                node->n_stages, TBCI_MAX_FILTER_STAGES);
        node->n_stages = TBCI_MAX_FILTER_STAGES;
    }

    TBCI_IIRFilterConfig hp_cfg;
    TBCI_IIRFilterConfig lp_cfg;
    for (size_t i = 0; i < node->n_stages; i++) {
        float q = (node->config.q_factors[i] > 0.0f)
                ? node->config.q_factors[i]
                : BUTTERWORTH_Q;

        compute_highpass(&hp_cfg, node->config.low_hz, fs, q);
        compute_lowpass(&lp_cfg,  node->config.high_hz, fs, q);
        iir_init(&node->hp[i], &hp_cfg);
        iir_init(&node->lp[i], &lp_cfg);
    }

    printf("bp_init_fn: fs=%.1f low=%.1f high=%.1f stages=%zu (order=%zu)\n",
           fs, node->config.low_hz, node->config.high_hz,
           node->n_stages, node->n_stages * 2);

    return TBCI_OK;
}

static TBCI_NodeResult bp_process_fn(TBCI_Node *self, void *data, struct TBCI_Context *ctx)
{
    return bp_process((TBCI_BandpassNode *)self, data, ctx);
}

static TBCI_Status bp_reset_fn(TBCI_Node *self)
{
    return bp_reset((TBCI_BandpassNode *)self);
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

TBCI_Status bp_init(TBCI_BandpassNode *node, TBCI_BandpassConfig *config)
{
    if (node == NULL || config == NULL)
        return TBCI_ERR_INVALID_ARG;
    node->base.name       = "bandpass";
    node->base.instance_size = sizeof(TBCI_BandpassNode);
    node->base.type       = TBCI_NODE_TYPE_PREPROCESSING;
    node->base.enabled    = true;
    node->base.init_fn    = (TBCI_NodeInitFn) bp_init_fn;
    node->base.process_fn = (TBCI_NodeProcessFn) bp_process_fn;
    node->base.reset_fn   = (TBCI_NodeResetFn) bp_reset_fn;
    node->base.tick_fn    = NULL;
    node->config          = *config;

    return TBCI_OK;
}

TBCI_Status bp_configure(TBCI_BandpassConfig *config, float low_hz, float high_hz, size_t n_stages)
{
    if (config == NULL) return TBCI_ERR_INVALID_ARG;
    if (n_stages == 0 || n_stages > TBCI_MAX_FILTER_STAGES) {
        fprintf(stderr, "bp_configure: n_stages=%zu out of range [1, %d]\n",
                n_stages, TBCI_MAX_FILTER_STAGES);
        return TBCI_ERR_INVALID_ARG;
    }

    /* true Butterworth Q values per stage count */
    static const float q_table[TBCI_MAX_FILTER_STAGES][TBCI_MAX_FILTER_STAGES] = {
        { 0.7071f, 0.0000f, 0.0000f, 0.0000f },  /* 1 stage — 2nd order  */
        { 0.5412f, 1.3066f, 0.0000f, 0.0000f },  /* 2 stages — 4th order */
        { 0.5176f, 0.7071f, 1.9319f, 0.0000f },  /* 3 stages — 6th order */
        { 0.5098f, 0.6013f, 0.8999f, 2.5628f },  /* 4 stages — 8th order */
    };

    config->low_hz   = low_hz;
    config->high_hz  = high_hz;
    config->n_stages = n_stages;
    memcpy(config->q_factors, q_table[n_stages - 1],
           n_stages * sizeof(float));

    return TBCI_OK;
}

TBCI_NodeResult bp_process(TBCI_BandpassNode *node, void *data, struct TBCI_Context *ctx)
{
    if (node == NULL || data == NULL || ctx == NULL)
        return TBCI_NODE_ERROR;

    TBCI_NodeResult r;
    for (size_t i = 0; i < node->n_stages; i++) {
        r = iir_process(&node->hp[i], data, ctx);
        if (r != TBCI_NODE_OK) return r;
        r = iir_process(&node->lp[i], data, ctx);
        if (r != TBCI_NODE_OK) return r;
    }
    return TBCI_NODE_OK;
}

TBCI_Status bp_reset(TBCI_BandpassNode *node)
{
    if (node == NULL) return TBCI_ERR_INVALID_ARG;

    for (size_t i = 0; i < node->n_stages; i++) {
        iir_reset(&node->hp[i]);
        iir_reset(&node->lp[i]);
    }
    return TBCI_OK;
}