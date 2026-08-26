/**
* @file tbci_core_node.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief TinyBCI pipeline Core implementation.
 */

#include "tbci_common.h"
#include "tbci_context.h"
#include "../../../include/nodes/core/tbci_core.h"

TBCI_Status cn_init(TBCI_Core *node, TBCI_CoreConfig *config, TBCI_Context *ctx)
{
    if (node == NULL || config == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    node->base.enabled = true;
    node->base.name    = "core";
    node->base.type    = TBCI_NODE_TYPE_CORE;
    node->config       = config;
    node->base.tick_fn = (TBCI_TopProcessFn)cn_process;
    node->base.instance_size = sizeof(TBCI_Core);

    if (config->log_enabled) {
        if (config->log_subject[0] == '\0')
            strncpy(config->log_subject, "S00", sizeof(config->log_subject) - 1);
        if (config->log_session[0] == '\0')
            strncpy(config->log_session, "000", sizeof(config->log_session) - 1);
    }

    ro_init(&node->raw_out, config, ctx);

    TBCI_Status s = sync_init(&node->sync, config, ctx);
    if (s != TBCI_OK) return s;

    return seg_init(&node->seg, config, ctx);
}

TBCI_NodeResult cn_process(TBCI_Core *node, TBCI_Context *ctx)
{
    if (node == NULL || ctx == NULL || ctx->inputs == NULL || ctx->epoch_queue == NULL)
        return TBCI_NODE_ERROR;

    TBCI_SyncResult sync_result = {0};
    TBCI_Input core_inputs = {
        .signal     = ctx->processed_signal,
        .triggers   = ctx->inputs->triggers,
        .n_channels = ctx->inputs->n_channels,
    };

    TBCI_EpochQueue* epoch_queue = ctx->epoch_queue;
    TBCI_NodeResult s = sync_process(&node->sync, &core_inputs, ctx, &sync_result);
    TBCI_NodeResult rw = ro_write(&node->raw_out, &sync_result, ctx);

    if (sync_result.trial_ended) {
        seg_reset(&node->seg);
        return TBCI_NODE_PENDING;
    }

    if (s != TBCI_NODE_OK) return s;

    TBCI_NodeResult seg_r = seg_process(&node->seg, &sync_result, epoch_queue, ctx);
    if (seg_r == TBCI_NODE_OK) {
        if (node->config->mode == SEG_MODE_TRIGGERED) {
            node->sync.state.synch_phase = SYNC_IDLE; // in Sliding Mode we keep RUNNING until trial ends
        }
    }
    return seg_r;
}

TBCI_Status cn_reset(TBCI_Core *node)
{
    if (node == NULL)
        return TBCI_ERR_INVALID_ARG;

    sync_reset(&node->sync);
    seg_reset(&node->seg);

    return TBCI_OK;
}
