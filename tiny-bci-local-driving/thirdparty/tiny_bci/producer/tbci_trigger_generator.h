/**
 * @file tbci_trigger_generator.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Synthetic trigger generator — fires timed triggers into the pipeline.
 *
 * TBCI_TriggerGenerator is an optional component that can be attached to any
 * producer to generate fake triggers at a fixed interval. Useful for testing
 * the pipeline with real EEG hardware (e.g. Unicorn) without an external
 * trigger source.
 *
 * ## Usage
 *
 * @code
 * TBCI_TriggerGeneratorConfig tg_config = {
 *     .trigger_interval_ms = 1000u,
 *     .trigger_code        = 1u,
 *     .trial_end_code      = 10u,
 *     .trial_duration_ms   = 3000u,
 * };
 * TBCI_TriggerGeneratorState tg_state = {0};
 * TBCI_TriggerGenerator      tg;
 *
 * tg_init(&tg, &tg_config, &tg_state);
 *
 * // attach to any producer
 * unicorn_producer.trigger_gen = &tg;
 *
 * // call each tick with current producer timestamp
 * tg_tick(&tg, &inputs, &ctx, timestamp_us);
 * @endcode
 */

#ifndef TBCI_TRIGGER_GENERATOR_H
#define TBCI_TRIGGER_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "tbci_common.h"
#include "../include/ioutils/tbci_input.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TBCI_Context;

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */

/**
 * @brief Configuration for the trigger generator.
 *
 * Set trial_end_code = 0 and trial_duration_ms = 0 for triggered mode —
 * only start triggers will be fired. Set both for sliding mode.
 */
typedef struct {
    uint32_t trigger_interval_ms; /**< Interval between start triggers in ms.  */
    uint16_t trigger_code;        /**< Start trigger code. Default 1u.         */
    uint16_t trial_end_code;      /**< End trigger code. 0 = disabled.         */
    uint32_t trial_duration_ms;   /**< Trial duration in ms. 0 = disabled.     */
} TBCI_TriggerGeneratorConfig;

/**
 * @brief Internal runtime state of the trigger generator.
 *
 * Zero-initialise before passing to tg_init.
 */
typedef struct {
    uint64_t last_trigger_us;     /**< Timestamp of last start trigger.        */
    int64_t  last_trial_start_us; /**< Timestamp of last trial start. -1=none. */
    uint32_t trigger_interval_us; /**< Computed at init from trigger_interval_ms. */
    uint64_t trial_duration_us;   /**< Computed at init from trial_duration_ms.   */
    bool     trial_ended;         /**< True if end trigger already fired.      */
} TBCI_TriggerGeneratorState;

/**
 * @brief Trigger generator handle.
 */
typedef struct {
    TBCI_TriggerGeneratorConfig *config; /**< Caller-owned config. Must not be NULL. */
    TBCI_TriggerGeneratorState  *state;  /**< Caller-owned state. Must not be NULL.  */
} TBCI_TriggerGenerator;

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialise the trigger generator.
 *
 * Computes timing intervals from config. Zero-initialises state.
 *
 * @param[out] gen     Pointer to an uninitialised generator. Must not be NULL.
 * @param[in]  config  Caller-owned configuration. Must not be NULL.
 * @param[in]  state   Caller-owned state, zero-initialised. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status tg_init(TBCI_TriggerGenerator       *gen,
                              TBCI_TriggerGeneratorConfig *config,
                              TBCI_TriggerGeneratorState  *state);

/**
 * @brief Fire triggers into the pipeline if due.
 *
 * Called each producer tick with the current producer timestamp.
 * Fires a start trigger if trigger_interval_us has elapsed.
 * Fires an end trigger if trial_duration_us has elapsed since trial start.
 *
 * @param[in,out] gen          Pointer to an initialised generator. Must not be NULL.
 * @param[in,out] inputs       Pipeline inputs. Must not be NULL.
 * @param[in,out] ctx          Pipeline context. Must not be NULL.
 * @param[in]     timestamp_us Current producer timestamp in microseconds.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status tg_tick(TBCI_TriggerGenerator *gen,
                              TBCI_Input            *inputs,
                              struct TBCI_Context   *ctx,
                              uint64_t               timestamp_us);

/**
 * @brief Reset the trigger generator to its initial state.
 *
 * Clears timing state. Config is preserved.
 *
 * @param[in,out] gen  Pointer to an initialised generator. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if gen is NULL.
 */
TBCI_API TBCI_Status tg_reset(TBCI_TriggerGenerator *gen);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_TRIGGER_GENERATOR_H */