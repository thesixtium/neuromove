/**
* @file tbci_seg_node.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Internal segmentation node — extracts fixed-length epochs from
 *        the continuous signal stream around synchronised trigger events.
 *
 * TBCI_SegNode is an internal component of TBCI_Core. It receives a
 * TBCI_SyncResult from the SyncNode and uses it to extract fixed-length
 * epochs from the signal buffer, pushing them into the epoch queue.
 *
 * It does NOT interact with the trigger queue or perform timestamp lookups —
 * that responsibility belongs to the SyncNode. SegNode only needs to know
 * where to start reading (window_start from TBCI_SyncResult) and what
 * trigger caused it (trigger from TBCI_SyncResult).
 *
 * ## State machine
 *
 * SEG_MODE_TRIGGERED:
 *   Stateless from seg's perspective — sync provides window_start and
 *   trigger each tick. Seg checks post-stimulus availability and extracts
 *   an epoch. Returns TBCI_NODE_PENDING if not enough data yet.
 *   SEG_IDLE and SEG_WAITING phases are unused in triggered mode.
 *
 * SEG_MODE_SLIDING:
 *   SEG_IDLE    — first sync result received, initialises window_start
 *                 and pending_trigger from sync result, transitions to
 *                 SEG_RUNNING.
 *   SEG_RUNNING — extracts one overlapping window per tick, advances
 *                 window_start by step_frames. Stays in SEG_RUNNING
 *                 until cn_process detects trial_ended from sync and
 *                 calls seg_reset, which returns to SEG_IDLE.
 *
 * ## Memory
 *
 * State is owned internally by TBCI_SegNode — no external allocation needed.
 * Config is shared with SyncNode via a pointer to TBCI_CoreConfig owned by
 * TBCI_Core.
 */

#ifndef TBCI_SEG_NODE_H
#define TBCI_SEG_NODE_H

#include "../../tbci_common.h"
#include "../../ioutils/tbci_input.h"
#include "../../containers/tbci_epoch_queue.h"
#include "../tbci_node.h"
#include "tbci_sync_node.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct TBCI_Context;

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */

/**
 * @brief Internal phase of the segmentation state machine.
 */
typedef enum {
    SEG_IDLE,    /**< Waiting for sync result.                              */
    SEG_WAITING, /**< Sync result received, waiting for post-stimulus data. */
    SEG_RUNNING, /**< Trial active, emitting overlapping windows. MI/SSVEP. */
} TBCI_SegmentationPhase;

/**
 * @brief Internal runtime state of the segmentation node.
 *
 * Owned by TBCI_SegNode by value. Zero-initialised at startup via seg_init.
 * Do not modify directly — use seg_reset to clear.
 *
 * pre_frames, post_frames, total_frames, overlap_frames and step_frames
 * are computed once at seg_init from TBCI_CoreConfig and target_srate.
 * They are preserved across seg_reset calls.
 *
 * window_start and pending_trigger are set from TBCI_SyncResult at the
 * start of each tick (triggered) or at trial start (sliding), and are
 * cleared by seg_reset.
 */
typedef struct {
    TBCI_SegmentationPhase phase;          /**< Current phase. SEG_RUNNING in sliding mode only.   */
    TBCI_Trigger           pending_trigger;/**< Trigger from sync result. Carries label/timestamp. */
    size_t                 pre_frames;     /**< Pre-stimulus frames, computed at init.             */
    size_t                 post_frames;    /**< Post-stimulus frames, computed at init.            */
    size_t                 total_frames;   /**< Total epoch frames = pre + post.                   */
    size_t                 overlap_frames; /**< Overlap frames. Sliding mode only.                 */
    size_t                 step_frames;    /**< total_frames - overlap_frames. Sliding mode only.  */
    uint64_t               window_start_us; /**< Timestamp of current window start. */
    uint64_t               spacing_us;      /**< Microseconds per frame.            */
} TBCI_SegmentationState;

/**
 * @brief Internal segmentation node.
 *
 * Extends TBCI_Node via composition. Owned by TBCI_Core — not registered
 * directly in the DAG. State is owned by value inside this struct.
 */
typedef struct {
    TBCI_Node base; /**< Base node. Must be first member.              */
    TBCI_CoreConfig* config; /**< Shared config, owned by TBCI_Core.            */
    TBCI_SegmentationState state; /**< Internal state, owned by this node.           */
} TBCI_SegNode;

/* --------------------------------------------------------------------------
 * API — internal to TBCI_Core
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialise the segmentation node.
 *
 * Computes frame counts from config and target_srate. Sets phase to SEG_IDLE.
 *
 * @param[out] node    Pointer to an uninitialised seg node. Must not be NULL.
 * @param[in]  config  Shared core config. Must not be NULL.
 * @param[in]  ctx     Pipeline context, used to read target_srate. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL or post_stimulus_ms is zero.
 * @return TBCI_WARN_PARADIGM_MODE_MISMATCH if mode and paradigm are inconsistent.
 */
TBCI_Status seg_init(TBCI_SegNode* node, TBCI_CoreConfig* config, struct TBCI_Context* ctx);

/**
 * @brief Run one tick of the segmentation state machine.
 *
 * Receives a TBCI_SyncResult from the SyncNode. Checks post-stimulus data
 * availability and extracts an epoch when ready. In sliding mode, advances
 * the window by step_frames each tick until the trial ends.
 *
 * @param[in,out] node         Pointer to an initialised seg node. Must not be NULL.
 * @param[in]     sync         Sync result from the SyncNode. Must not be NULL.
 * @param[in,out] epoch_queue  Destination queue for extracted epochs. Must not be NULL.
 * @param[in]     ctx          Pipeline context. Must not be NULL.
 * @return TBCI_NODE_OK      if an epoch was extracted and pushed.
 * @return TBCI_NODE_PENDING if waiting for more signal data.
 * @return TBCI_NODE_ERROR   if epoch push failed.
 */
TBCI_NodeResult seg_process(TBCI_SegNode* node, TBCI_SyncResult* sync, TBCI_EpochQueue* epoch_queue,
                            struct TBCI_Context* ctx);

/**
 * @brief Reset the segmentation node to its initial state.
 *
 * Clears pending trigger, phase, and window position.
 * Config and computed frame counts are preserved.
 *
 * @param[in,out] node  Pointer to an initialised seg node. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if node is NULL.
 */
TBCI_Status seg_reset(TBCI_SegNode* node);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_SEG_NODE_H */
