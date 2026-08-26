/**
 * @file tbci_cca_node.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief CCA-based SSVEP feature extraction inner node.
 *
 * Computes Canonical Correlation Analysis between a channel-major epoch
 * and a set of sinusoidal reference signals at each target frequency.
 * Writes per-frequency canonical correlations into epoch->samples and
 * stores the best frequency index for downstream decoder consumption.
 *
 * Must be preceded by the transpose node in FeatureExtractionGroup —
 * expects epoch->samples in channel-major layout.
 *
 * Reference signals are caller-provided and computed externally from
 * ctx->total_frames, n_freqs, and n_harmonics at init time.
 *
 * ## Memory
 *
 * @code
 * size_t n_frames = ...; // pre_ms + post_ms / 1000 * srate
 * size_t ref_cap  = TBCI_CCA_MAX_FREQS * TBCI_CCA_MAX_HARMONICS * 2 * n_frames;
 * float  ref_buf[ref_cap];
 * TBCI_CCAConfig cca_config = { .freqs = {7.0f, 8.0f, ...}, .n_freqs = 6, .n_harmonics = 2 };
 * TBCI_CCANode   cca_node;
 * cca_init(&cca_node, &cca_config, ref_buf, ref_cap);
 * group_add_node(&ctx.features.group, (TBCI_Node*)&cca_node);
 * @endcode
 */

#ifndef TBCI_CCA_NODE_H
#define TBCI_CCA_NODE_H

#include "../../tbci_common.h"
#include "../../tbci_context.h"
#include "../tbci_node.h"
#include "../../mathutils/tbci_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of SSVEP target frequencies. Override at build time. */
#define TBCI_CCA_MAX_FREQS     6
/** Number of harmonics used for CCA reference signals. Override at build time. */
#define TBCI_CCA_MAX_HARMONICS 3
/** Small value added to the diagonal of Cxx and Cyy to prevent singular matrices. */
#define REGULARIZATION 1e-6f
/** Convergence threshold for power iteration — stops when L1 change between iterations drops below this. */
#define TOL 1e-6f
/** Maximum number of power iteration steps before forced exit. */
#define MAX_ITER 100

/**
 * @brief Configuration for the CCA inner node.
 *
 * Provided by the caller at cca_init time. Frequencies and harmonics
 * determine the reference signals computed into the caller-provided buffer.
 */
typedef struct {
    float   freqs[TBCI_CCA_MAX_FREQS]; /**< Target stimulus frequencies in Hz.          */
    size_t  n_freqs;                    /**< Number of active entries in freqs[].         */
    uint8_t n_harmonics;                /**< Number of harmonics per frequency.           */
} TBCI_CCAConfig;

/**
 * @brief CCA inner node state and configuration.
 *
 * Owns per-frequency canonical correlations and the index of the
 * best-matching frequency after each process call. The reference
 * signals buffer is caller-provided and not owned by this struct.
 */
/**
 * @brief CCA inner node state and configuration.
 *
 * Owns all intermediate matrix buffers required for CCA computation.
 * No dynamic allocation — all buffers are fixed-size arrays sized by
 * TBCI_MAX_CHANNELS and TBCI_CCA_MAX_HARMONICS at build time.
 *
 * The reference signals buffer is caller-provided and not owned by this struct.
 */
