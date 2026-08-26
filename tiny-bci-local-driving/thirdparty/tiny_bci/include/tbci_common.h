/**
 * @file tbci_common.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Shared types, status codes, and top-level BCI context for TinyBCI.
 *
 * This header defines the foundational types used across all TinyBCI components.
 * It should be included by every other TinyBCI header and translation unit.
 */

#ifndef TBCI_COMMON_H
#define TBCI_COMMON_H

#include "pch.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Version
 * -------------------------------------------------------------------------- */

#define TBCI_VERSION_MAJOR 0
#define TBCI_VERSION_MINOR 5
#define TBCI_VERSION_PATCH 0

/* --------------------------------------------------------------------------
 * API export macro
 * Handles DLL export on Windows and symbol visibility on GCC/Clang.
 * On bare-metal targets this expands to nothing.
 * -------------------------------------------------------------------------- */

#if defined(_WIN32) || defined(_WIN64)
#   if defined(TBCI_BUILD_SHARED)
#       define TBCI_API __declspec(dllexport)
#   elif defined(TBCI_USE_SHARED)
#       define TBCI_API __declspec(dllimport)
#   else
#       define TBCI_API
#   endif
#elif defined(__GNUC__) && defined(TBCI_BUILD_SHARED)
#   define TBCI_API __attribute__((visibility("default")))
#else
#   define TBCI_API
#endif

#ifndef TBCI_MAX_CHANNELS
#define TBCI_MAX_CHANNELS 64
#endif

/** Maximum number of output classes. Override at build time. */
#ifndef TBCI_MAX_CLASSES
#define TBCI_MAX_CLASSES 64
#endif

/* --------------------------------------------------------------------------
 * Macros
 * -------------------------------------------------------------------------- */
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

/* --------------------------------------------------------------------------
 * Status codes
 * Every TinyBCI function that can fail returns a tbci_status_t.
 * -------------------------------------------------------------------------- */

/**
 * @brief Return codes for all TinyBCI API functions.
 *
 * Functions return TBCI_OK on success. All other values indicate an error
 * or a notable condition. Callers should always check the return value.
 */
typedef enum {
    TBCI_OK                  =  0,          /**< Operation completed successfully. */
    TBCI_WARN_PARADIGM_MODE_MISMATCH = 1,   /**< Paradigm and segmentation mode may be inconsistent. */
    TBCI_WARN_FULL_TRIALS    = 1,           /**< Fine-Tuning / Calibration trials capacity reached. */
    TBCI_ERR_INVALID_ARG     = -1,          /**< A NULL or out-of-range argument was passed. */
    TBCI_ERR_FULL            = -2,          /**< Buffer or queue is full; data was rejected. */
    TBCI_ERR_EMPTY           = -3,          /**< Buffer or queue is empty; no data available. */
    TBCI_ERR_OVERFLOW        = -4,          /**< Buffer wrapped around; old data was overwritten. */
    TBCI_ERR_INVALID_STATE   = -5,          /**< Operation is not valid in the current BCI state. */
    TBCI_ERR_NOT_IMPLEMENTED = -6,          /**< Feature is not yet implemented. */
    TBCI_ERR_NOT_FOUND       = -7,          /**< Requested timestamp or item not found in buffer. */
    TBCI_ERR_NOT_YET         = -8,          /**< Target timestamp is ahead of buffered data. */
    TBCI_ERR_ALLOC_FAILED    = -9,          /**< Memory allocation failed. */
} TBCI_Status;

/**
 * @brief Result of a timestamp search in a signal buffer.
 */
typedef enum {
    TBCI_MATCH_EXACT,   /**< Frame timestamp matched exactly. */
    TBCI_MATCH_NEAREST, /**< No exact match; nearest frame strictly after target returned. */
} TBCI_MatchType;

/* --------------------------------------------------------------------------
 * BCI application state
 * -------------------------------------------------------------------------- */

/**
 * @brief Top-level operational state of the BCI pipeline.
 *
 * The state determines which pipeline stages are active and how data is
 * routed. For example, segmentation labels epochs during TBCI_STATE_TRAINING
 * but produces unlabelled epochs during TBCI_STATE_INFERENCE.
 */
typedef enum {
    TBCI_STATE_IDLE,       /**< Pipeline is inactive. No data is processed. */
    TBCI_STATE_TRAINING,   /**< Collecting labeled epochs to train a decoder. */
    TBCI_STATE_INFERENCE,  /**< Running live classification on incoming data. */
} TBCI_State;

