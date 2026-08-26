/**
 * @file tbci_producer.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Abstract producer interface for TinyBCI.
 *
 * A producer is any data source that pushes EEG samples and triggers
 * into the pipeline via TBCI_Input. Concrete producers (synthetic,
 * Unicorn, LSL, file) extend TBCI_Producer via composition —
 * TBCI_Producer must be the first member of every concrete producer struct.
 *
 * ## Usage
 *
 * @code
 * SyntheticProducer sp = { .base.type = TBCI_PRODUCER_SYNTHETIC, ... };
 * TBCI_Producer *producer = (TBCI_Producer *)&sp;
 *
 * producer->init(producer, &inputs, &ctx);
 *
 * while (running) {
 *     producer->tick(producer, &inputs, &ctx);
 *     tbci_context_tick(&ctx);
 * }
 *
 * producer->close(producer);
 * @endcode
 */

#ifndef TBCI_PRODUCER_H
#define TBCI_PRODUCER_H

#include "../tbci_common.h"
#include "tbci_input.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TBCI_Context;

/**
 * @brief Abstract producer base.
 *
 * Must be the first member of every concrete producer struct to enable
 * safe casting between TBCI_Producer* and the concrete type.
 */
typedef struct TBCI_Producer {
    const char       *name;       /**< Human-readable name for logging.      */
    bool              connected;  /**< True if producer is ready to tick.    */

    /**
     * @brief Initialise the producer and connect to the data source.
     *
     * @param[in,out] producer  Pointer to concrete producer. Must not be NULL.
     * @param[in,out] inputs    Pipeline inputs. Must not be NULL.
     * @param[in,out] ctx       Pipeline context. Must not be NULL.
     * @return TBCI_OK on success.
     * @return TBCI_ERR_INVALID_STATE if connection fails.
     */
    TBCI_Status (*init)(struct TBCI_Producer *producer, TBCI_Input *inputs, struct TBCI_Context *ctx);

    /**
     * @brief Push one sample tick into the pipeline.
     *
     * @param[in,out] producer  Pointer to concrete producer. Must not be NULL.
     * @param[in,out] inputs    Pipeline inputs. Must not be NULL.
     * @param[in,out] ctx       Pipeline context. Must not be NULL.
     * @return TBCI_OK on success.
     * @return TBCI_ERR_EMPTY if no data available yet.
     */
    TBCI_Status (*tick)(struct TBCI_Producer    *producer,
                        TBCI_Input              *inputs,
                        struct TBCI_Context     *ctx);

    /**
     * @brief Reset the producer to its initial state.
     *
     * @param[in,out] producer  Pointer to concrete producer. Must not be NULL.
     * @return TBCI_OK on success.
     */
    TBCI_Status (*reset)(struct TBCI_Producer *producer);

    /**
     * @brief Close the connection and release resources.
     *
     * @param[in,out] producer  Pointer to concrete producer. Must not be NULL.
     * @return TBCI_OK on success.
     */
    TBCI_Status (*close)(struct TBCI_Producer *producer);

} TBCI_Producer;

#ifdef __cplusplus
}
#endif

#endif /* TBCI_PRODUCER_H */