typedef struct {
    TBCI_Node       base;                              /**< Inner node base. MUST be first member.                                */
    TBCI_CCAConfig  config;                            /**< Copy of caller-provided CCA configuration.                           */
    float          *ref_signals;                       /**< Caller-provided reference signals buffer. Not owned.                  */
    size_t          ref_signals_capacity;              /**< Number of floats in ref_signals.                                     */
    float           correlations[TBCI_CCA_MAX_FREQS];  /**< Canonical correlation per frequency, updated each process call.       */
    int             best_freq_idx;                     /**< 0-based index of highest correlation frequency. -1 if not yet set.   */

    /* Intermediate matrix buffers — reused across ticks, owned by node */
    float Cxx    [TBCI_MAX_CHANNELS][TBCI_MAX_CHANNELS];                        /**< EEG auto-covariance matrix.                */
    float Cyy    [TBCI_CCA_MAX_HARMONICS * 2][TBCI_CCA_MAX_HARMONICS * 2];      /**< Reference signal auto-covariance matrix.   */
    float Cxy    [TBCI_MAX_CHANNELS][TBCI_CCA_MAX_HARMONICS * 2];               /**< EEG-reference cross-covariance matrix.     */
    float inv_Cxx[TBCI_MAX_CHANNELS][TBCI_MAX_CHANNELS];                        /**< Inverse of Cxx.                           */
    float inv_Cyy[TBCI_CCA_MAX_HARMONICS * 2][TBCI_CCA_MAX_HARMONICS * 2];      /**< Inverse of Cyy.                           */
    float Cxy_T  [TBCI_CCA_MAX_HARMONICS * 2][TBCI_MAX_CHANNELS];               /**< Transpose of Cxy.                         */
    float M      [TBCI_MAX_CHANNELS][TBCI_MAX_CHANNELS];                        /**< CCA objective matrix for eigendecomposition. */
    float aug    [TBCI_MAX_CHANNELS * TBCI_MAX_CHANNELS * 2];                   /**< Gauss-Jordan workspace for mat_inverse.   */
    float temp1  [TBCI_MAX_CHANNELS][TBCI_CCA_MAX_HARMONICS * 2];               /**< Intermediate product buffer, step 1.      */
    float temp2  [TBCI_MAX_CHANNELS][TBCI_CCA_MAX_HARMONICS * 2];               /**< Intermediate product buffer, step 2.      */
    float eigenvectors[TBCI_MAX_CHANNELS];                                      /**< Dominant eigenvector from power iteration. */
} TBCI_CCANode;

/**
 * @brief Initialise the CCA node.
 *
 * Stores config and ref_signals pointer. Reference signals are computed
 * by cca_init_fn when group_init is called, using ctx->total_frames
 * and ctx->config.target_srate.
 *
 * @param[out] node                  Pointer to an uninitialised node. Must not be NULL.
 * @param[inI_OK on success.
 * @return TBCI_ER]  config                CCA configuration. Must not be NULL.
 * @param[in]  ref_signals           Caller-provided float buffer for reference signals. Must not be NULL.
 * @param[in]  ref_signals_capacity  Number of floats in ref_signals. Must be >=
 *                                   n_freqs * n_harmonics * 2 * total_frames.
 * @return TBCR_INVALID_ARG if any pointer is NULL or n_freqs/n_harmonics are 0.
 */
TBCI_API TBCI_Status cca_init(TBCI_CCANode *node, TBCI_CCAConfig *config,
                               float *ref_signals, size_t ref_signals_capacity);

/**
 * @brief Compute CCA between epoch and reference signals for each target frequency.
 *
 * Expects data->samples in channel-major layout (transpose node must run first).
 * Writes canonical correlations into node->correlations[] and sets node->best_freq_idx.
 *
 * @param[in,out] node   Pointer to an initialised node. Must not be NULL.
 * @param[in,out] data  Channel-major epoch to process. Must not be NULL.
 * @param[in]     ctx    Pipeline context. Must not be NULL.
 * @return TBCI_NODE_OK on success.
 * @return TBCI_NODE_ERROR if any pointer is NULL or epoch dimensions mismatch config.
 */
TBCI_API TBCI_NodeResult cca_process(TBCI_CCANode *node, void *data, struct TBCI_Context *ctx);

/**
 * @brief Reset the CCA node to its initial state.
 *
 * Clears correlations and best_freq_idx. Reference signals and config are retained.
 *
 * @param[in,out] node  Pointer to an initialised node. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if node is NULL.
 */
TBCI_API TBCI_Status cca_reset(TBCI_CCANode *node);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_CCA_NODE_H */