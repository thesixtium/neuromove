/**
 * @file tbci_onnx_model.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief ONNX Runtime inference model for the TinyBCI decoder group.
 *
 * Wraps an ONNX Runtime session as a TBCI_Model inner node.
 * Input/output tensor dimensions are read from the model file at init time.
 *
 * ## Pretrained mode (current)
 *
 * train() accumulates epochs and labels for cross-validated evaluation.
 * eval()  runs K-fold cross-validation over accumulated trials.
 * infer() runs a forward pass and writes class probabilities into epoch->samples.
 *
 * ## Backbone + FC head (future)
 *
 * Not yet implemented. train() will fine-tune the FC head via SGD/Adam.
 *
 * ## Memory
 *
 * No dynamic allocation in library core. IO tensor buffers are fixed-size
 * arrays on the struct sized by build-time defines. Calibration trial storage
 * is caller-provided via config.
 *
 * ## Guards
 *
 * Entire file is #ifdef TBCI_WITH_ONNX guarded. Bare-metal targets omit
 * the decoder group entirely rather than linking this node.
 *
 * ## Usage
 *
 * @code
 * float train_trials[MAX_TRIALS * MY_INPUT_SIZE];
 * uint16_t train_labels[MAX_TRIALS];
 *
 * TBCI_ONNXModelConfig cfg = {
 *     .model_path     = "eegnet.onnx",
 *     .train_trials   = train_trials,
 *     .train_labels   = train_labels,
 *     .train_capacity = MAX_TRIALS,
 *     .n_folds        = 5,
 * };
 * TBCI_ONNXModel model;
 * onnx_model_init(&model, &cfg, ctx);
 * group_add_node(&ctx.decoder.group, (TBCI_Node *)&model);
 * @endcode
 */

#ifndef TBCI_ONNX_MODEL_H
#define TBCI_ONNX_MODEL_H

#ifdef TBCI_WITH_ONNX

#include "tbci_decoder.h"
#include "mathutils/tbci_math.h"
#include <onnxruntime_c_api.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Build-time defines
 * -------------------------------------------------------------------------- */
#define TBCI_MAX_ONNX_TENSOR_DIMS 8

/** Flat upper bound for the input tensor buffer. Override at build time. */
#ifndef TBCI_MAX_ONNX_INPUT_SIZE
#define TBCI_MAX_ONNX_INPUT_SIZE 4096
#endif

/** Flat upper bound for the output tensor buffer (max n_classes). Override at build time. */
#ifndef TBCI_MAX_ONNX_OUTPUT_SIZE
#define TBCI_MAX_ONNX_OUTPUT_SIZE 64
#endif

/** Maximum length of the ONNX model file path including null terminator. */
#define TBCI_ONNX_MAX_PATH_LEN 256

/* --------------------------------------------------------------------------
 * Configuration
 * -------------------------------------------------------------------------- */

/**
 * @brief Configuration for the ONNX model node.
 *
 * Caller provides all storage. Pointers must remain valid for the
 * lifetime of the TBCI_ONNXModel node.
 */
typedef struct {
    char      model_path[TBCI_ONNX_MAX_PATH_LEN];  /**< Path to the .onnx file.                        */
    float     temperature;                         /**< Softmax temperature. Higher = softer. Default 1.0f. */
    float    *train_trials;                        /**< Caller-provided [train_capacity * input_size]. */
    uint16_t *train_labels;                        /**< Caller-provided [train_capacity].              */
    size_t    train_capacity;                      /**< Max trials the caller allocated for.           */
    size_t    n_folds;                             /**< K for K-fold cross-validation in eval().       */
    char input_name[64];                           /**< default "input"                                */
    char output_name[64];                          /**< default "output"                               */
    TBCI_OutputMode output_mode;                   /**< Default TBCI_OUTPUT_SOFTMAX                    */
    TBCI_ScorerFn scorer;                          /**< Scoring function for eval(). Default: tbci_score_accuracy. */
    float     sigmoid_threshold;                   /**< Default 0.5f, sigmoid mode only                */
} TBCI_ONNXModelConfig;

/* --------------------------------------------------------------------------
 * Node struct
 * -------------------------------------------------------------------------- */

/**
 * @brief ONNX inference model node. Extends TBCI_Model via composition.
 *
 * base_model must be first member for safe casting to TBCI_Model* and TBCI_Node*.
 */
