/**
* @file tbci_input.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief TinyBCI pipeline input implementation.
 */

#include "tbci_context.h"
#include "../../include/containers/tbci_signal_buffer.h"
#include "../../include/containers/tbci_trigger_queue.h"


TBCI_Status in_push_signal(TBCI_Input *input, float *samples, uint64_t timestamp_us, uint32_t index)
{
    if (input == NULL || samples == NULL)
        return TBCI_ERR_INVALID_ARG;
    return sb_put(input->signal, samples, timestamp_us, index);
}

TBCI_Status in_push_trigger(TBCI_Input *input, TBCI_Trigger *trigger, TBCI_Context *ctx)
{
    if (input == NULL || trigger == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    // commands [192-255] → dispatch to context
    if (trigger->code >= 192) {
        switch (trigger->code) {
        case 192: return tbci_context_stop(ctx);
        case 193: return tbci_context_start(ctx, TBCI_STATE_INFERENCE);
        case 194: return tbci_context_start(ctx, TBCI_STATE_TRAINING);
        default:  return TBCI_ERR_INVALID_ARG;
        }
    }

    // reserved [0] → ignore
    if (trigger->code == 0)
        return TBCI_OK;

    // data class [1-191] → push to trigger queue
    return tq_push(input->triggers, trigger);
}