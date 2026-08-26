/**
 * @file tbci_iir_filter_state.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Single biquad IIR filter inner state for PreprocessingGroup.
 *
 * Implements a Direct Form II transposed biquad section:
 *
 *   y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2]
 *                  - a1*y[n-1] - a2*y[n-2]
 *
 * Coefficients are caller-provided at init time — use TBCI_BandpassNode
 * or TBCI_NotchNode which compute coefficients automatically from
 * cutoff frequency and sampling rate.
 *
 * Operates in-place on a float array of n_channels samples.
 * Each channel maintains independent filter state.
 *
 * ## Memory
 *
 * Filter state is owned by the node struct — no malloc, no statics.
 * State is sized by TBCI_MAX_CHANNELS at build time.
 *
 * ## Usage
 *
 * @code
 * TBCI_IIRFilterConfig cfg = {
 *     .b = {b0, b1, b2},
 *     .a = {1.0f, a1, a2},
 * };
 * TBCI_IIRFilterState state;
 * iir_init(&state, &cfg);
 * @endcode
 */

#ifndef TBCI_IIR_FILTER_STATE_H
#define TBCI_IIR_FILTER_STATE_H

#include "tbci_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TBCI_Context;


#define TBCI_BIQUAD_ORDER 2     /** Maximum biquad order — fixed at 2 for all biquad sections. */
#define TBCI_NOTCH_Q_MIN 0.1f   /**< Minimum Q factor to avoid division by zero. */
#define BUTTERWORTH_Q 0.707f    /**< maximally flat filter response */

/**
 * @brief Coefficients for one biquad IIR section.
 *
 * b[0..2] are feedforward (FIR) coefficients.
 * a[0..2] are feedback coefficients. a[0] is always 1.0 (normalized).
 * w[0..1] are initial state conditions per channel (optional, zero if unused).
 */
typedef struct {
    float b[TBCI_BIQUAD_ORDER + 1];  /**< Feedforward coefficients [b0, b1, b2]. */
    float a[TBCI_BIQUAD_ORDER + 1];  /**< Feedback coefficients [1.0, a1, a2].   */
    float zi[TBCI_BIQUAD_ORDER];     /**< Initial state per unit input.           */
} TBCI_IIRFilterConfig;

/**
 * @brief Single biquad IIR filter inner state.
 *
 * Owns per-channel filter state. State is initialized from zi_coeffs
 * scaled by the first input sample (standard filtfilt-style init).
 */
typedef struct {
    TBCI_IIRFilterConfig config;                          /**< Filter coefficients. Copied at init.            */
    float              w[TBCI_MAX_CHANNELS][TBCI_BIQUAD_ORDER]; /**< Per-channel biquad state.                */
    bool               initialized[TBCI_MAX_CHANNELS];   /**< True after first sample processed per channel.  */
} TBCI_IIRFilterState;

/**
 * @brief Initialise the IIR filter state with caller-provided coefficients.
 *
 * Does not require ctx — coefficients are fully specified by config.
 * Wires iir_process into base.process_fn.
 *
 * @param[out] state    Pointer to an uninitialised state. Must not be NULL.
 * @param[in]  config  Filter coefficients. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status iir_init(TBCI_IIRFilterState *state, TBCI_IIRFilterConfig *config);

/**
 * @brief Process one frame of n_channels samples in-place.
 *
 * Applies the biquad filter independently to each channel.
 * On the first sample per channel, initialises state from zi_coeffs
 * scaled by the input to avoid transient startup artifacts.
 *
 * @param[in,out] state   Pointer to an initialised state. Must not be NULL.
 * @param[in,out] data   Float array of n_channels samples. Modified in-place.
 * @param[in]     ctx    Pipeline context, used for n_channels. Must not be NULL.
 * @return TBCI_NODE_OK on success.
 * @return TBCI_NODE_ERROR if any pointer is NULL.
 */
TBCI_API TBCI_NodeResult iir_process(TBCI_IIRFilterState *state, void *data, struct TBCI_Context *ctx);

/**
 * @brief Reset per-channel filter state to zero.
 *
 * Call between trials or on pipeline reset to avoid state bleeding
 * across epochs.
 *
 * @param[in,out] state  Pointer to an initialised state. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if state is NULL.
 */
TBCI_API TBCI_Status iir_reset(TBCI_IIRFilterState *state);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_IIR_FILTER_STATE_H */