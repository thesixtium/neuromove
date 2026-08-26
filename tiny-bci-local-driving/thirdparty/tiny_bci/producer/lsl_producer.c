/**
 * @file lsl_producer.c
 *
 * @brief LSL EEG and trigger producer implementation.
 */

#include "lsl_producer.h"
#include "tbci_context.h"

static const char *lsl_mode_str(LSLMode mode)
{
    switch (mode) {
    case LSL_MODE_EEG_AND_MARKERS: return "EEG_AND_MARKERS";
    case LSL_MODE_EEG_ONLY:        return "EEG_ONLY";
    case LSL_MODE_MARKERS_ONLY:    return "MARKERS_ONLY";
    default:                       return "UNKNOWN";
    }
}

static TBCI_Status lp_connect(TBCI_Producer *producer, TBCI_Input *inputs, struct TBCI_Context *ctx)
{
    (void)ctx;
    LSLProducer *lp = (LSLProducer *)producer;
    LSLProducerState  *state  = &lp->state;
    LSLProducerConfig *config = lp->config;

    bool ok = false;
    switch (config->mode) {
    case LSL_MODE_EEG_AND_MARKERS:
        ok = lsl_init_all(&state->lsl_ctx,
                           config->data_stream,
                           config->marker_stream,
                           config->temp_buf,
                           config->max_channels,
                           config->resolve_mode);
        break;
    case LSL_MODE_EEG_ONLY:
        ok = lsl_init_data(&state->lsl_ctx,
                           config->data_stream,
                           config->temp_buf,
                           config->max_channels,
                           config->resolve_mode);
        break;
    case LSL_MODE_MARKERS_ONLY:
        ok = lsl_init_markers(&state->lsl_ctx, config->marker_stream, config->resolve_mode);
        break;
    }

    if (!ok) return TBCI_ERR_INVALID_STATE;

    /* propagate discovered channel count back to inputs */
    if (config->mode != LSL_MODE_MARKERS_ONLY)
        inputs->n_channels = (size_t)state->lsl_ctx.n_channels;

    state->lsl_ctx.inputs    = inputs;
    state->lsl_ctx.connected = true;
    producer->connected      = true;

    printf("lsl: connected (mode=%s)\n", lsl_mode_str(config->mode));
    return TBCI_OK;
}

TBCI_Status lp_tick(LSLProducer *producer, TBCI_Input *inputs, TBCI_Context *ctx)
{
    if (producer == NULL) return TBCI_ERR_INVALID_ARG;

    LSLProducerState  *state  = &producer->state;

    /* attempt reconnect if disconnected */
    if (!state->lsl_ctx.connected) {
        state->reconnect_ticks++;
        if (state->reconnect_ticks < LSL_RECONNECT_INTERVAL_TICKS)
            return TBCI_OK;

        state->reconnect_ticks = 0;
        fprintf(stderr, "lsl: attempting reconnect...\n");
        TBCI_Status s = lp_connect((TBCI_Producer *)producer, inputs, ctx);
        if (s != TBCI_OK) {
            fprintf(stderr, "lsl: reconnect failed, will retry\n");
            return TBCI_OK;  /* non-fatal, keep trying */
        }
        fprintf(stderr, "lsl: reconnected\n");
    }

    bool ok = lsl_update(&state->lsl_ctx);
    if (!ok && !state->lsl_ctx.connected) {
        lp_reset(producer);
    }
    return TBCI_OK;
}

TBCI_Status lp_init(LSLProducer *producer, LSLProducerConfig *config)
{
    if (producer == NULL || config == NULL)
        return TBCI_ERR_INVALID_ARG;


    producer->base.name = "lsl";
    producer->base.connected = false;
    producer->base.init = lp_connect;
    producer->base.tick = (TBCI_Status(*)(TBCI_Producer*, TBCI_Input*, struct TBCI_Context*)) lp_tick;
    producer->base.reset     = (TBCI_Status(*)(TBCI_Producer*)) lp_reset;
    producer->base.close     = (TBCI_Status(*)(TBCI_Producer*)) lp_close;

    producer->config = config;
    producer->state  = (LSLProducerState) {0};

    return TBCI_OK;
}

TBCI_Status lp_close(LSLProducer *producer)
{
    if (producer == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (producer->state.lsl_ctx.connected)
    {
        lsl_close(&producer->state.lsl_ctx);
        producer->base.connected         = false;
        producer->state.lsl_ctx.connected = false;
        printf("lsl: streams closed\n");
    }
    return TBCI_OK;
}

TBCI_Status lp_reset(LSLProducer *producer)
{
    if (producer == NULL)
        return TBCI_ERR_INVALID_ARG;
    producer->state.reconnect_ticks    = 0;
    producer->state.lsl_ctx.connected  = false;

    return TBCI_OK;
}

float lp_get_srate(const LSLProducer *producer)
{
    if (producer->state.lsl_ctx.data_inlet == NULL) {
        fprintf(stderr, "lp_get_srate: data_inlet is NULL\n");
        return 0.0f;
    }
    int errcode = 0;
    lsl_streaminfo info = lsl_get_fullinfo(
        producer->state.lsl_ctx.data_inlet, LSL_FOREVER, &errcode);
    if (info == NULL) {
        fprintf(stderr, "lp_get_srate: lsl_get_fullinfo failed (errcode=%d)\n", errcode);
        return 0.0f;
    }
    float srate = (float)lsl_get_nominal_srate(info);
    lsl_destroy_streaminfo(info);
    return srate;
}

int lp_get_n_channels(const LSLProducer *producer)
{
    return producer->state.lsl_ctx.n_channels;
}

