/**
 * @file tbci_decoder.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Base model interface and decoder group for the TinyBCI pipeline.
 *
 * TBCI_Model is the base struct extended by concrete decoder types
 * via composition (first-member pattern). Concrete types implement train,
 * eval, and infer function pointers.
 *
 * TBCI_Decoder is the 4th top-level DAG node. It reads from
 * ctx->features_queue, dispatches to inner decoder nodes based on
 * ctx->state, and writes results to ctx->output_queue (if set).
 *
 * ## State-driven dispatch
 *
 * - TBCI_STATE_TRAINING:  calls train() on each inner model node
 * - TBCI_STATE_INFERENCE: calls infer(), pushes result to output_queue
 * - TBCI_STATE_IDLE:      returns TBCI_NODE_PENDING
 *
 * ## Online training flow
 *
 * 1. Send TRAIN command (194) → pipeline enters TBCI_STATE_TRAINING
 * 2. Epochs accumulate, train() called each tick
 * 3. Send IDLE command (192) → tbci_context_stop calls eval() automatically
 * 4. Send INFERENCE command (193) → pipeline enters TBCI_STATE_INFERENCE
 *
 * ## Memory
 *
 * No dynamic allocation. Concrete decoder types own their model weights
 * as fixed-size arrays sized by build-time defines.
 *
 * ## Disabling
 *
 * Set config.use_decoder = false to disable the group. The group is
 * still registered in the DAG but returns TBCI_NODE_PENDING immediately,
 * leaving features_queue unconsumed for external readers (Python, Unity).
 *
 * ## Guarded queue
 *
 * output_queue on context is optional (NULL is valid) when the group
 * is disabled. If the group is enabled but output_queue is NULL,
 * dc_init returns TBCI_ERR_INVALID_STATE.
 */

#ifndef TBCI_DECODER_H
#define TBCI_DECODER_H

#include "../../tbci_common.h"
#include "../tbci_node_group.h"
#include "mathutils/tbci_math.h"

