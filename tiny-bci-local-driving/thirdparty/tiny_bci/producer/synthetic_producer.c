/**
 * @file synthetic_producer.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Synthetic EEG and trigger producer implementation.
 */

#include "synthetic_producer.h"
#include "tbci_context.h"
#include "../include/mathutils/tbci_math.h"


static TBCI_Status sp_connect(TBCI_Producer *producer,
                              TBCI_Input *inputs,
                              struct TBCI_Context *ctx)
{
    (void)inputs;
    (void)ctx;
    producer->connected = true;
    return TBCI_OK;  // no-op for synthetic
}

TBCI_Status sp_init(SyntheticProducer *producer, SyntheticProducerConfig *config)
{
    if (producer == NULL || config == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (config->n_channels == 0 || config->srate == 0.0f)
        return TBCI_ERR_INVALID_ARG;

    producer->base.init  = sp_connect;
    producer->base.tick  = (TBCI_Status(*)(TBCI_Producer*, TBCI_Input*, struct TBCI_Context*)) sp_tick;
    producer->base.reset = (TBCI_Status(*)(TBCI_Producer*)) sp_reset;
    producer->base.close = (TBCI_Status(*)(TBCI_Producer*)) sp_close;
    producer->config = config;

    producer->state = (SyntheticProducerState){0};
    producer->state.spacing_us = (uint32_t)(1000000.0f / config->srate);
    producer->state.last_trial_start_us = -1;

    return TBCI_OK;
}

TBCI_Status sp_tick(SyntheticProducer *producer, TBCI_Input *inputs, TBCI_Context *ctx)
{
    if (producer == NULL || inputs == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    SyntheticProducerConfig *config = producer->config;
    SyntheticProducerState   *state  = &producer->state;

    /* generate one multichannel sample — each channel is a sine wave
     * at a slightly different phase so channels are distinguishable */
    #if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    float samples[TBCI_MAX_CHANNELS];
    #else
    float samples[config->n_channels];
    #endif
    float t = (float)state->timestamp_us / 1000000.0f;  /* time in seconds */

    for (size_t ch = 0; ch < config->n_channels; ch++) {
        float phase_offset = (float)ch * (2.0f * (float)TBCI_M_PI / (float)config->n_channels);
        samples[ch] = config->amplitude *
                      sinf(2.0f * (float)TBCI_M_PI * config->freq_hz * t + phase_offset);

        /* additive white noise */
        if (config->noise_amplitude > 0.0f) {
            float noise = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;  /* [-1, 1] */
            samples[ch] += config->noise_amplitude * noise;
        }
    }

    /* push signal frame into buffer */
    in_push_signal(inputs, samples, state->timestamp_us, state->sample_index);

    // in up_tick after pushing signal
    if (producer->trigger_gen != NULL) {
        tg_tick(producer->trigger_gen, inputs, ctx, state->timestamp_us);
    }

    /* advance time */
    state->timestamp_us += state->spacing_us;
    state->sample_index++;

    return TBCI_OK;
}

TBCI_Status sp_reset(SyntheticProducer *producer)
{
    if (producer == NULL)
        return TBCI_ERR_INVALID_ARG;

    producer->state = (SyntheticProducerState){0};
    producer->state.spacing_us = (uint32_t)(1000000.0f / producer->config->srate);
    producer->state.last_trial_start_us = -1;

    return TBCI_OK;
}

TBCI_Status sp_close(SyntheticProducer *producer)
{
    if (producer == NULL) return TBCI_ERR_INVALID_ARG;
    producer->base.connected = false;
    return TBCI_OK;
}