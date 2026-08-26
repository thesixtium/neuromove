/**
 * @file lsl_producer.h
 *
 * @brief LSL EEG and trigger producer for TinyBCI pipeline.
 *
 * Reads from two LSL streams (EEG data + markers) and feeds them into
 * TBCI_Input via lsl_push_to_signal_buffer and lsl_push_to_trigger_queue.
 *
 * ## Usage
 *
 * @code
 * LSLProducerConfig cfg = {
 *     .data_stream_name   = "DSI7",
 *     .marker_stream_name = "MyMarkerStream",
 *     .max_channels       = 64,
 * };
 *
 * float scratch[64];
 * LSLProducerState state;
 * LSLProducer producer;
 *
 * if (lp_init(&producer, &cfg, &state, scratch) != TBCI_OK) {
 *     // handle connection failure
 * }
 *
 * while (running) {
 *     lp_tick(&producer, &inputs, &ctx);
 *     tbci_context_tick(&ctx);
 * }
 * @endcode
 */

#ifndef TBCI_LSL_PRODUCER_H
#define TBCI_LSL_PRODUCER_H

#ifdef TBCI_WITH_LSL

#include "ioutils/tbci_input.h"
#include "tbci_context.h"
#include "../include/ioutils/tbci_lsl_reader.h"
#include "../include/ioutils/tbci_producer.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef LSL_RECONNECT_INTERVAL_TICKS
#define LSL_RECONNECT_INTERVAL_TICKS 250  /* ~1s at 250Hz*/
#endif

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */

/**
 * @brief Configuration for the LSL producer.
 */

typedef enum {
    LSL_MODE_EEG_AND_MARKERS,  /* both streams */
    LSL_MODE_EEG_ONLY,         /* no marker stream */
    LSL_MODE_MARKERS_ONLY,     /* no EEG stream */
} LSLMode;

typedef struct {
    const char *data_stream;         /**< Name|Type of the EEG LSL stream to resolve.    */
    const char *marker_stream;       /**< Name|Type of the marker LSL stream to resolve. */
    int         max_channels;        /**< Capacity of the sample scratch buffer.    */
    float      *temp_buf;            /**< caller-allocated, at least max_channels floats */
    LSLResolveMode resolve_mode;     /**< Resolving streams by type or name */
    LSLMode     mode;                /**< Input modality.    */
} LSLProducerConfig;

/**
 * @brief Internal runtime state of the LSL producer.
 *
 * Zero-initialise before passing to lp_init.
 */
typedef struct {
    TBCI_LSLContext lsl_ctx;                    /**< LSL stream context (inlets, flags, etc).   */
    uint32_t reconnect_ticks;                   /**< counts ticks since last failed reconnect */
} LSLProducerState;

/**
 * @brief LSL producer handle.
 */
typedef struct {
    TBCI_Producer base;         /**< Must be first member. */
    LSLProducerConfig *config;  /**< Caller-owned configuration. Must not be NULL. */
    LSLProducerState   state;   /**< Caller-owned runtime state. Must not be NULL. */
} LSLProducer;

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialise the LSL producer and connect to both LSL streams.
 *
 * @param[out] producer     Pointer to an uninitialised producer. Must not be NULL.
 * @param[in]  config       Caller-owned configuration. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 * @return TBCI_ERR_INVALID_STATE if LSL streams could not be resolved.
 */
TBCI_API TBCI_Status lp_init(LSLProducer *producer, LSLProducerConfig *config);

/**
 * @brief Poll LSL streams and push any new data into the pipeline.
 *
 * Calls lsl_update, then pushes new EEG samples and markers into
 * inputs->signal and inputs->triggers respectively.
 * No-ops cleanly if no new data is available this tick.
 *
 * @param[in,out] producer  Pointer to an initialised producer. Must not be NULL.
 * @param[in,out] inputs    Pipeline inputs to write into. Must not be NULL.
 * @param[in,out] ctx       Pipeline context for trigger dispatch. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status lp_tick(LSLProducer *producer, TBCI_Input *inputs, struct TBCI_Context *ctx);

/**
 * @brief Re-initializes the producer.
 *
 * @param[in,out] producer  Pointer to an initialised producer. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if producer is NULL.
 */
TBCI_API TBCI_Status lp_reset(LSLProducer *producer);

/**
 * @brief Close LSL inlets and release stream resources.
 *
 * @param[in,out] producer  Pointer to an initialised producer. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if producer is NULL.
 */
TBCI_API TBCI_Status lp_close(LSLProducer *producer);

TBCI_API float   lp_get_srate(const LSLProducer *producer);

TBCI_API int     lp_get_n_channels(const LSLProducer *producer);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_WITH_LSL */

#endif /* TBCI_LSL_PRODUCER_H */