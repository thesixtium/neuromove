/**
* @file tbci_preprocessing.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief A group of nodes dedicated to pre-process the signal TinyBCI pipeline.
 *
 * Each node encapsulates one processing stage. The group has visibility on the shared
 * Context, from which reads input and finally writes in the processed_signal buffer.
 *
 * Inner nodes feeds their output to the following nodes and there is no visibility
 * on the shared Context. Data type is shared.
 *

 */

#ifndef TBCI_PREPROCESSING_H
#define TBCI_PREPROCESSING_H

#include "../../tbci_common.h"
#include "../tbci_node_group.h"

#ifdef __cplusplus
extern "C" {

#endif

typedef struct
{
    TBCI_NodeGroup group;           /**< External macro node with visibility on the context, MUST be first member of the struct */
    bool filtering_enabled;         /**< Whether data pass-through the inner nodes is enabled */
    uint64_t last_processed_ts;     /**< timestamp of last frame copied to processed_signal */
} TBCI_Preprocessing;


typedef struct {
    TBCI_Preprocessing *node;
    struct TBCI_Context *ctx;
} PpWriteCtx;


/**
 * @brief Initialise the preprocessing group.
 *
 * @param[out] node    Pointer to an uninitialised group. Must not be NULL.
 * @param[in]  enabled  Wether the node should be enabled or skipped through.
 * @param[in]  ctx     Pipeline context. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status pp_init(TBCI_Preprocessing *node, bool enabled, struct TBCI_Context *ctx);

/**
 * @brief Run one tick of the preprocessing group.
 *
 * If disabled or no internal nodes registered, copies the newest frame
 * from ctx->signal_buf to ctx->processed_buf unchanged.
 *
 * @param[in,out] node  Pointer to an initialised group. Must not be NULL.
 * @param[in,out] ctx   Pipeline context. Must not be NULL.
 * @return TBCI_NODE_OK on success.
 * @return TBCI_NODE_ERROR if node or ctx is NULL.
 */
TBCI_API TBCI_NodeResult pp_process(TBCI_Preprocessing *node, struct TBCI_Context *ctx);

/**
 * @brief Reset the preprocessing group to its initial state.
 *
 * @param[in,out] node  Pointer to an initialised group. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if node is NULL.
 */
TBCI_API TBCI_Status pp_reset(TBCI_Preprocessing *node);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_PREPROCESSING_H */
