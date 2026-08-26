/**
 * @file tbci_context.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Top-level TinyBCI session context.
 *
 * The central object that owns or references all pipeline resources.
 * Created once at startup and passed to the runner each tick.
 *
 * ## Ownership
 *
 * TBCIContext holds pointers to externally-allocated buffers and queues.
 * The application is responsible for allocating and initialising:
 *   - TBCISignalBuffer  (via TBCIInputs)
 *   - TBCITriggerQueue  (via TBCIInputs)
 *   - TBCIEpochQueue
 *
 * TBCIContext owns the node array directly — nodes are stored by value,
 * not by pointer, to avoid heap allocation on embedded targets.
 *
 * ## Initialisation
 *
 * Always initialise via tbci_context_init(). Do not zero-initialise
 * manually as the function sets up node enable flags from TBCIConfig.
 *
 * @code
 * TBCIConfig  config  = { .paradigm = TBCI_PARADIGM_P300, ... };
 * TBCIContext ctx;
 * tbci_context_init(&ctx, &config, &inputs, &epoch_queue);
 * @endcode
 *
 * ## Embedded usage
 *
 * Declare as a static global on embedded targets. The context and
 * all referenced buffers should be statically allocated and live for
 * the duration of the program.
 */


#ifndef TBCI_CONTEXT_H
#define TBCI_CONTEXT_H

#include "pch.h"
#include "containers/tbci_epoch_queue.h"
#include "tbci_common.h"
#include "ioutils/tbci_input.h"
#include "nodes/tbci_node.h"
#include "nodes/core/tbci_core.h"
#include "tbci_config.h"
#include "nodes/preprocessing/tbci_preprocessing.h"
#include "nodes/features/tbci_features_extraction.h"
#include "nodes/decoder/tbci_decoder.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct TBCI_Context{
    TBCI_State      state;                    /**< Current operational state of the pipeline.           */
    TBCI_Config     config;                   /**< Pipeline configuration. Copied at init time.         */
    TBCI_Input      *inputs;                  /**< Pointer to shared input buffers. Not owned.          */
    TBCI_SignalBuffer *processed_signal;      /**< Processed signal stream. Written by the PreprocessingGroup */
    TBCI_EpochQueue   *epoch_queue;             /**< Pointer to epoch handoff queue. Not owned.           */
    TBCI_EpochQueue   *features_queue;        /**< Pointer to features handoff queue. Not owned.           */
    TBCI_EpochQueue   *output_queue;       /**< Optional. NULL if no downstream consumer.              */


    /* Computed at init time */
    size_t total_frames;                      /**< Total frames in an epoch           */

    /* Node registry — pointers to caller or internally owned nodes */
    TBCI_Node       *nodes[TBCI_MAX_NODES];   /**< Ordered node pointers. Set by pipeline builder.   */
    size_t           n_nodes;                 /**< Number of registered nodes.                       */

    TBCI_Preprocessing preprocessing;       /**< Preprocessing group.                       */
    TBCI_FeatureExtraction features;

    /* CoreNode group — always owned by context, built from config at init */
    TBCI_Core   core_node;                  /**< Segmentation node instance.                       */
    TBCI_CoreConfig core_config;            /**< Derived from TBCI_Config at init.                 */
    TBCI_SegmentationState  core_state;     /**< Runtime state for segmentation node.              */
    TBCI_SyncState        sync_state;       /**< Runtime state for sync node.                      */

    TBCI_Decoder  decoder;                  /**< Decoder group. Always registered, enabled by config. */

} TBCI_Context;


TBCI_API TBCI_Status tbci_context_init(TBCI_Context    *ctx,
                                       const TBCI_Config *config,
                                       TBCI_Input        *inputs,
                                       TBCI_SignalBuffer *processed_signal,
                                       TBCI_EpochQueue   *epoch_queue,
                                       TBCI_EpochQueue   *features_queue,
                                       TBCI_EpochQueue   *output_queue);

TBCI_API TBCI_Status tbci_context_start(TBCI_Context *ctx, TBCI_State state);
TBCI_API TBCI_Status tbci_context_stop(TBCI_Context *ctx);
TBCI_API TBCI_Status tbci_context_reset(TBCI_Context *ctx);
TBCI_API TBCI_Status tbci_context_tick(TBCI_Context *ctx);
TBCI_API TBCI_Status tbci_context_add_node(TBCI_Context *ctx, TBCI_Node *node);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_CONTEXT_H */