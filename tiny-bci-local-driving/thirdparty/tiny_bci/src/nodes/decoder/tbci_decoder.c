/**
 * @file tbci_decoder.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief TinyBCI decoder implementation.
 */

#include "tbci_context.h"
#include "../../../include/nodes/decoder/tbci_decoder.h"

#include "nodes/decoder/tbci_label_encoder_node.h"

TBCI_Status dc_init(TBCI_Decoder* node, bool enabled, struct TBCI_Context* ctx)
{
    if (node == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (enabled && ctx->output_queue == NULL)
        return TBCI_ERR_INVALID_STATE;

    node->group.base.enabled = true;
    node->group.base.name    = "decoder";
    node->group.base.type    = TBCI_NODE_TYPE_DECODER;
    node->group.base.tick_fn = (TBCI_TopProcessFn) dc_process;
    node->group.base.instance_size = sizeof(TBCI_Decoder);
    node->decoding_enabled = enabled;

    return group_init(&node->group, ctx);
}

TBCI_NodeResult dc_process(TBCI_Decoder* node, struct TBCI_Context* ctx)
{
    if (node == NULL || ctx == NULL)
        return TBCI_NODE_ERROR;

    if (eq_is_empty(ctx->features_queue) || ctx->state == TBCI_STATE_IDLE)
        return TBCI_NODE_PENDING;

    TBCI_Epoch epoch;
    eq_pop(ctx->features_queue, &epoch);
    epoch.predicted_label = -1;

    if (node->decoding_enabled) {
        TBCI_NodeResult r = group_process(&node->group, &epoch, ctx);
        if (r != TBCI_NODE_OK) return r;

        TBCI_Status s;
        /* find encoder and decode before pushing to output_queue */
        TBCI_LabelEncoderNode *enc = NULL;
        for (size_t i = 0; i < node->group.n_nodes; i++) {
            if (node->group.nodes[i]->name != NULL &&
                strcmp(node->group.nodes[i]->name, "label_encoder") == 0) {
                enc = (TBCI_LabelEncoderNode *)node->group.nodes[i];
                break;
                }
        }
        if (enc != NULL)
        {
            s = le_decode(enc, &epoch);
            if (s != TBCI_OK) return TBCI_NODE_ERROR;
        }

        /* only push during inference when enabled */
        if (ctx->state == TBCI_STATE_INFERENCE && ctx->output_queue != NULL) {
            s = eq_push(ctx->output_queue, &epoch);
            if (s != TBCI_OK) return TBCI_NODE_ERROR;
        }
    } else {
        /* disabled — passthrough to output_queue regardless of state */
        if (ctx->output_queue != NULL) {
            TBCI_Status s = eq_push(ctx->output_queue, &epoch);
            if (s != TBCI_OK) return TBCI_NODE_ERROR;
        }
    }
    return TBCI_NODE_OK;
}

TBCI_Status dc_reset(TBCI_Decoder* node)
{
    if (node == NULL)
        return TBCI_ERR_INVALID_ARG;
    return group_reset(&node->group);
}