/**
 * @brief Supported BCI paradigm types.
 *
 * Determines how the segmentation node interprets trigger codes and
 * carves epochs from the signal stream. New paradigms require adding
 * a value to this enum and updating the segmentation node accordingly.
 */
typedef enum {
    TBCI_PARADIGM_P300,  /**< P300 oddball paradigm. Target vs non-target triggers.     */
    TBCI_PARADIGM_SSVEP, /**< Steady-state visually evoked potential. Frequency coding. */
    TBCI_PARADIGM_MI,    /**< Motor imagery. Left/right hand or feet/rest coding.        */
} TBCI_Paradigm;

/* --------------------------------------------------------------------------
 * Epoch
 * -------------------------------------------------------------------------- */

/**
 * @brief A segmented signal epoch produced by the segmentation node.
 *
 * Represents a fixed-length window of multichannel signal data extracted
 * around a trigger event. The epoch is the primary unit of data consumed
 * by feature extraction and classification nodes.
 *
 * The @p samples pointer refers to the first sample of the epoch in the
 * epoch buffer's backing storage. The caller must ensure the backing
 * storage remains valid for the lifetime of this struct.
 *
 * Layout: samples[frame * n_channels + channel]
 */
typedef struct {
    float    *samples;          /**< Channel-major layout: samples[channel * n_frames + frame]. */
    size_t    n_frames;         /**< Number of time points in the epoch. */
    size_t    n_channels;       /**< Number of signal channels per frame. */
    uint64_t  timestamp_us;     /**< Timestamp of the trigger or first sample in microseconds. */
    uint16_t  label;            /**< Class label for this epoch, derived from the trigger code. */
    int16_t   predicted_label;  /**< Predicted label for this epoch, after model inference. */
    uint16_t  encoded_label;    /**< Class label encoded by the decoder module. */
    float     confidence;       /**< probability of predicted class after softmax, set by infer()  */
    float     eval_score;       /**< cross-validated score from last eval(), carried per-epoch  */
} TBCI_Epoch;

/* --------------------------------------------------------------------------
 * Node
 * -------------------------------------------------------------------------- */

/**
 * @brief Return value for all TinyBCI node process() functions.
 *
 * The DAG runner inspects this value after each node executes to decide
 * whether to continue downstream. TBCI_NODE_PENDING short-circuits the
 * chain for the current tick without treating the absence of output as
 * an error — this is the normal condition for segmentation waiting for
 * enough post-stimulus data to accumulate.
 */
typedef enum {
    TBCI_NODE_OK,      /**< Node produced output. Runner continues to next node.  */
    TBCI_NODE_PENDING, /**< No output this tick. Runner stops here and waits.     */
    TBCI_NODE_ERROR,   /**< Node encountered an error. Inspect status for cause.  */
} TBCI_NodeResult;

/**
 * @brief Windowing mode for the segmentation node.
 *
 * Determines how epochs are extracted from the continuous signal stream.
 * Moved to tbci_common.h to avoid circular dependency between
 * tbci_config.h and tbci_segmentation.h.
 */
typedef enum {
    SEG_MODE_TRIGGERED, /**< One trigger → one epoch. Used for P300.          */
    SEG_MODE_SLIDING,   /**< Continuous overlapping windows. Used for MI/SSVEP. */
} TBCI_SegmentationMode;


/**
 * @brief Configuration for the segmentation node.
 *
 * Window lengths are specified in milliseconds and converted to frames
 * at init time using target_srate from TBCIContext. Setting
 * pre_stimulus_ms to 0 disables baseline extraction — the epoch starts
 * exactly at the trigger timestamp.
 */
typedef struct {
    TBCI_SegmentationMode mode;     /**< Windowing mode. Drives epoch extraction strategy.     */
    uint32_t pre_stimulus_ms;       /**< Pre-stimulus window in ms. 0 = no baseline.  */
    uint32_t post_stimulus_ms;      /**< Post-stimulus window in ms. Must be > 0.      */
    uint32_t overlap_ms;            /** SEG_MODE_SLIDING only */
    uint16_t trial_end_code;        /** trigger code that ends a trial   */
    /* raw output logging */
    bool     log_enabled;
    bool     log_commands;
    bool     log_processed;         /**< true = log processed_signal, false = log raw inputs->signal */
    char     log_subject[64];
    char     log_session[32];
} TBCI_CoreConfig;

#ifdef __cplusplus
}
#endif

#endif /* TBCI_COMMON_H */