/**
* @file tbci_core.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Core pipeline stage — orchestrates signal synchronisation
 *        and epoch segmentation.
 *
 * TBCI_Core is a special NodeGroup that internally contains and sequences a SyncNode and a
 * SegmentationNode. The only nodes that can be inserted between them are
 * OutNodes for data tapping. From the outside, the pipeline sees TBCI_Core
 * as a group with one init, one process tick, and one reset.
 *
 * ## Internal architecture
 *
 *   SyncNode        — aligns trigger timestamps to signal buffer frames
 *   (OutNode?)      — optional data tap between sync and segmentation
 *   SegmentationNode — extracts fixed-length epochs around trigger events
 *
 * ## State machine
 *
 * TBCI_Core delegates its state machine to the internal SegmentationNode:
 *
 *   SEG_IDLE    — waiting for next trigger
 *   SEG_WAITING — trigger found, waiting for enough signal data (triggered mode)
 *   SEG_RUNNING — trial active, emitting overlapping windows (sliding mode)
 *
 * ## Epoch extraction
 *
 *   pre_frames   = pre_stimulus_ms  / 1000 * target_srate
 *   post_frames  = post_stimulus_ms / 1000 * target_srate
 *   total_frames = pre_frames + post_frames
 *
 * ## Memory
 *
 * No dynamic allocation. All storage is caller-provided.
 *
 * @code
 * TBCI_CoreConfig core_config = {
 *     .mode             = SEG_MODE_TRIGGERED,
 *     .pre_stimulus_ms  = 200,
 *     .post_stimulus_ms = 800,
 * };
 * TBCI_CoreState core_state;
 * TBCI_Core      core_node;
 *
 * cn_init(&core_node, &core_config, &core_state, &ctx);
 * @endcode
 */

#ifndef TBCI_CORE_H
#define TBCI_CORE_H

#include "../../tbci_common.h"
#include "../../ioutils/tbci_input.h"
#include "../tbci_node.h"
#include "tbci_sync_node.h"
#include "tbci_seg_node.h"
#include "tbci_raw_out.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */
/* Forward declaration to avoid circular dependency */
struct TBCI_Context;
/**
 * @brief Core pipeline stage.
 *
 * Internally orchestrates SyncNode → (OutNode?) → SegmentationNode.
 * Registered in TBCI_Context as a TBCI_Node* via the base member.
 * External code treats this as an opaque unit.
 */
typedef struct {
    TBCI_Node base;               /**< DAG-visible base. Must be first member for safe casting. */
    TBCI_CoreConfig *config;      /**< Shared config for both nodes.    */
    TBCI_SyncNode    sync;        /**< Internal sync node.              */
    TBCI_RawOutNode  raw_out;     /**< Optional raw EEG logger. NULL = disabled. */
    TBCI_SegNode     seg;         /**< Internal segmentation node.      */
} TBCI_Core;

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialise the segmentation node.
 *
 * Associates the node with its config and state, computes frame counts
 * from window lengths and target_srate, and sets the initial phase to
 * SEG_IDLE. No dynamic allocation is performed.
 *
 * @param[out] node    Pointer to an uninitialised segmentation node. Must not be NULL.
 * @param[in]  seg_config  Caller-owned configuration struct. Must not be NULL.
 * @param[in]  ctx     Pipeline context, used to read target_srate. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL or post_stimulus_ms is zero.
 */
TBCI_API TBCI_Status cn_init(TBCI_Core *node, TBCI_CoreConfig *seg_config, struct TBCI_Context *ctx);

/**
 * @brief Run one tick of the segmentation state machine.
 *
 * Called by the DAG runner each tick. Peeks at the trigger queue,
 * checks signal buffer availability, and extracts an epoch when ready.
 * Returns TBCI_NODE_PENDING if no epoch could be produced this tick.
 *
 * @param[in,out] node         Pointer to an initialised segmentation node. Must not be NULL.
 * @param[in]     ctx          Pipeline context, used to read target_srate. Must not be NULL.
 * @return TBCI_NODE_OK      if an epoch was successfully extracted and pushed.
 * @return TBCI_NODE_PENDING if waiting for a trigger or more signal data.
 * @return TBCI_NODE_ERROR   if epoch push failed or an unrecoverable error occurred.
 */
TBCI_API TBCI_NodeResult cn_process(TBCI_Core *node, struct TBCI_Context *ctx);

/**
 * @brief Reset the segmentation node to its initial state.
 *
 * Clears the pending trigger and resets the phase to SEG_IDLE.
 * Config and computed frame counts are preserved — sn_init does
 * not need to be called again after reset.
 *
 * @param[in,out] node  Pointer to an initialised segmentation node. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if node is NULL.
 */
TBCI_API TBCI_Status cn_reset(TBCI_Core *node);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_CORE_H */
