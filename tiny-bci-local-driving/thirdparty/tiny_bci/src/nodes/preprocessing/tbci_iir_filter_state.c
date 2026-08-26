/**
 * @file tbci_iir_filter_state.c
 */

#include "tbci_context.h"
#include "nodes/preprocessing/tbci_iir_filter_state.h"


TBCI_Status iir_init(TBCI_IIRFilterState *state, TBCI_IIRFilterConfig *config)
{
    if (state == NULL || config == NULL)
        return TBCI_ERR_INVALID_ARG;

    state->config = *config;
    memset(state->w,           0, sizeof(state->w));
    memset(state->initialized, 0, sizeof(state->initialized));

    return TBCI_OK;
}

TBCI_NodeResult iir_process(TBCI_IIRFilterState *state, void *data, struct TBCI_Context *ctx)
{
    if (state == NULL || data == NULL || ctx == NULL)
        return TBCI_NODE_ERROR;

    float  *samples   = (float *)data;
    size_t  n_ch      = ctx->config.n_channels;

    float b0 = state->config.b[0];
    float b1 = state->config.b[1];
    float b2 = state->config.b[2];
    float a1 = state->config.a[1];  /* a[0] is always 1.0, skip it */
    float a2 = state->config.a[2];

    for (size_t ch = 0; ch < n_ch; ch++) {
        float x = samples[ch];

        /* initialise state on first sample to avoid startup transient */
        if (!state->initialized[ch]) {
            state->w[ch][0] =  state->config.zi[0] * x;
            state->w[ch][1] =  state->config.zi[1] * x;
            state->initialized[ch] = true;
        }

        /* Direct Form II transposed biquad */
        float y        = b0*x + state->w[ch][0];
        state->w[ch][0] = b1*x - a1*y + state->w[ch][1];
        state->w[ch][1] = b2*x - a2*y;

        samples[ch] = y;
    }

    return TBCI_NODE_OK;
}

TBCI_Status iir_reset(TBCI_IIRFilterState *state)
{
    if (state == NULL)
        return TBCI_ERR_INVALID_ARG;

    memset(state->w,           0, sizeof(state->w));
    memset(state->initialized, 0, sizeof(state->initialized));

    return TBCI_OK;
}