#ifdef __cplusplus
extern "C" {
#endif


/* --------------------------------------------------------------------------
 * Model type and output mode enum
 * -------------------------------------------------------------------------- */
typedef enum {
    TBCI_OUTPUT_SOFTMAX,  /**< n_classes outputs, argmax → predicted_class */
    TBCI_OUTPUT_SIGMOID,  /**< single output, threshold → predicted_class   */
} TBCI_OutputMode;

/**
 * @brief Identifies the concrete model implementation.
 *
 * Descriptive metadata only — dispatch is via function pointers,
 * not a switch on this value.
 */
typedef enum {
    TBCI_CCA_MODEL,   /**< Argmax on CCA correlation vector. No training needed. SSVEP only.  */
    TBCI_LDA_MODEL,   /**< Linear Discriminant Analysis. Requires calibration. MI/P300.        */
    TBCI_ONNX_MODEL,  /**< ONNX Runtime inference. Pretrained or fine-tunable backbone.        */
} TBCI_ModelType;

/* --------------------------------------------------------------------------
 * Base decoder
 * -------------------------------------------------------------------------- */

/**
 * @brief Base decoder struct. Extended by concrete types via composition.
 *
 * Concrete types embed TBCI_Model as their first member for safe casting.
 * Function pointers are set by the concrete type's init function.
 *
 * infer() writes class probabilities back into epoch->samples in-place:
 *   epoch->n_channels = n_classes
 *   epoch->n_frames   = 1
 *   epoch->samples    = [prob_class0, prob_class1, ..., prob_classN]
 *
 * train() accumulates training data. The decoder is not usable for
 * inference until eval() confirms training quality is acceptable.
 *
 * eval() is called automatically by tbci_context_stop when transitioning
 * out of TBCI_STATE_TRAINING. It writes a quality metric (e.g. accuracy)
 * to accuracy_out.
 */
typedef struct TBCI_Model {
    TBCI_Node       base;         /**< Inner node base_model. MUST be first member.                        */
    TBCI_ModelType  type;         /**< Concrete decoder type. Descriptive only.                   */
    float        eval_score;      /**< quality metric from last eval(), metric-agnostic */
    float        confidence;      /**< probability of predicted class, set by infer()   */
    int          predicted_class; /**< 0-based predicted class, last infer() */
    TBCI_ScorerFn scorer;         /**< default: tbci_score_accuracy if NULL */

    /**
     * @brief Accumulate one training epoch.
     *
     * Called automatically by dc_process when ctx->state == TBCI_STATE_TRAINING.
     *
     * @param[in,out] self   Pointer to the concrete model. Must not be NULL.
     * @param[in]     epoch  Feature epoch from features_queue. Must not be NULL.
     * @return TBCI_OK on success.
     */
    TBCI_Status (*train)(struct TBCI_Model *self, TBCI_Epoch *epoch);

    /**
     * @brief Evaluate training quality.
     *
     * Called automatically by tbci_context_stop after TBCI_STATE_TRAINING ends.
     * Writes a quality metric to accuracy_out (e.g. cross-validated accuracy).
     *
     * @param[in,out] self          Pointer to the concrete model. Must not be NULL.
     * @param[out]    accuracy_out  Quality metric in [0, 1]. Must not be NULL.
     * @return TBCI_OK on success.
     * @return TBCI_ERR_INVALID_STATE if not enough training data has been accumulated.
     */
    TBCI_Status (*eval)(struct TBCI_Model *self, float *accuracy_out);

    /**
     * @brief Run inference on one feature epoch.
     *
     * Called automatically by dc_process when ctx->state == TBCI_STATE_INFERENCE.
     * Writes class probabilities into epoch->samples in-place.
     *
     * @param[in,out] self   Pointer to the concrete model. Must not be NULL.
     * @param[in,out] epoch  Feature epoch. samples overwritten with probabilities.
     * @return TBCI_OK on success.
     * @return TBCI_ERR_INVALID_STATE if the model has not been trained.
     */
    TBCI_Status (*infer)(struct TBCI_Model *self, TBCI_Epoch *epoch);
} TBCI_Model;

/* --------------------------------------------------------------------------
 * Decoding group
 * -------------------------------------------------------------------------- */

/**
 * @brief Top-level DAG node that chains decoding stages.
 *
 * Reads from ctx->features_queue, dispatches to inner nodes based on ctx->state, writes to ctx->output_queue.
 *
 * Inner nodes must be TBCI_Model instances (or concrete subtypes).
 * Registered via group_add_node before dc_init is called.
 */
typedef struct {
    TBCI_NodeGroup group;                /**< DAG-visible base. MUST be first member.               */
    bool           decoding_enabled; /**< Gates inner node dispatch. Passthrough if false.    */
} TBCI_Decoder;

/**
 * @brief Initialise the decoder group.
 *
 * Wires dc_process into group.base.tick_fn. If enabled, validates that
 * ctx->output_queue is not NULL.
 *
 * @param[out] node     Pointer to an uninitialised group. Must not be NULL.
 * @param[in]  enabled  Whether classification dispatch is active.
 * @param[in]  ctx      Pipeline context. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if node or ctx is NULL.
 * @return TBCI_ERR_INVALID_STATE if enabled but ctx->output_queue is NULL.
 */
TBCI_API TBCI_Status dc_init(TBCI_Decoder *node, bool enabled,
                              struct TBCI_Context *ctx);

/**
 * @brief Run one tick of the decoder group.
 *
 * Pops one epoch from ctx->features_queue. Dispatches based on ctx->state:
 * - TBCI_STATE_TRAINING:  calls train() on each inner node
 * - TBCI_STATE_INFERENCE: calls infer() on each inner node, pushes to output_queue
 * - TBCI_STATE_IDLE:      returns TBCI_NODE_PENDING
 *
 * If decoding_enabled is false, pops and pushes epoch unchanged (passthrough).
 *
 * @param[in,out] node  Pointer to an initialised group. Must not be NULL.
 * @param[in,out] ctx   Pipeline context. Must not be NULL.
 * @return TBCI_NODE_OK on success.
 * @return TBCI_NODE_PENDING if features_queue is empty or state is IDLE.
 * @return TBCI_NODE_ERROR on failure.
 */
TBCI_API TBCI_NodeResult dc_process(TBCI_Decoder *node,
                                     struct TBCI_Context *ctx);

/**
 * @brief Reset the decoder group and all inner nodes.
 *
 * @param[in,out] node  Pointer to an initialised group. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if node is NULL.
 */
TBCI_API TBCI_Status dc_reset(TBCI_Decoder *node);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_DECODER_H */