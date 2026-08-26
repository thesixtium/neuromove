/**
 * @file tbci_label_encoder_node.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Label encoder inner node for the TinyBCI decoder group.
 *
 * Maps trigger codes [1-127] to class indices [0-126] before model inference,
 * and decodes predicted class indices back to trigger codes after inference.
 *
 * ## Encoding
 *   encoded_label = label - 1
 *
 * ## Decoding
 *   label = encoded_label + 1
 *
 * ## Usage
 *
 * Register as the first inner node in the decoder group — before any model node.
 * dc_process will automatically find it and call le_decode before pushing to
 * output_queue.
 *
 * @code
 * TBCI_LabelEncoderNode encoder;
 * le_init(&encoder);
 * group_add_node(&ctx.decoder.group, (TBCI_Node *)&encoder);
 * group_add_node(&ctx.decoder.group, (TBCI_Node *)&onnx_model);
 * @endcode
 */

#ifndef TBCI_LABEL_ENCODER_NODE_H
#define TBCI_LABEL_ENCODER_NODE_H

#include "../tbci_node.h"
#include "../../tbci_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration for the CCA model.
 */
typedef struct {
    bool binarize_target;       /**< For P300, makes all Targets=0 and all NonTargets=1 */
} TBCI_LabelEncoderConfig;
/**
 * @brief Label encoder node. No configuration needed.
 *
 * base must be first member for safe casting to TBCI_Node*.
 */
typedef struct {
    TBCI_Node base;                 /**< Inner node base. MUST be first member. */
    TBCI_LabelEncoderConfig config; /**< Configuration for the Label Encoder node. */
} TBCI_LabelEncoderNode;

/**
 * @brief Initialise the label encoder node.
 *
 * Wires process_fn. No ctx needed.
 *
 * @param[out] node  Pointer to an uninitialised node. Must not be NULL.
 * @param[in] config Label Encoder configuration. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if node is NULL.
 */
TBCI_API TBCI_Status le_init(TBCI_LabelEncoderNode *node, TBCI_LabelEncoderConfig *config);

/**
 * @brief Encode epoch->label to epoch->encoded_label.
 *
 * encoded_label = label - 1.
 * Called automatically by group_process during dc_process.
 *
 * @param[in,out] node   Must not be NULL.
 * @param[in,out] epoch  Must not be NULL. label must be in [1, 127].
 * @param[in] ctx   Pipeline context. Must not be NULL.
 * @return TBCI_NODE_OK on success.
 * @return TBCI_NODE_ERROR if label is 0 or > 127.
 */
TBCI_API TBCI_NodeResult le_encode(TBCI_LabelEncoderNode *node, TBCI_Epoch *epoch, struct TBCI_Context *ctx);

/**
 * @brief Decode epoch->encoded_label back to epoch->label.
 *
 * label = encoded_label + 1.
 * Called by dc_process after group_process, before pushing to output_queue.
 *
 * @param[in,out] node   Must not be NULL.
 * @param[in,out] epoch  Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status le_decode(TBCI_LabelEncoderNode *node, TBCI_Epoch *epoch);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_LABEL_ENCODER_NODE_H */