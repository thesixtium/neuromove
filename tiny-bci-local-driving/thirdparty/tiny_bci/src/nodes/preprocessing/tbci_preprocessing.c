/**
 * @file tbci_preprocessing.c
 */

#include "tbci_context.h"
#include "../../../include/nodes/preprocessing/tbci_preprocessing.h"

static void pp_write_cb(const float *samples, const TBCI_Frame *frame, void *user_data)
{
    PpWriteCtx *wctx = (PpWriteCtx *)user_data;
    size_t n = wctx->ctx->config.n_channels;

#ifdef _MSC_VER
    float *filtered = (float *)malloc(n * sizeof(float));
    if (!filtered) return;
#else
    float filtered[n];  /* VLA — safe on GCC/Clang */
#endif

    memcpy(filtered, samples, n * sizeof(float));

    if (wctx->node->filtering_enabled)
    {
        group_process(&wctx->node->group, filtered, wctx->ctx);
    }

    sb_put(wctx->ctx->processed_signal, filtered, frame->timestamp_us, frame->sample_index);
    wctx->node->last_processed_ts = frame->timestamp_us;

#ifdef _MSC_VER
    free(filtered);
#endif
}

TBCI_Status pp_init(TBCI_Preprocessing* node, bool enabled, struct TBCI_Context* ctx)
{
    if (node == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    node->group.base.enabled = true;
    node->group.base.name = "preprocessing";
    node->group.base.type = TBCI_NODE_TYPE_PREPROCESSING;
    node->group.base.tick_fn = (TBCI_TopProcessFn)pp_process;
    node->filtering_enabled = enabled;
    node->group.base.instance_size = sizeof(TBCI_Preprocessing);
    node->last_processed_ts = 0;

    return group_init(&node->group, ctx);
}

TBCI_NodeResult pp_process(TBCI_Preprocessing *node, struct TBCI_Context *ctx)
{
    if (node == NULL || ctx == NULL) return TBCI_NODE_ERROR;

    if (ctx->config.n_channels > TBCI_MAX_CHANNELS) {
        fprintf(stderr, "pp_process: n_channels=%zu exceeds TBCI_MAX_CHANNELS=%d\n",
                ctx->config.n_channels, TBCI_MAX_CHANNELS);
        return TBCI_NODE_ERROR;
    }

    PpWriteCtx wctx = { .node = node, .ctx = ctx };
    TBCI_Status s = sb_read_since(ctx->inputs->signal, node->last_processed_ts, pp_write_cb, &wctx);

    if (s == TBCI_ERR_EMPTY) return TBCI_NODE_PENDING;
    if (s != TBCI_OK)        return TBCI_NODE_ERROR;

    return TBCI_NODE_OK;
}

TBCI_Status pp_reset(TBCI_Preprocessing* node)
{
    if (node == NULL)
        return TBCI_ERR_INVALID_ARG;

    return group_reset(&node->group);
}
