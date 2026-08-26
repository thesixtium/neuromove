/**
 * @file tbci_trigger_generator.c
 */

#include "tbci_trigger_generator.h"
#include "tbci_context.h"

TBCI_Status tg_init(TBCI_TriggerGenerator       *gen,
                    TBCI_TriggerGeneratorConfig *config,
                    TBCI_TriggerGeneratorState  *state)
{
    if (gen == NULL || config == NULL || state == NULL)
        return TBCI_ERR_INVALID_ARG;

    gen->config = config;
    gen->state  = state;

    state->trigger_interval_us  = config->trigger_interval_ms * 1000u;
    state->trial_duration_us    = (uint64_t)config->trial_duration_ms * 1000u;
    state->last_trigger_us      = 0;
    state->last_trial_start_us  = -1;
    state->trial_ended          = false;

    return TBCI_OK;
}

TBCI_Status tg_tick(TBCI_TriggerGenerator *gen,
                    TBCI_Input            *inputs,
                    struct TBCI_Context   *ctx,
                    uint64_t               timestamp_us)
{
    if (gen == NULL || inputs == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    TBCI_TriggerGeneratorConfig *config = gen->config;
    TBCI_TriggerGeneratorState  *state  = gen->state;

    /* fire start trigger if interval elapsed */
    if (state->trigger_interval_us > 0 &&
        (timestamp_us - state->last_trigger_us) >= state->trigger_interval_us)
    {
        TBCI_Trigger trigger = {
            .timestamp_us = timestamp_us,
            .code         = config->trigger_code,
            .type         = TBCI_TRIGGER_DATA
        };
        in_push_trigger(inputs, &trigger, ctx);
        state->last_trigger_us     = timestamp_us;
        state->last_trial_start_us = (int64_t)timestamp_us;
        state->trial_ended         = false;
    }

    /* fire end trigger if trial duration elapsed */
    if (config->trial_end_code > 0 &&
        state->last_trial_start_us >= 0 &&
        !state->trial_ended &&
        (timestamp_us - (uint64_t)state->last_trial_start_us) >= state->trial_duration_us)
    {
        TBCI_Trigger end_trigger = {
            .timestamp_us = timestamp_us,
            .code         = config->trial_end_code,
            .type         = TBCI_TRIGGER_DATA
        };
        printf("Pushing trigger %d\n", config->trial_end_code);
        in_push_trigger(inputs, &end_trigger, ctx);
        state->trial_ended = true;
    }

    return TBCI_OK;
}

TBCI_Status tg_reset(TBCI_TriggerGenerator *gen)
{
    if (gen == NULL) return TBCI_ERR_INVALID_ARG;

    gen->state->last_trigger_us     = 0;
    gen->state->last_trial_start_us = -1;
    gen->state->trial_ended         = false;

    return TBCI_OK;
}