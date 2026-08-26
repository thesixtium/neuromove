#ifndef TBCI_SYNTHETIC_PRODUCER_H
#define TBCI_SYNTHETIC_PRODUCER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../include/ioutils/tbci_producer.h"
#include "../include/ioutils/tbci_input.h"
#include "tbci_trigger_generator.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */

typedef struct {
    size_t   n_channels;        /**< Number of synthetic electrodes. */
    float    srate;             /**< Sampling rate of the synthetic sine wave. */
    float    freq_hz;           /**< Frequency of the synthetic sine wave. */
    float    amplitude;         /**< Amplitude of the synthetic sine wave. */
    float    noise_amplitude;   /**< Amplitude of additive white noise. 0.0 = no noise. */

} SyntheticProducerConfig;

typedef struct {
    uint32_t sample_index;
    uint64_t timestamp_us;
    uint64_t last_trigger_us;
    uint32_t spacing_us;
    uint32_t trigger_interval_us;
    int64_t  last_trial_start_us;
    bool     trial_ended;
} SyntheticProducerState;

typedef struct {
    TBCI_Producer            base;         /**< Must be first member. */
    SyntheticProducerConfig *config;
    SyntheticProducerState   state;
    TBCI_TriggerGenerator   *trigger_gen;  /**< Optional. NULL = no fake triggers. */
} SyntheticProducer;

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */
/**
 * @brief Initialise the synthetic producer.
 *
 * Must be called before tick.
 *
 * @param[out] producer  Pointer to an uninitialised producer. Must not be NULL.
 * @param[in]  config    Caller-owned configuration. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 * @return TBCI_ERR_INVALID_STATE if serial port cannot be opened.
 */
TBCI_API TBCI_Status sp_init(SyntheticProducer *producer, SyntheticProducerConfig *config);

/**
 * @brief Produce a fake sample and push it into the pipeline.
 *
 * Non-blocking — returns TBCI_ERR_EMPTY if no data is available yet.
 * Validates packet start/stop bytes and discards invalid packets.
 *
 * @param[in,out] producer  Pointer to an initialised producer. Must not be NULL.
 * @param[in,out] inputs    Pipeline inputs to write into. Must not be NULL.
 * @param[in,out] ctx       Pipeline context. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_EMPTY if no full packet available yet.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 * @return TBCI_ERR_INVALID_STATE if packet validation fails.
 */
TBCI_API TBCI_Status sp_tick(SyntheticProducer  *producer, TBCI_Input *inputs, struct TBCI_Context *ctx);

/**
 * @brief Re-initializes the producer.
 *
 * @param[in,out] producer  Pointer to an initialised producer. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if producer is NULL.
 */
TBCI_API TBCI_Status sp_reset(SyntheticProducer *producer);

/**
* @brief Fakes the closure of a serial port.
*
* @param[in,out] producer  Pointer to an initialised producer. Must not be NULL.
* @return TBCI_OK on success.
* @return TBCI_ERR_INVALID_ARG if producer is NULL.
*/
TBCI_API TBCI_Status sp_close(SyntheticProducer *producer);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_SYNTHETIC_PRODUCER_H */