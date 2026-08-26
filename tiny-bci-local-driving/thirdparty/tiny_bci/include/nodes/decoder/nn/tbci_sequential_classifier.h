/**
 * @file tbci_sequential_classifier.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Sequential fully-connected classifier for TinyBCI decoder group.
 *
 * Chains an ordered sequence of TBCI_LinearLayer instances. Extends TBCI_Model
 * via composition — base_model must be first member for safe casting.
 *
 * Supports online training via SGD with gradient clipping. Evaluation via
 * K-fold cross-validation using the shared TBCI_ScorerFn interface.
 *
 * ## Typical use cases
 *
 * - Standalone classifier after CCA or other feature extraction
 * - FC head attached after an ONNX backbone (TBCI_ONNXModel backbone+FC mode)
 *
 * ## Memory
 *
 * No dynamic allocation. All layer storage is fixed-size arrays on the struct
 * sized by TBCI_MAX_SEQ_LAYERS and TBCI_MAX_SEQ_NEURONS. Calibration trial
 * storage is caller-provided via config.
 *
 * RAM estimate with defaults (4 layers, 128 neurons):
 *   4 layers * ~131KB per layer = ~524KB
 * Override at build time:
 *   -DTBCI_MAX_SEQ_LAYERS=1 -DTBCI_MAX_SEQ_NEURONS=64
 *
 * ## Usage
 *
 * @code
 * float train_trials[MAX_TRIALS * INPUT_SIZE];
 * uint16_t train_labels[MAX_TRIALS];
 *
 * TBCI_SequentialConfig cfg = {
 *     .n_layers       = 2,
 *     .layer_sizes    = {64, 32, 2},   // input=64, hidden=32, output=2
 *     .activations    = {TBCI_ACT_RELU, TBCI_ACT_SOFTMAX},
 *     .weight_init    = TBCI_INIT_HE,
 *     .learning_rate  = 0.001f,
 *     .grad_clip      = 1.0f,
 *     .n_folds        = 5,
 *     .scorer         = tbci_score_accuracy,
 *     .train_trials   = train_trials,
 *     .train_labels   = train_labels,
 *     .train_capacity = MAX_TRIALS,
 * };
 * TBCI_SequentialClassifier clf;
 * seq_init(&clf, &cfg, &ctx);
 * group_add_node(&ctx.decoder.group, (TBCI_Node *)&clf);
 * @endcode
 */

#ifndef TBCI_SEQUENTIAL_CLASSIFIER_H
#define TBCI_SEQUENTIAL_CLASSIFIER_H

#include "../tbci_decoder.h"
#include "tbci_linear.h"
#include "../../../mathutils/tbci_activations.h"
#include "mathutils/tbci_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Build-time defines
 * -------------------------------------------------------------------------- */

/** Maximum number of layers. Override at build time. */
#ifndef TBCI_MAX_SEQ_LAYERS
#define TBCI_MAX_SEQ_LAYERS 4
#endif

/* --------------------------------------------------------------------------
 * Configuration
 * -------------------------------------------------------------------------- */

typedef struct {
    /* topology */
    size_t            n_layers;                              /**< Number of layers (excluding input). */
    size_t            layer_sizes[TBCI_MAX_SEQ_LAYERS + 1];  /**< Sizes: [input, layer0, ..., layerN].*/
    TBCI_ActivationFn activations[TBCI_MAX_SEQ_LAYERS];      /**< Activation per layer.               */
    TBCI_WeightInit   weight_init;                           /**< Weight initialization strategy.     */

    /* training */
    float           learning_rate;  /**< SGD step size.                                */
    float           grad_clip;      /**< Max gradient norm. 0.0f disables clipping.    */
    size_t          n_folds;        /**< K for K-fold cross-validation in eval().       */
    TBCI_ScorerFn   scorer;         /**< Scoring function. NULL defaults to accuracy.  */

    /* calibration storage — caller-provided */
    float          *train_trials;   /**< [train_capacity * input_size] flat buffer.    */
    uint16_t       *train_labels;   /**< [train_capacity] label buffer.                */
    size_t          train_capacity; /**< Max trials the caller allocated for.           */
} TBCI_SequentialConfig;

/* --------------------------------------------------------------------------
 * Node struct
 * -------------------------------------------------------------------------- */

/**
 * @brief Sequential classifier node. Extends TBCI_Model via composition.
 *
 * base_model must be first member for safe casting to TBCI_Model* and TBCI_Node*.
 */
