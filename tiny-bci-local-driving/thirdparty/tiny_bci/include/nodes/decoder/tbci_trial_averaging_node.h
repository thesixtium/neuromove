/**
 * @file tbci_trial_averaging_node.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Trial averaging inner node for the TinyBCI decoder group.
 *
 * Accumulates class probabilities across N repetitions of M stimuli,
 * averages them, and outputs a single epoch with the winning class set
 * as predicted_label. Paradigm-agnostic — works for P300, MI, SSVEP.
 *
 * Returns TBCI_NODE_PENDING until enough epochs are accumulated.
 * Resets automatically after output.
 *
 * ## Early stopping
 *
 * If enabled, checks confidence and margin after each completed repetition.
 * Outputs early if confidence >= min_confidence AND margin >= min_margin.
 *
 * ## Usage
 *
 * @code
 * TBCI_TrialAveragingConfig cfg = {
 *     .n_reps          = 5,
 *     .n_targets       = 6,
 *     .min_confidence  = 0.4f,
 *     .min_margin      = 0.2f,
 *     .early_stopping  = true,
 * };
 * TBCI_TrialAveragingNode ta_node;
 * ta_init(&ta_node, &cfg);
 * group_add_node(&ctx.decoder.group, (TBCI_Node *)&ta_node);
 * @endcode
 */

#ifndef TBCI_TRIAL_AVERAGING_NODE_H
#define TBCI_TRIAL_AVERAGING_NODE_H

#include "../tbci_node.h"
#include "../../tbci_common.h"
#include "../../mathutils/tbci_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Build-time defines
 * -------------------------------------------------------------------------- */

/** Maximum number of repetitions. Override at build time. */
#ifndef TBCI_MAX_TRIAL_REPS
#define TBCI_MAX_TRIAL_REPS 5
#endif

/* --------------------------------------------------------------------------
 * Configuration
 * -------------------------------------------------------------------------- */

typedef struct {
    size_t n_reps;          /**< Total epochs to accumulate before output. */
    float  min_confidence;  /**< Early stopping: min winner probability.   */
    float  min_margin;      /**< Early stopping: min gap between top two.  */
    bool   early_stopping;  /**< Enable early stopping.                    */
} TBCI_TrialAveragingConfig;

typedef struct {
    TBCI_Node               base;
    TBCI_TrialAveragingConfig config;
    float  acc[TBCI_MAX_CLASSES];       /**< Running sum per class.         */
    float  avg_probs[TBCI_MAX_CLASSES]; /**< Averaged output probabilities. */
    size_t current_epochs;              /**< Epochs accumulated so far.     */
} TBCI_TrialAveragingNode;

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialise the trial averaging node.
 *
 * @param[out] node    Must not be NULL.
 * @param[in]  config  Must not be NULL. n_reps <= TBCI_MAX_TRIAL_REPS,
 *                     n_targets <= TBCI_MAX_CLASSES.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL or config values out of range.
 */
TBCI_API TBCI_Status ta_init(TBCI_TrialAveragingNode *node, TBCI_TrialAveragingConfig *config);

/**
 * @brief Process one epoch.
 *
 * Stores epoch->samples[0] (model output probability for predicted class)
 * into probs[current_epochs][current_target_count]. Increments counters.
 * Returns TBCI_NODE_PENDING until n_reps * n_reps epochs are accumulated
 * (or early stopping fires). On completion, writes averaged probabilities
 * into epoch->samples, sets epoch->predicted_label to argmax, resets state.
 *
 * @param[in,out] node   Must not be NULL.
 * @param[in,out] epoch  Must not be NULL.
 * @param[in]     ctx    Pipeline context. Must not be NULL.
 * @return TBCI_NODE_OK on completion.
 * @return TBCI_NODE_PENDING if still accumulating.
 * @return TBCI_NODE_ERROR on failure.
 */
TBCI_API TBCI_NodeResult ta_process(TBCI_TrialAveragingNode *node, TBCI_Epoch *epoch, struct TBCI_Context *ctx);

/**
 * @brief Reset accumulator state.
 *
 * Zeroes probs, avg_probs, current_epochs, current_target_count.
 * Does not touch config.
 *
 * @param[in,out] node  Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if node is NULL.
 */
TBCI_API TBCI_Status ta_reset(TBCI_TrialAveragingNode *node);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_TRIAL_AVERAGING_NODE_H */