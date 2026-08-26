/**
 * @file tbci_notch_node.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Notch filter inner node for PreprocessingGroup.
 *
 * Implements a cascaded notch filter — one 2nd-order biquad section per
 * harmonic. E.g. notch at 50Hz with 2 harmonics → filters at 50Hz and 100Hz.
 * Coefficients are computed at init_fn time from (freq_hz, q_factor, srate).
 * Works at any sampling rate.
 *
 * ## Usage
 *
 * @code
 * TBCI_NotchConfig cfg = { .freq_hz = 50.0f, .q_factor = 30.0f, .n_harmonics = 2 };
 * TBCI_NotchNode   node;
 * notch_init(&node, &cfg);
 * group_add_node(&ctx.preprocessing.group, (TBCI_Node *)&node);
 * @endcode
 */

#ifndef TBCI_NOTCH_NODE_H
#define TBCI_NOTCH_NODE_H

#include "tbci_iir_filter_state.h"
#include "../tbci_node.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of notch harmonics (fundamental + harmonics). */
#ifndef TBCI_MAX_NOTCH_HARMONICS
#define TBCI_MAX_NOTCH_HARMONICS 2
#endif

/**
 * @brief Configuration for the notch filter node.
 *
 * Coefficients are derived automatically at init_fn time.
 * Higher Q = narrower notch. Typical EEG values: Q = 10-30.
 */
typedef struct {
    float   freq_hz;     /**< Fundamental notch frequency in Hz (e.g. 50.0 or 60.0). */
    float   q_factor;    /**< Quality factor — higher = narrower notch.               */
    size_t  n_harmonics; /**< Number of harmonics to notch (1 = fundamental only).    */
} TBCI_NotchConfig;

/**
 * @brief Notch filter node — cascaded biquad sections, one per harmonic.
 *
 * filters[0] notches at freq_hz, filters[1] at 2*freq_hz, etc.
 */
typedef struct {
    TBCI_Node          base;                              /**< Inner node base. MUST be first member.   */
    TBCI_NotchConfig   config;                            /**< Notch configuration. Copied at init.     */
    TBCI_IIRFilterState filters[TBCI_MAX_NOTCH_HARMONICS]; /**< One biquad per harmonic.               */
    size_t             n_filters;                         /**< Active number of filters.                */
} TBCI_NotchNode;

/**
 * @brief Initialise the notch node.
 *
 * Stores config and wires function pointers. Coefficient computation
 * happens in notch_init_fn when group_init is called with ctx.
 *
 * @param[out] node    Pointer to an uninitialised node. Must not be NULL.
 * @param[in]  config  Notch configuration. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL or n_harmonics == 0
 *         or n_harmonics > TBCI_MAX_NOTCH_HARMONICS.
 */
TBCI_API TBCI_Status notch_init(TBCI_NotchNode *node, TBCI_NotchConfig *config);

/**
 * @brief Process one frame of n_channels samples in-place.
 *
 * Applies each notch biquad in sequence to each channel independently.
 *
 * @param[in,out] node  Pointer to an initialised node. Must not be NULL.
 * @param[in,out] data  Float array of n_channels samples. Modified in-place.
 * @param[in]     ctx   Pipeline context. Must not be NULL.
 * @return TBCI_NODE_OK on success.
 * @return TBCI_NODE_ERROR if any pointer is NULL.
 */
TBCI_API TBCI_NodeResult notch_process(TBCI_NotchNode *node, void *data,
                                        struct TBCI_Context *ctx);

/**
 * @brief Reset all biquad sections to initial state.
 *
 * @param[in,out] node  Pointer to an initialised node. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if node is NULL.
 */
TBCI_API TBCI_Status notch_reset(TBCI_NotchNode *node);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_NOTCH_NODE_H */