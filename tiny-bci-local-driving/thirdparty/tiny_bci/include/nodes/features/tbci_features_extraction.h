/**
* @file tbci_features_extraction.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief A group of nodes dedicated to extract features from the signal in the TinyBCI pipeline.
 *
 * Each node encapsulates one feature extraction stage. The group has visibility on the shared
 * Context, from which reads from the EpochQueue and finally writes in the FeatureQueue buffer.
 *
 * Inner nodes feeds their output to the following nodes and there is no visibility
 * on the shared Context. Data type is shared.
 *

 */

#ifndef TBCI_FEATURES_EXTRACTION_H
#define TBCI_FEATURES_EXTRACTION_H

#include "../../tbci_common.h"
#include "../tbci_node_group.h"

#ifdef __cplusplus
extern "C" {

#endif

typedef struct
{
    TBCI_NodeGroup group; /**< External macro node with visibility on the context, MUST be first member of the struct */
    bool extraction_enabled; /**< Whether data pass-through the inner nodes is enabled */
} TBCI_FeatureExtraction;

/**
 * @brief Initialise the feature extraction group.
 *
 * @param[out] node    Pointer to an uninitialised group. Must not be NULL.
 * @param[in]  enabled  Wether the node should be enabled or skipped through.
 * @param[in]  ctx     Pipeline context. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status fe_init(TBCI_FeatureExtraction *node, bool enabled, struct TBCI_Context *ctx);

/**
 * @brief Run one tick of the feature extraction group.
 *
 * If disabled or no internal nodes registered, copies the newest frame
 * from ctx->signal_buf to ctx->processed_buf unchanged.
 *
 * @param[in,out] node  Pointer to an initialised group. Must not be NULL.
 * @param[in,out] ctx   Pipeline context. Must not be NULL.
 * @return TBCI_NODE_OK on success.
 * @return TBCI_NODE_ERROR if node or ctx is NULL.
 */
TBCI_API TBCI_NodeResult fe_process(TBCI_FeatureExtraction *node, struct TBCI_Context *ctx);

/**
 * @brief Reset the feature extraction group to its initial state.
 *
 * @param[in,out] node  Pointer to an initialised group. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if node is NULL.
 */
TBCI_API TBCI_Status fe_reset(TBCI_FeatureExtraction *node);

/**
 * @brief Transpose epoch samples from time-major to channel-major layout.
 *
 * Reads src_epoch->samples (time-major: samples[frame * n_channels + channel])
 * and writes channel-major (samples[channel * n_frames + frame]) into dst.
 * src_epoch->samples and dst must not overlap.
 *
 * @param[in]  src_epoch   Source epoch with time-major samples. Must not be NULL.
 * @param[out] dst         Destination buffer, must hold n_channels * n_frames floats. Must not be NULL.
 * @param[in]  n_channels  Number of signal channels.
 * @param[in]  n_frames    Number of time frames.
 */
TBCI_API void fe_transpose_epoch(const TBCI_Epoch *src_epoch, float *dst, size_t n_channels, size_t n_frames);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_FEATURES_EXTRACTION_H */
