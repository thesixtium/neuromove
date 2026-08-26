/**
 * @file tbci_features_extraction.c
 */

#include "tbci_context.h"
#include "../../../include/nodes/features/tbci_features_extraction.h"

TBCI_Status fe_init(TBCI_FeatureExtraction *node, bool enabled, struct TBCI_Context *ctx)
{
    if (node == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    node->group.base.enabled = true;
    node->group.base.name    = "features";
    node->group.base.type    = TBCI_NODE_TYPE_FEATURE_EXTRACTION;
    node->group.base.tick_fn   = (TBCI_TopProcessFn)fe_process;
    node->extraction_enabled  = enabled;
    node->group.base.instance_size = sizeof(TBCI_FeatureExtraction);

    return group_init(&node->group, ctx);
}

TBCI_NodeResult fe_process(TBCI_FeatureExtraction *node, struct TBCI_Context *ctx)
{
    if (node == NULL || ctx == NULL)
        return TBCI_NODE_ERROR;


    if (eq_is_empty(ctx->epoch_queue))
        return TBCI_NODE_PENDING;

    TBCI_Epoch epoch;
    eq_pop(ctx->epoch_queue, &epoch);

    /* Always transpose time-major → channel-major into features_queue slot */
    float *dst = eq_next_slot(ctx->features_queue);
    if (dst == NULL)
        return TBCI_NODE_ERROR;
    fe_transpose_epoch(&epoch, dst, epoch.n_channels, epoch.n_frames);
    epoch.samples = dst;

    if (node->extraction_enabled) {
        TBCI_NodeResult r = group_process(&node->group, &epoch, ctx);
        if (r != TBCI_NODE_OK) return r;
    }
    TBCI_Status status = eq_push(ctx->features_queue, &epoch);

    if (status != TBCI_OK)
        return TBCI_NODE_ERROR;
    return TBCI_NODE_OK;
}

TBCI_Status fe_reset(TBCI_FeatureExtraction *node)
{
    if (node == NULL)
        return TBCI_ERR_INVALID_ARG;

    return group_reset(&node->group);
}

void fe_transpose_epoch(const TBCI_Epoch *src_epoch, float *dst, size_t n_channels, size_t n_frames)
{
    const float *src = src_epoch->samples;
    for (size_t ch = 0; ch < n_channels; ch++)
        for (size_t fr = 0; fr < n_frames; fr++)
            dst[ch * n_frames + fr] = src[fr * n_channels + ch];
}