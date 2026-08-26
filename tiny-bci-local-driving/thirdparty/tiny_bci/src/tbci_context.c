/**
 * @file tbci_context.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief TinyBCI pipeline context implementation.
 */

#include "tbci_context.h"
#include "tbci_config.h"
#include "../include/nodes/tbci_node.h"
#include "../include/nodes/core/tbci_core.h"
#include "../include/nodes/preprocessing/tbci_preprocessing.h"

static TBCI_Status tbci_build_pipeline(TBCI_Context *ctx)
{
    TBCI_Status status;
    TBCI_Status warning = TBCI_OK;

    ctx->core_config = (TBCI_CoreConfig) {
        .mode             = ctx->config.mode,
        .pre_stimulus_ms  = ctx->config.pre_stimulus_ms,
        .post_stimulus_ms = ctx->config.post_stimulus_ms,
        .overlap_ms       = ctx->config.overlap_ms,
        .trial_end_code   = ctx->config.trial_end_code,
        .log_enabled      = ctx->config.log_enabled,
        .log_commands     = ctx->config.log_commands,
        .log_processed    = ctx->config.log_processed,
    };
    strncpy(ctx->core_config.log_subject, ctx->config.log_subject, sizeof(ctx->core_config.log_subject) - 1);
    strncpy(ctx->core_config.log_session, ctx->config.log_session, sizeof(ctx->core_config.log_session) - 1);

    /* Preprocessing — always registered, enabled flag controls pass-through vs filtering */
    status = pp_init(&ctx->preprocessing, ctx->config.use_preprocessing, ctx);
    if (status < TBCI_OK) return status;
    if (status > TBCI_OK) warning = status;
    ctx->nodes[ctx->n_nodes++] = (TBCI_Node *)&ctx->preprocessing;

    /* Core — synchronisation and segmentation */
    status = cn_init(&ctx->core_node, &ctx->core_config, ctx);
    if (status < TBCI_OK) return status;
    if (status > TBCI_OK) warning = status;
    ctx->nodes[ctx->n_nodes++] = (TBCI_Node *)&ctx->core_node;

    /* Features extraction — enabled flag controls pass-through vs extraction */
    status = fe_init(&ctx->features, ctx->config.use_feature_extraction, ctx);
    if (status < TBCI_OK) return status;
    if (status > TBCI_OK) warning = status;
    ctx->nodes[ctx->n_nodes++] = (TBCI_Node *)&ctx->features;

    /* Decoder — enabled flag controls pass-through vs extraction */
    status = dc_init(&ctx->decoder, ctx->config.use_decoder, ctx);
    if (status < TBCI_OK) return status;
    if (status > TBCI_OK) warning = status;
    ctx->nodes[ctx->n_nodes++] = (TBCI_Node *)&ctx->decoder;

    return warning;
}

TBCI_Status tbci_context_init(TBCI_Context* ctx, const TBCI_Config* config, TBCI_Input* inputs, TBCI_SignalBuffer* processed_signal,
    TBCI_EpochQueue* epoch_queue, TBCI_EpochQueue *features_queue, TBCI_EpochQueue *output_queue) {

    if (ctx == NULL || config == NULL || inputs == NULL || processed_signal == NULL || epoch_queue == NULL || features_queue == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (config->nominal_srate == 0 || config->target_srate == 0 || config->n_channels == 0 || config->window_length_ms == 0)
        return TBCI_ERR_INVALID_ARG;

    ctx->config = *config;
    ctx->inputs = inputs;
    ctx->processed_signal = processed_signal;
    ctx->epoch_queue = epoch_queue;
    ctx->features_queue = features_queue;
    ctx->output_queue = output_queue;
    ctx->state = TBCI_STATE_IDLE;

    memset(ctx->nodes, 0, sizeof(ctx->nodes));
    ctx->n_nodes = 0;
    ctx->total_frames = (size_t)roundf((float)(config->pre_stimulus_ms + config->post_stimulus_ms)
        / 1000.0f * config->target_srate);

    return tbci_build_pipeline(ctx);
}

TBCI_Status tbci_context_start(TBCI_Context* ctx, TBCI_State state)
{
    if (ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (ctx->state != TBCI_STATE_IDLE)
        return TBCI_ERR_INVALID_STATE;

    if (state != TBCI_STATE_TRAINING && state != TBCI_STATE_INFERENCE)
        return TBCI_ERR_INVALID_STATE;

    ctx->state = state;
    return TBCI_OK;
}

TBCI_Status tbci_context_stop(TBCI_Context* ctx)
{
    if (ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (ctx->state == TBCI_STATE_IDLE)
        return TBCI_ERR_INVALID_STATE;

    ctx->state = TBCI_STATE_IDLE;

    ro_close(&ctx->core_node.raw_out);

    return TBCI_OK;
}

TBCI_Status tbci_context_reset(TBCI_Context* ctx)
{
    if (ctx == NULL)
        return TBCI_ERR_INVALID_ARG;
    sb_reset(ctx->inputs->signal);
    tq_reset(ctx->inputs->triggers);
    sb_reset(ctx->processed_signal);
    eq_reset(ctx->epoch_queue);
    eq_reset(ctx->features_queue);
    cn_reset(&ctx->core_node);

    memset(ctx->nodes, 0, sizeof(ctx->nodes));
    ctx->n_nodes = 0;
    tbci_build_pipeline(ctx);

    ctx->state = TBCI_STATE_IDLE;

    return TBCI_OK;
}

TBCI_Status tbci_context_tick(TBCI_Context* ctx)
{
    if (ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (ctx->state == TBCI_STATE_IDLE)
        return TBCI_OK; // nothing to do

    for (size_t i = 0; i < ctx->n_nodes; i++) {
        if (!ctx->nodes[i]->enabled) continue;

        TBCI_NodeResult result = ctx->nodes[i]->tick_fn(ctx->nodes[i], ctx);

        if (result == TBCI_NODE_PENDING) break;
        if (result == TBCI_NODE_ERROR)   return TBCI_ERR_INVALID_STATE;
    }

    return TBCI_OK;
}

TBCI_Status tbci_context_add_node(TBCI_Context *ctx, TBCI_Node *node)
{
    if (ctx == NULL || node == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (ctx->n_nodes >= TBCI_MAX_NODES)
        return TBCI_ERR_FULL;

    ctx->nodes[ctx->n_nodes++] = node;
    return TBCI_OK;
}