typedef struct {
    TBCI_Model           base_model;                          /**< Base model. MUST be first member.          */
    TBCI_ONNXModelConfig config;                              /**< Model configuration. Copied at init time.  */

    /* ORT session handles */
    const OrtApi        *ort;                                 /**< ORT API vtable. Owned by ORT.              */
    OrtEnv              *env;                                 /**< ORT environment. Owned by this node.       */
    OrtSession          *session;                             /**< ORT inference session. Owned by this node. */
    OrtMemoryInfo       *memory_info;                         /**< CPU memory info. Owned by this node.       */

    /* Tensor dims — read from model at init */
    int64_t input_dims[TBCI_MAX_ONNX_TENSOR_DIMS];
    size_t  n_input_dims;
    size_t               input_size;                          /**< Flat input length (product of input dims). */
    size_t               output_size;                         /**< Flat output length (n_classes).            */

    /* Fixed-size IO buffers */
    float                input_buf[TBCI_MAX_ONNX_INPUT_SIZE]; /**< Scratch buffer for one input tensor.      */
    float                output_buf[TBCI_MAX_ONNX_OUTPUT_SIZE];/**< Scratch buffer for one output tensor.    */

    /* Training state */
    size_t               train_count;                         /**< Trials accumulated so far.                 */
    int                  predicted_class;                     /**< 0-based predicted class, last infer.       */
} TBCI_ONNXModel;

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialise the ONNX model node.
 *
 * Opens the ORT session from config.model_path. Reads input_size and
 * output_size from the model. Validates that input_size <= TBCI_MAX_ONNX_INPUT_SIZE
 * and output_size <= TBCI_MAX_ONNX_OUTPUT_SIZE. Wires train/eval/infer and process_fn.
 *
 * @param[out] model   Pointer to an uninitialised model. Must not be NULL.
 * @param[in]  config  Model configuration. Must not be NULL.
 * @param[in]  ctx     Pipeline context. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL or model_path exceeds TBCI_ONNX_MAX_PATH_LEN.
 * @return TBCI_ERR_INVALID_STATE if input or output dims exceed build-time buffer sizes.
 */
TBCI_API TBCI_Status onnx_model_init(TBCI_ONNXModel *model,
                                      TBCI_ONNXModelConfig *config,
                                      struct TBCI_Context *ctx);

/**
 * @brief Accumulate one training trial.
 *
 * Copies epoch->samples (flattened) into config.train_trials and stores
 * epoch->label into config.train_labels. Increments train_count.
 * If train_count == train_capacity, logs a warning and skips silently.
 *
 * @param[in,out] self   Pointer to the model. Must not be NULL.
 * @param[in]     epoch  Feature epoch. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_WARNING_FULL if train_capacity has been reached (trial skipped).
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status onnx_model_train(TBCI_Model *self, TBCI_Epoch *epoch);

/**
 * @brief Evaluate model quality via K-fold cross-validation.
 *
 * Partitions accumulated trials into n_folds folds using index-based
 * splitting (trial i is in fold i % n_folds). For each fold, runs infer()
 * on validation trials and compares predicted class to stored label.
 * Writes mean accuracy across folds to score_out and epoch->score.
 *
 * Returns TBCI_ERR_INVALID_STATE if train_count < n_folds.
 *
 * @param[in,out] self          Pointer to the model. Must not be NULL.
 * @param[out]    score_out  Mean K-fold accuracy in [0, 1]. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_STATE if not enough trials have been accumulated.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status onnx_model_eval(TBCI_Model *self, float *score_out);

/**
 * @brief Run inference on one feature epoch.
 *
 * Copies epoch->samples into input_buf, runs the ORT session, applies
 * softmax to the output logits, writes class probabilities back into
 * epoch->samples in-place. Sets predicted_class to argmax of probabilities.
 *
 * @param[in,out] self   Pointer to the model. Must not be NULL.
 * @param[in,out] epoch  Feature epoch. samples overwritten with probabilities. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 * @return TBCI_ERR_INVALID_STATE if input_size does not match epoch->n_channels * epoch->n_frames.
 */
TBCI_API TBCI_Status onnx_model_infer(TBCI_Model *self, TBCI_Epoch *epoch);

/**
 * @brief Release ORT session, environment, and memory info.
 *
 * Called automatically by dc_reset. Does not free caller-provided buffers.
 *
 * @param[in,out] model  Pointer to an initialised model. Must not be NULL.
 * @return TBCI_OK on success.
 */
TBCI_API TBCI_Status onnx_model_close(TBCI_ONNXModel *model);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_WITH_ONNX */
#endif /* TBCI_ONNX_MODEL_H */