/**
 * @file tbci_sync_node.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Internal sync node — aligns trigger timestamps to signal buffer frames.
 *
 * SyncNode is an internal component of TBCI_Core. It is not part of the
 * public API and should not be included by any code outside tbci_core.c.
 *
 * ## Responsibility
 *
 * Given a trigger timestamp, SyncNode finds the corresponding frame in
 * the signal buffer and verifies that enough pre-stimulus data is available.
 * It produces a TBCI_SyncResult that SegmentationNode consumes to extract
 * the epoch without needing to know anything about timestamps or buffers.
 *
 * ## State machine
 *
 *   SYNC_IDLE     — pop trigger from queue, transition to SYNC_MATCHING
 *   SYNC_MATCHING — find timestamp in signal buffer, check pre_frames,
 *                   populate result, transition back to SYNC_IDLE
 */

#ifndef TBCI_SYNC_H
#define TBCI_SYNC_H

#include "../../tbci_common.h"
#include "../../ioutils/tbci_input.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
struct TBCI_Context;

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */

typedef enum
{
    SYNC_IDLE,      /**< Waiting for next trigger.                          */
    SYNC_MATCHING,  /**< Trigger found, searching signal buffer.            */
    SYNC_RUNNING,   /**< Trial active (sliding mode only).            */
} TBCI_SyncPhase;

typedef struct
{
    TBCI_SyncPhase synch_phase;     /**< Current phase.                    */
    TBCI_Trigger pending_trigger;   /**< Trigger being processed.          */
    size_t pre_frames;              /**< Computed at init from srate.      */
    size_t window_start;            /**< Current window start in signal buffer.         */
    uint64_t window_start_us;       /**< Timestamp of window start. Replaces logical index. */
    uint64_t spacing_us;            /**< Microseconds per frame. Computed at init. */
    bool new_trigger;               /**< true only when trigger freshly consumed this tick */
} TBCI_SyncState;

/**
 * @brief Output of a successful sync operation.
 *
 * Populated by sync_process when a trigger is matched to a signal frame.
 * Consumed by the segmentation stage to extract the epoch.
 */
typedef struct
{
    TBCI_Trigger trigger;           /**< The matched trigger, carries code and timestamp.  */
    TBCI_SignalBuffer* signal;      /**< Pointer to signal buffer. Enables OutNode to read the raw window without needing inputs. */
    bool trial_ended;               /**< True when sync detected end trigger. Sliding only.  */
    bool              new_trigger;  /**< true only when trigger freshly consumed this tick */
    uint64_t window_start_us;

} TBCI_SyncResult;

typedef struct
{
    TBCI_Node base;
    TBCI_CoreConfig* config;
    TBCI_SyncState state;
} TBCI_SyncNode;

/* --------------------------------------------------------------------------
 * API — internal use only
 * -------------------------------------------------------------------------- */
/**
 * @brief Initialise the sync node.
 *
 * Associates the node with its config, computes pre_frames from
 * pre_stimulus_ms and target_srate, and sets the initial phase to
 * SYNC_IDLE. No dynamic allocation is performed.
 *
 * @param[out] node    Pointer to an uninitialised sync node. Must not be NULL.
 * @param[in]  config  Shared core config. Must not be NULL.
 * @param[in]  ctx     Pipeline context, used to read target_srate. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status sync_init(TBCI_SyncNode *node, TBCI_CoreConfig *config, struct TBCI_Context *ctx);

/**
 * @brief Run one tick of the sync state machine.
 *
 * Called by cn_process each tick. Pops a trigger from the queue,
 * finds the corresponding frame in the signal buffer, verifies
 * pre-stimulus availability, and populates result_out on success.
 *
 * In triggered mode the state machine is:
 *   SYNC_IDLE     → pop trigger → SYNC_MATCHING
 *   SYNC_MATCHING → find frame, check pre_frames → SYNC_IDLE → OK
 *
 * In sliding mode the state machine is:
 *   SYNC_IDLE     → pop trigger, find frame → SYNC_MATCHING
 *   SYNC_MATCHING → populate result → SYNC_RUNNING → OK
 *   SYNC_RUNNING  → re-emit window_start each tick until end trigger
 *                   end trigger found → SYNC_IDLE, trial_ended=true → PENDING
 *
 * @param[in,out] node        Pointer to an initialised sync node. Must not be NULL.
 * @param[in,out] inputs      Pipeline inputs (signal buffer + trigger queue). Must not be NULL.
 * @param[in]     ctx         Pipeline context. Must not be NULL.
 * @param[out]    result_out  Populated on TBCI_NODE_OK. Must not be NULL.
 *                            result_out->trial_ended is always set before returning.
 * @return TBCI_NODE_OK      if a sync result was produced.
 * @return TBCI_NODE_PENDING if waiting for a trigger or enough signal data,
 *                           or if a trial end was detected (trial_ended=true).
 * @return TBCI_NODE_ERROR   if any pointer is NULL or an unrecoverable error occurred.
 */
TBCI_API TBCI_NodeResult sync_process(TBCI_SyncNode *node, TBCI_Input *inputs, struct TBCI_Context *ctx, TBCI_SyncResult  *result_out);

/**
 * @brief Reset the sync node to its initial state.
 *
 * Clears the pending trigger, window_start, and resets the phase to
 * SYNC_IDLE. Config and computed pre_frames are preserved — sync_init
 * does not need to be called again after reset.
 *
 * @param[in,out] node  Pointer to an initialised sync node. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if node is NULL.
 */
TBCI_API TBCI_Status sync_reset(TBCI_SyncNode *node);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_SYNC_H */