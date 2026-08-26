/* --------------------------------------------------------------------------
* Factory functions
 * -------------------------------------------------------------------------- */
#ifndef TBCI_PRODUCER_FACTORY_H
#define TBCI_PRODUCER_FACTORY_H

#include "tbci_common.h"
#include "../include/ioutils/tbci_input.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/ioutils/tbci_producer.h"
#include "synthetic_producer.h"
#include "unicorn_producer.h"
#include "neuropawn_producer.h"

#ifdef TBCI_WITH_LSL
#include "lsl_producer.h"
#endif
/**
 * @brief Create and initialise a synthetic producer.
 *
 * Wires up function pointers, initialises state, and connects the producer.
 * After this call use only producer->tick, producer->reset, producer->close.
 *
 * @param[out] producer  Pointer to a SyntheticProducer cast to TBCI_Producer*.
 * @param[in]  config    Caller-owned configuration. Must not be NULL.
 * @param[in,out] inputs Pipeline inputs. Must not be NULL.
 * @param[in,out] ctx    Pipeline context. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status tbci_producer_create_synthetic(TBCI_Producer           *producer,
                                                     SyntheticProducerConfig *config,
                                                     TBCI_Input              *inputs,
                                                     struct TBCI_Context     *ctx);
#ifdef TBCI_WITH_LSL
TBCI_API TBCI_Status tbci_producer_create_lsl(TBCI_Producer       *producer,
                                               LSLProducerConfig   *config,
                                               TBCI_Input          *inputs,
                                               struct TBCI_Context *ctx);
#endif

/**
 * @brief Create and initialise a Unicorn serial producer.
 */
TBCI_API TBCI_Status tbci_producer_create_unicorn(TBCI_Producer         *producer,
                                                   UnicornProducerConfig *config,
                                                   TBCI_Input            *inputs,
                                                   struct TBCI_Context   *ctx);

/**
 * @brief Create and initialise a NeuroPawn Knight serial producer.
 */
TBCI_API TBCI_Status tbci_producer_create_neuropawn(TBCI_Producer           *producer,
                                                     NeuroPawnProducerConfig *config,
                                                     TBCI_Input              *inputs,
                                                     struct TBCI_Context     *ctx);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_PRODUCER_FACTORY_H */