typedef struct {
    TBCI_Model            base_model;                        /**< Base model. MUST be first member.         */
    TBCI_SequentialConfig config;                            /**< Configuration. Copied at init.            */
    TBCI_LinearLayer      layers[TBCI_MAX_SEQ_LAYERS];      /**< Ordered layer array.                      */

    /* forward pass scratch — input to each layer, needed for backprop */
    float                 layer_inputs[TBCI_MAX_SEQ_LAYERS + 1][TBCI_MAX_SEQ_NEURONS]; /**< Cached inputs. */

    /* training state */
    size_t                train_count;  /**< Trials accumulated so far.                */
    float                 eval_score;   /**< Last eval score. -1.0f if not evaluated.  */
} TBCI_SequentialClassifier;

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialise the sequential classifier.
 *
 * Initialises all layers, applies weight initialization strategy, wires
 * train/eval/infer and process_fn to base_model function pointers.
 *
 * @param[out] clf     Must not be NULL.
 * @param[in]  config  Must not be NULL. n_layers <= TBCI_MAX_SEQ_LAYERS,
 *                     all layer_sizes <= TBCI_MAX_SEQ_NEURONS.
 * @param[in]  ctx     Pipeline context. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL or config out of range.
 */
TBCI_API TBCI_Status seq_init(TBCI_SequentialClassifier *clf,
                               TBCI_SequentialConfig     *config,
                               struct TBCI_Context       *ctx);

/**
 * @brief Accumulate one training trial.
 *
 * Copies epoch->samples into train_trials and stores epoch->encoded_label
 * into train_labels. Increments train_count.
 * Logs a warning and returns TBCI_WARN_FULL_TRIALS if capacity reached.
 *
 * @param[in,out] self   Must not be NULL.
 * @param[in]     epoch  Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_WARN_FULL_TRIALS if train_capacity reached (trial skipped).
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status seq_train(TBCI_Model *self, TBCI_Epoch *epoch);

/**
 * @brief Train the classifier on accumulated trials using SGD.
 *
 * Runs forward + backward pass on all accumulated training trials.
 * Called automatically by tbci_context_stop when leaving TBCI_STATE_TRAINING.
 * Writes eval score to score_out via K-fold cross-validation.
 *
 * @param[in,out] self       Must not be NULL.
 * @param[out]    score_out  Quality metric in [0, 1]. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_STATE if train_count < n_folds.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status seq_eval(TBCI_Model *self, float *score_out);

/**
 * @brief Run inference on one feature epoch.
 *
 * Runs forward pass through all layers. Writes class probabilities into
 * epoch->samples in-place. Sets base_model.predicted_class, confidence
 * and eval_score on epoch.
 *
 * @param[in,out] self   Must not be NULL.
 * @param[in,out] epoch  Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 * @return TBCI_ERR_INVALID_STATE if input size mismatches layer_sizes[0].
 */
TBCI_API TBCI_Status seq_infer(TBCI_Model *self, TBCI_Epoch *epoch);

/**
 * @brief Reset training state and zero all gradients.
 *
 * Does not reset weights — call seq_init to reinitialize from scratch.
 *
 * @param[in,out] clf  Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if clf is NULL.
 */
TBCI_API TBCI_Status seq_reset(TBCI_SequentialClassifier *clf);

/**
 * @brief Save all layer weights to a caller-provided buffer.
 *
 * Serializes layers in order. Buffer must be at least seq_weight_size(clf) bytes.
 *
 * @param[in]  clf  Must not be NULL.
 * @param[out] buf  Caller-provided buffer. Must not be NULL.
 * @param[in]  len  Buffer length in bytes.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL or buf too small.
 */
TBCI_API TBCI_Status seq_save(const TBCI_SequentialClassifier *clf,
                               float *buf, size_t len);

/**
 * @brief Load all layer weights from a caller-provided buffer.
 *
 * @param[in,out] clf  Must not be NULL.
 * @param[in]     buf  Caller-provided buffer. Must not be NULL.
 * @param[in]     len  Buffer length in bytes.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL or buf too small.
 */
TBCI_API TBCI_Status seq_load(TBCI_SequentialClassifier *clf,
                               const float *buf, size_t len);

/**
 * @brief Returns the number of bytes needed to serialize all layer weights.
 *
 * @param[in] clf  Must not be NULL.
 * @return Size in bytes, or 0 if clf is NULL.
 */
size_t seq_weight_size(const TBCI_SequentialClassifier *clf);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_SEQUENTIAL_CLASSIFIER_H */