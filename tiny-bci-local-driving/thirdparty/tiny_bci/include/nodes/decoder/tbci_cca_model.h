/**
 * @file tbci_cca_model.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief CCA-based SSVEP classifier model for the TinyBCI decoder group.
 *
 * Trainless classifier — infer() applies z-score normalization, softmax
 * with temperature scaling, and argmax on the CCA correlation vector
 * produced by TBCI_CCANode.
 *
 * train() and eval() are no-ops — CCA requires no calibration.
 *
 * ## Usage
 *
 * @code
 * TBCI_CCAModelConfig cfg = { .temperature = 1.0f };
 * TBCI_CCAModel model;
 * cca_model_init(&model, &cfg);
 * group_add_node(&ctx.classifier.group, (TBCI_Node *)&model);
 * @endcode
 */

#ifndef TBCI_CCA_MODEL_H
#define TBCI_CCA_MODEL_H

#include "tbci_decoder.h"
#include "../../mathutils/tbci_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration for the CCA model.
 */
typedef struct {
    float temperature; /**< Softmax temperature. Higher = softer. Default 1.0f. */
    size_t  n_freqs;   /**< Number of active entries in freqs[].                */
} TBCI_CCAModelConfig;

/**
 * @brief CCA model node. Extends TBCI_Model via composition.
 *
 * base_model must be first member for safe casting to TBCI_Model* and TBCI_Node*.
 */
typedef struct {
    TBCI_Model       base_model;    /**< Base base_model. MUST be first member.            */
    TBCI_CCAModelConfig config; /**< Model configuration. Copied at init time. */
} TBCI_CCAModel;

/**
 * @brief Initialise the CCA model node.
 *
 * Wires train/eval/infer and process_fn. No ctx needed — trainless.
 *
 * @param[out] model   Pointer to an uninitialised model. Must not be NULL.
 * @param[in]  config  Model configuration. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status cca_model_init(TBCI_CCAModel *model, TBCI_CCAModelConfig *config);

/**
 * @brief No-op. CCA requires no training.
 */
TBCI_API TBCI_Status cca_model_train(TBCI_Model *self, TBCI_Epoch *epoch);

/**
 * @brief No-op. CCA has no training quality to evaluate.
 *
 * Sets accuracy_out to 1.0f — trainless classifiers are always "ready".
 */
TBCI_API TBCI_Status cca_model_eval(TBCI_Model *self, float *accuracy_out);

/**
 * @brief Classify a CCA feature epoch.
 *
 * Applies z-score normalization, softmax with temperature, argmax.
 * Writes class probabilities back into epoch->samples in-place.
 * Sets base_model->predicted_class to the 0-based winner index.
 *
 * @param[in,out] self   Pointer to the model. Must not be NULL.
 * @param[in,out] epoch  Feature epoch from CCANode. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status cca_model_infer(TBCI_Model *self, TBCI_Epoch *epoch);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_CCA_MODEL_H */