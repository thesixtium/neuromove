/**
 * @file tbci_bandpass_node.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Bandpass filter inner node for PreprocessingGroup.
 *
 * Implements a 2nd-order Butterworth bandpass as two cascaded biquad
 * sections: a highpass at low_hz followed by a lowpass at high_hz.
 * Coefficients are computed at init_fn time from (low_hz, high_hz, srate).
 * Works at any sampling rate.
 *
 * ## Usage
 *
 * @code
 * TBCI_BandpassConfig cfg = { .low_hz = 1.0f, .high_hz = 40.0f };
 * TBCI_BandpassNode   node;
 * bp_init(&node, &cfg);
 * group_add_node(&ctx.preprocessing.group, (TBCI_Node *)&node);
 * @endcode
 */

#ifndef TBCI_BANDPASS_NODE_H
#define TBCI_BANDPASS_NODE_H

#include "tbci_iir_filter_state.h"
#include "../../mathutils/tbci_math.h"
#include "../tbci_node.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TBCI_MAX_FILTER_STAGES
#define TBCI_MAX_FILTER_STAGES 4  /* max cascade stages per direction, 4 = 8th order */
#endif

/**
 * @brief Configuration for the bandpass filter node.
 *
 * Coefficients are derived automatically from these parameters at init_fn
 * time using the sampling rate from ctx->config.target_srate.
 */
typedef struct {
    float low_hz;       /**< High-pass cutoff frequency in Hz. Removes DC and slow drift. */
    float high_hz;      /**< Low-pass cutoff frequency in Hz. Removes high-frequency noise. */
    float  q_factors[TBCI_MAX_FILTER_STAGES];  /**< Q per stage. 0.0f = use default BUTTERWORTH_Q */
    size_t n_stages;    /**< Number of cascaded biquad sections. 1=2nd, 2=4th order.
                           Default 1 if 0 is passed. Max TBCI_MAX_FILTER_STAGES.   */
} TBCI_BandpassConfig;

/**
 * @brief Bandpass filter node — two cascaded biquad sections.
 *
 * hp filters low frequencies, lp filters high frequencies.
 * Cascading them passes only the band between low_hz and high_hz.
 */
typedef struct {
    TBCI_Node          base;    /**< Inner node base. MUST be first member. */
    TBCI_BandpassConfig config; /**< Bandpass configuration. Copied at init. */
    TBCI_IIRFilterState  hp[TBCI_MAX_FILTER_STAGES];     /**< Highpass biquad section. */
    TBCI_IIRFilterState  lp[TBCI_MAX_FILTER_STAGES];     /**< Lowpass biquad section.  */
    size_t              n_stages;                        /**< Active stages, set at init. */

} TBCI_BandpassNode;

/**
 * @brief Initialise the bandpass node.
 *
 * Stores config and wires function pointers. Coefficient computation
 * happens in bp_init_fn when group_init is called with ctx.
 *
 * @param[out] node    Pointer to an uninitialised node. Must not be NULL.
 * @param[in]  config  Bandpass configuration. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status bp_init(TBCI_BandpassNode *node, TBCI_BandpassConfig *config);

/**
* @brief Configure a TBCI_BandpassConfig with optimal Butterworth Q values
*        for the given number of stages.
*
* Sets q_factors to the correct pole placement for a true nth order
* Butterworth response. If n_stages is not in the lookup table,
* falls back to BUTTERWORTH_Q for all stages and logs a warning.
*
* @param[out] config    Must not be NULL.
* @param[in]  low_hz    Highpass cutoff frequency in Hz.
* @param[in]  high_hz   Lowpass cutoff frequency in Hz.
* @param[in]  n_stages  Number of biquad stages (1-4).
* @return TBCI_OK on success.
* @return TBCI_ERR_INVALID_ARG if config is NULL or n_stages > TBCI_MAX_FILTER_STAGES.
*/
TBCI_Status bp_configure(TBCI_BandpassConfig *config, float low_hz, float high_hz, size_t n_stages);

/**
 * @brief Process one frame of n_channels samples in-place.
 *
 * Applies highpass then lowpass biquad to each channel independently.
 *
 * @param[in,out] node  Pointer to an initialised node. Must not be NULL.
 * @param[in,out] data  Float array of n_channels samples. Modified in-place.
 * @param[in]     ctx   Pipeline context. Must not be NULL.
 * @return TBCI_NODE_OK on success.
 * @return TBCI_NODE_ERROR if any pointer is NULL.
 */
TBCI_API TBCI_NodeResult bp_process(TBCI_BandpassNode *node, void *data, struct TBCI_Context *ctx);

/**
 * @brief Reset both biquad sections to initial state.
 *
 * @param[in,out] node  Pointer to an initialised node. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if node is NULL.
 */
TBCI_API TBCI_Status bp_reset(TBCI_BandpassNode *node);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_BANDPASS_NODE_H */