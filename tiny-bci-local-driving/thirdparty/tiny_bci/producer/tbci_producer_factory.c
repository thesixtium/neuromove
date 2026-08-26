/**
* @file tbci_producer_factory.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Wire each producer implementation to the generic interface.
 */

#include "tbci_producer_factory.h"

TBCI_Status tbci_producer_create_synthetic(TBCI_Producer *producer,
                                            SyntheticProducerConfig *config,
                                            TBCI_Input *inputs,
                                            struct TBCI_Context *ctx)
{
    if (producer == NULL || config == NULL)
        return TBCI_ERR_INVALID_ARG;

    SyntheticProducer *sp = (SyntheticProducer *)producer;
    sp_init(sp, config);

    if (producer->init == NULL) {
        fprintf(stderr, "producer->init is NULL\n");
        return TBCI_ERR_INVALID_STATE;
    }
    return producer->init(producer, inputs, ctx);
}

TBCI_Status tbci_producer_create_unicorn(TBCI_Producer *producer,
                                          UnicornProducerConfig *config,
                                          TBCI_Input *inputs,
                                          struct TBCI_Context *ctx)
{
    if (producer == NULL || config == NULL)
        return TBCI_ERR_INVALID_ARG;

    UnicornProducer *up = (UnicornProducer *)producer;
    up_init(up, config);

    if (producer->init == NULL) {
        fprintf(stderr, "producer->init is NULL\n");
        return TBCI_ERR_INVALID_STATE;
    }
    return producer->init(producer, inputs, ctx);
}

TBCI_Status tbci_producer_create_neuropawn(TBCI_Producer *producer,
                                           NeuroPawnProducerConfig *config,
                                           TBCI_Input *inputs,
                                           struct TBCI_Context *ctx)
{
    if (producer == NULL || config == NULL)
        return TBCI_ERR_INVALID_ARG;

    NeuroPawnProducer *np = (NeuroPawnProducer *)producer;
    np_init(np, config);

    if (producer->init == NULL) {
        fprintf(stderr, "producer->init is NULL\n");
        return TBCI_ERR_INVALID_STATE;
    }
    return producer->init(producer, inputs, ctx);
}

#ifdef TBCI_WITH_LSL
TBCI_Status tbci_producer_create_lsl(TBCI_Producer *producer,
                                      LSLProducerConfig *config,
                                      TBCI_Input *inputs,
                                      struct TBCI_Context *ctx)
{
    if (producer == NULL || config == NULL)
        return TBCI_ERR_INVALID_ARG;

    LSLProducer *lp = (LSLProducer *)producer;
    TBCI_Status s = lp_init(lp, config);
    if (s != TBCI_OK) return s;

    if (producer->init == NULL) {
        fprintf(stderr, "producer->init is NULL\n");
        return TBCI_ERR_INVALID_STATE;
    }
    return producer->init(producer, inputs, ctx);
}
#endif /* TBCI_WITH_LSL */