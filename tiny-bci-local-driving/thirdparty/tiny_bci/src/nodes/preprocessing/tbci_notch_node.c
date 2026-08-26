/**
 * @file tbci_notch_node.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Notch filter node implementation.
 */
#include "tbci_context.h"
#include "../../../include/mathutils/tbci_math.h"
#include "nodes/preprocessing/tbci_notch_node.h"

/* --------------------------------------------------------------------------
 * Internal helpers — coefficient computation
 * -------------------------------------------------------------------------- */

/**
 * @brief Compute notch biquad coefficients at a given frequency.
 *
 * 2nd order IIR notch (band-reject) filter via bilinear transform:
 *
 *   w0    = 2π * freq_hz / fs
 *   alpha = sin(w0) / (2 * Q)
 *   b0 =  1,  b1 = -2*cos(w0),  b2 = 1
 *   a0 =  1 + alpha,  a1 = -2*cos(w0),  a2 = 1 - alpha
 *   → normalize all by a0
 *
 * @param[out] cfg      Output config. Must not be NULL.
 * @param[in]  freq_hz  Notch frequency in Hz.
 * @param[in]  q_factor Quality factor. Higher = narrower notch.
 * @param[in]  fs       Sampling rate in Hz.
 */
static void compute_notch(TBCI_IIRFilterConfig *cfg, float freq_hz, float q_factor, float fs)
{
    if (q_factor < TBCI_NOTCH_Q_MIN) q_factor = TBCI_NOTCH_Q_MIN;
    float w0     = 2 * TBCI_M_PI * freq_hz / fs;
    float alpha  = sinf(w0) / (2 * (q_factor));
    float cos_w0 = cosf(w0);

    float b0 = 1.0f;
    float b1 = -2 * cos_w0;
    float b2 = 1.0f;
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

static TBCI_Status notch_init_fn(TBCI_Node *self, struct TBCI_Context *ctx)
{
    TBCI_NotchNode *node = (TBCI_NotchNode *)self;

    if (ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    float fs = ctx->config.target_srate;

    /* compute one biquad per harmonic */
    for (size_t h = 1; h <= node->config.n_harmonics; h++) {
        float harmonic_freq = h * node->config.freq_hz;
        TBCI_IIRFilterConfig cfg;
        compute_notch(&cfg, harmonic_freq, node->config.q_factor, fs);
        iir_init(&node->filters[h-1], &cfg);
    }
    node->n_filters = node->config.n_harmonics;

    return TBCI_OK;
}

static TBCI_NodeResult notch_process_fn(TBCI_Node *self, void *data, struct TBCI_Context *ctx)
{
    return notch_process((TBCI_NotchNode *)self, data, ctx);
}

static TBCI_Status notch_reset_fn(TBCI_Node *self)
{
    return notch_reset((TBCI_NotchNode *)self);
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

TBCI_Status notch_init(TBCI_NotchNode *node, TBCI_NotchConfig *config)
{
    if (node == NULL || config == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (config->n_harmonics == 0 || config->n_harmonics > TBCI_MAX_NOTCH_HARMONICS)
        return TBCI_ERR_INVALID_ARG;

    node->base.name       = "notch";
    node->base.instance_size = sizeof(TBCI_NotchNode);
    node->base.type       = TBCI_NODE_TYPE_PREPROCESSING;
    node->base.enabled    = true;
    node->base.init_fn    = (TBCI_NodeInitFn) notch_init_fn;
    node->base.process_fn = (TBCI_NodeProcessFn) notch_process_fn;
    node->base.reset_fn   = (TBCI_NodeResetFn) notch_reset_fn;
    node->base.tick_fn    = NULL;
    node->config          = *config;
    node->n_filters       = 0;  /* set by notch_init_fn */

    return TBCI_OK;
}

TBCI_NodeResult notch_process(TBCI_NotchNode *node, void *data, struct TBCI_Context *ctx)
{
    if (node == NULL || data == NULL || ctx == NULL)
        return TBCI_NODE_ERROR;

    /* apply each notch biquad in sequence */
    for (size_t i = 0; i < node->n_filters; i++) {
        TBCI_NodeResult r = iir_process(&node->filters[i], data, ctx);
        if (r != TBCI_NODE_OK) return r;
    }

    return TBCI_NODE_OK;
}

TBCI_Status notch_reset(TBCI_NotchNode *node)
{
    if (node == NULL)
        return TBCI_ERR_INVALID_ARG;

    for (size_t i = 0; i < node->n_filters; i++)
        iir_reset(&node->filters[i]);

    return TBCI_OK;
}