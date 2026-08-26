/**
* @file tbci_math.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Shared math utilities for TinyBCI model nodes.
 *
 * Pure stateless functions operating on flat float arrays.
 * No dynamic allocation. Safe to call from any pipeline node.
 *
 * All functions operate in-place unless otherwise noted.
 */

#ifndef TBCI_MATH_H
#define TBCI_MATH_H

#include <stddef.h>
#include "../tbci_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TBCI_MATH_EPSILON 1e-6f
#define TBCI_M_PI 3.14159265358979323846264338327950288   /* pi             */

/**
 * @brief Apply softmax with temperature scaling in-place.
 *
 * Divides each element by temperature, subtracts max for numerical
 * stability, exponentiates, then normalizes so elements sum to 1.
 *
 * @param[in,out] x           Float array of length n. Modified in-place.
 * @param[in]     n           Number of elements.
 * @param[in]     temperature Scaling factor. Must be > 0. Higher = softer distribution.
 */
TBCI_API void tbci_softmax(float *x, size_t n, float temperature);

/**
 * @brief Z-score normalize a float array in-place.
 *
 * Subtracts mean and divides by standard deviation + epsilon.
 * No-op if n == 0.
 *
 * @param[in,out] x  Float array of length n. Modified in-place.
 * @param[in]     n  Number of elements.
 */
TBCI_API void tbci_normalize_zscore(float *x, size_t n);

/**
 * @brief Min-max normalize a float array to [0, 1] in-place.
 *
 * No-op if range is zero.
 *
 * @param[in,out] x  Float array of length n. Modified in-place.
 * @param[in]     n  Number of elements.
 */
TBCI_API void tbci_normalize_minmax(float *x, size_t n);

/**
 * @brief Return the 0-based index of the maximum element.
 *
 * Returns -1 if n == 0 or x is NULL.
 *
 * @param[in] x  Float array of length n.
 * @param[in] n  Number of elements.
 * @return 0-based index of the maximum element, or -1 on error.
 */
TBCI_API int tbci_argmax(const float *x, size_t n);

/**
 * @brief Compute classification accuracy from a confusion matrix.
 *
 * Accuracy = correct predictions / total predictions.
 * Equivalent to micro-averaged F1 for balanced classes.
 *
 * @param[in] confusion  Square confusion matrix [true_label][predicted].
 *                       confusion[i][j] = number of samples with true label i
 *                       predicted as class j.
 * @param[in] n_classes  Number of classes. Must be <= TBCI_MAX_CLASSES.
 * @return Accuracy in [0, 1]. Returns 0.0f if confusion matrix is empty.
 */
TBCI_API float tbci_score_accuracy(const size_t confusion[][TBCI_MAX_CLASSES], size_t n_classes);

/**
 * @brief Compute macro-averaged F1 score from a confusion matrix.
 *
 * Computes per-class F1 = 2 * precision * recall / (precision + recall),
 * then averages uniformly across all classes (macro average).
 * Classes with no true or predicted samples contribute 0.0 to the average.
 *
 * Suitable for balanced and imbalanced multiclass problems.
 * For binary classification, equivalent to the standard F1 score.
 *
 * @param[in] confusion  Square confusion matrix [true_label][predicted].
 * @param[in] n_classes  Number of classes. Must be <= TBCI_MAX_CLASSES.
 * @return Macro-averaged F1 in [0, 1]. Returns 0.0f if confusion matrix is empty.
 */
TBCI_API float tbci_score_f1(const size_t confusion[][TBCI_MAX_CLASSES], size_t n_classes);

/**
 * @brief Compute Matthews Correlation Coefficient (MCC) from a confusion matrix.
 *
 * MCC is a balanced metric that accounts for all four confusion matrix cells.
 * Particularly robust for imbalanced binary classification (e.g. P300 target vs
 * non-target). Returns values in [-1, 1]: 1.0 = perfect, 0.0 = random, -1.0 = inverse.
 *
 * For multiclass problems, uses the generalized formula:
 *   MCC = (N * sum_diag - dot(pk, tk)) /
 *         sqrt((N^2 - dot(pk, pk)) * (N^2 - dot(tk, tk)))
 * where pk = predicted class totals, tk = true class totals, N = total samples.
 * This is an approximation of the exact multiclass MCC — exact for binary.
 *
 * @param[in] confusion  Square confusion matrix [true_label][predicted].
 * @param[in] n_classes  Number of classes. Must be <= TBCI_MAX_CLASSES.
 * @return MCC in [-1, 1]. Returns 0.0f if denominator is zero (degenerate case).
 */
TBCI_API float tbci_score_mcc(const size_t confusion[][TBCI_MAX_CLASSES], size_t n_classes);

/**
 * @brief Scorer function pointer type for TBCI_ONNXModel evaluation.
 *
 * Caller-provided scoring function used by onnx_model_eval() to compute
 * a quality metric from the K-fold confusion matrix. Built-in implementations:
 * tbci_score_accuracy, tbci_score_f1, tbci_score_mcc.
 *
 * Custom scorers can be provided via TBCI_ONNXModelConfig.scorer.
 * If NULL, onnx_model_eval() defaults to tbci_score_accuracy.
 *
 * @param[in] confusion  Square confusion matrix accumulated over all folds.
 * @param[in] n_classes  Number of classes.
 * @return Quality metric, conventionally in [0, 1]. Higher is better.
 */
typedef float (*TBCI_ScorerFn)(const size_t confusion[][TBCI_MAX_CLASSES], size_t n_classes);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_MATH_H */