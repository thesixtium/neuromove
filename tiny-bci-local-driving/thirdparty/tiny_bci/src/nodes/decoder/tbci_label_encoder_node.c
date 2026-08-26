/**
* @file tbci_label_encoder_node.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Label encoder node implementation.
 */

#include "nodes/decoder/tbci_label_encoder_node.h"
#include "tbci_context.h"

static TBCI_NodeResult le_process_fn(TBCI_Node *self, void *data, struct TBCI_Context *ctx)
{
    (void)ctx;
    TBCI_LabelEncoderNode *node  = (TBCI_LabelEncoderNode *)self;
    TBCI_Epoch            *epoch = (TBCI_Epoch *)data;
    return le_encode(node, epoch, ctx);
}

TBCI_Status le_init(TBCI_LabelEncoderNode *node, TBCI_LabelEncoderConfig *config)
{
    if (node == NULL) return TBCI_ERR_INVALID_ARG;

    node->base.name          = "label_encoder";
    node->base.type          = TBCI_NODE_TYPE_DECODER;
    node->base.enabled       = true;
    node->base.instance_size = sizeof(TBCI_LabelEncoderNode);
    node->base.init_fn       = NULL;
    node->base.process_fn    = le_process_fn;
    node->base.reset_fn      = NULL;
    node->base.tick_fn       = NULL;
    node->config = *config;

    return TBCI_OK;
}

TBCI_NodeResult le_encode(TBCI_LabelEncoderNode *node, TBCI_Epoch *epoch, struct TBCI_Context *ctx)
{
    if (node == NULL || epoch == NULL || ctx == NULL) return TBCI_NODE_ERROR;

    if (epoch->label == 0 || epoch->label > 127) {
        fprintf(stderr, "le_encode: invalid label %u — must be in [1, 127]\n", epoch->label);
        return TBCI_NODE_ERROR;
    }
    if (node->config.binarize_target)
        epoch->encoded_label = epoch->label == ctx->config.target_code? 0 : 1;
    else
        epoch->encoded_label = epoch->label - 1;
    epoch->predicted_label = -1;
    return TBCI_NODE_OK;
}

TBCI_Status le_decode(TBCI_LabelEncoderNode *node, TBCI_Epoch *epoch)
{
    if (node == NULL || epoch == NULL) return TBCI_ERR_INVALID_ARG;

    // if (epoch->predicted_label > -1)
    //     epoch->label = epoch->predicted_label + 1;
    // else
    //     epoch->label = epoch->encoded_label + 1;
    return TBCI_OK;
}