/**
* @file tbci_sync_node.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief TinyBCI pipeline Synchronization Node implementation.
 */

#include "tbci_context.h"
#include "../../../include/nodes/core/tbci_sync_node.h"

/* --------------------------------------------------------------------------
 *  Helpers
 * -------------------------------------------------------------------------- */

/* Pop trigger from queue into state->pending_trigger.
 * Returns TBCI_OK if trigger found, TBCI_ERR_EMPTY if queue empty,
 * TBCI_ERR_INVALID_ARG on null input. */
static TBCI_Status sn_idle_pop_trigger(TBCI_SyncState* state, const TBCI_Input* inputs)
{
    TBCI_Status s = tq_pop(inputs->triggers, &state->pending_trigger);
    if (s == TBCI_OK)
        state->new_trigger = true;  /* add new_trigger to TBCI_SyncState too */
    return s;
}

/* Attempt to start a new sliding-mode trial from the trigger queue.
 *
 * Peeks (does not pop) the front of the trigger queue:
 *   - If empty, returns without side effects.
 *   - If the front trigger is a trial-end code, it is popped and discarded
 *     (a stray end trigger while idle has no trial to end) and an error is
 *     returned so the caller stays in SYNC_IDLE.
 *   - Otherwise, the trigger is treated as a candidate trial start. It is
 *     located in the processed signal buffer to confirm enough pre-stimulus
 *     history (pre_frames) is already buffered. If not yet available, the
 *     trigger is left in the queue (not popped) so it can be re-evaluated
 *     on a later tick once more signal has arrived.
 *   - Only once the trigger is confirmed usable is it popped from the queue,
 *     stored as state->pending_trigger, and used to compute window_start_us
 *     (trigger timestamp minus pre_frames of lead-in).
 *
 * The trigger is deliberately left unconsumed in the two "not ready" cases
 * (NOT_YET / signal lookup failure) so no trigger is ever lost while the
 * pipeline is still catching up. It is consumed exactly once a trial is
 * actually going to start, or discarded immediately if it can never be
 * used (end code while idle).
 *
 * Returns:
 *   TBCI_OK              trial started; window_start_us and pending_trigger are set,
 *                         caller should transition to SYNC_MATCHING.
 *   TBCI_ERR_EMPTY        no trigger available.
 *   TBCI_ERR_NOT_FOUND    front trigger was a trial-end code; discarded, no trial started.
 *   TBCI_ERR_NOT_YET      candidate start trigger found, but not enough pre-stimulus
 *                         signal buffered yet; trigger left in queue for retry.
 *   (propagated)          any other non-OK status from the signal buffer lookup is
 *                         returned as-is so the caller can retry.
 *   TBCI_ERR_INVALID_ARG  programming error (null pointers), propagated from callees.
 */
static TBCI_Status sn_idle_start_trial(TBCI_SyncState* state, TBCI_CoreConfig* config,
                                       const TBCI_Input* inputs, TBCI_Context* ctx)
{
    TBCI_Trigger peek;
    TBCI_Status s = tq_peek(inputs->triggers, &peek);
    if (s != TBCI_OK) return s;   // ERR_EMPTY — nothing to do

    if (peek.code == config->trial_end_code)
    {
        tq_pop(inputs->triggers, &peek);   // discard stray end trigger while idle
        state->pending_trigger = peek;
        state->new_trigger     = true;
        return TBCI_ERR_NOT_FOUND;
    }

    TBCI_MatchType match_type;
    size_t trigger_frame;
    TBCI_Status fs = sb_find_timestamp(ctx->processed_signal, peek.timestamp_us, &trigger_frame, &match_type);
    if (fs != TBCI_OK) return fs;
    if (trigger_frame < state->pre_frames)
        return TBCI_ERR_NOT_YET;

    tq_pop(inputs->triggers, &peek);
    state->pending_trigger = peek;
    state->new_trigger     = true;
    state->window_start_us = peek.timestamp_us - (state->pre_frames * state->spacing_us);
    return TBCI_OK;
}

/* Peek at front of trigger queue for end trigger.
 * Returns TBCI_OK if end trigger found, TBCI_ERR_EMPTY if queue empty,
 * TBCI_ERR_NOT_FOUND if front trigger is not end trigger. */
static TBCI_Status check_end_trigger(TBCI_CoreConfig *config, const TBCI_Input *inputs)
{
    TBCI_Trigger peek;
    TBCI_Status s = tq_peek(inputs->triggers, &peek);
    if (s != TBCI_OK) return s;
    if (peek.code == config->trial_end_code)
        return TBCI_OK;
    return TBCI_ERR_NOT_FOUND;
}
// TODO: verify if this function correctly prevents stale triggers over check_end_trigger() while not breaking existing behaviour
/* Drain non-end triggers from the front of the queue, updating pending_trigger
 * to the most recent one seen. Returns TBCI_OK if the end trigger is now at
 * the front (not consumed), TBCI_ERR_EMPTY if the queue is drained with no
 * end trigger found yet. */
static TBCI_Status sn_running_drain_and_check_end(TBCI_SyncState* state, TBCI_CoreConfig* config,
                                                   const TBCI_Input* inputs)
{
    TBCI_Trigger peek;
    while (tq_peek(inputs->triggers, &peek) == TBCI_OK)
    {
        if (peek.code == config->trial_end_code)
        {
            TBCI_Trigger consumed;
            tq_pop(inputs->triggers, &consumed);  /* consume end trigger */
            state->pending_trigger = consumed;
            state->new_trigger     = true;
            return TBCI_OK;
        }

        TBCI_Trigger consumed;
        tq_pop(inputs->triggers, &consumed);
        state->pending_trigger = consumed;   // update label to most recent class marker
        state->new_trigger = true;
    }
    return TBCI_ERR_EMPTY;
}

static TBCI_NodeResult sn_sync_triggered(TBCI_SyncNode* node, const TBCI_Input* inputs, TBCI_SyncResult* result_out, TBCI_Context* ctx)
{
    TBCI_SyncState *state = &node->state;

    result_out->new_trigger = state->new_trigger;

    if (state->synch_phase == SYNC_IDLE)
    {
        TBCI_Status s = sn_idle_pop_trigger(state, inputs);
        if (s != TBCI_OK) return (s == TBCI_ERR_INVALID_ARG) ? TBCI_NODE_ERROR : TBCI_NODE_PENDING;
        state->synch_phase = SYNC_MATCHING;
    }

    if (state->synch_phase == SYNC_MATCHING) {
        TBCI_MatchType match_type;
        size_t trigger_frame;
        TBCI_Status s = sb_find_timestamp( ctx->processed_signal,
                                           state->pending_trigger.timestamp_us,
                                           &trigger_frame, &match_type);
        if (s == TBCI_ERR_INVALID_ARG) return TBCI_NODE_ERROR;
        if (s != TBCI_OK)              return TBCI_NODE_PENDING;
        if (trigger_frame < state->pre_frames) return TBCI_NODE_PENDING;

        result_out->trigger = state->pending_trigger;
        result_out->signal = ctx->processed_signal;
        result_out->new_trigger = state->new_trigger;

        return TBCI_NODE_OK;
    }
    return TBCI_NODE_PENDING;
}

static TBCI_NodeResult sn_sync_sliding(TBCI_SyncNode* node, const TBCI_Input* inputs, TBCI_SyncResult* result_out, TBCI_Context* ctx)
{
    TBCI_SyncState* state = &node->state;
    TBCI_CoreConfig* config = node->config;

    if (state->synch_phase == SYNC_IDLE)
    {
        // step 1: look for a trigger in the queue
        // any non-end trigger starts a trial
        // end trigger while idle → ignore, stay idle
        TBCI_Status s = sn_idle_start_trial(state, config, inputs, ctx);
        if (s == TBCI_ERR_INVALID_ARG)  return TBCI_NODE_ERROR;   // programming error
        if (s != TBCI_OK)               return TBCI_NODE_PENDING;  // not ready — covers EMPTY, NOT_YET, NOT_OK
        state->synch_phase = SYNC_MATCHING;
    }

    if (state->synch_phase == SYNC_MATCHING) {
        // window_start already set by sn_idle_start_trial
        result_out->window_start_us = state->window_start_us;
        result_out->trigger      = state->pending_trigger;
        result_out->signal       =  ctx->processed_signal;
        result_out->new_trigger = state->new_trigger;

        state->synch_phase = SYNC_RUNNING;
        return TBCI_NODE_OK;
    }

    if (state->synch_phase == SYNC_RUNNING)
    {
        // check for end trigger without consuming it, if found put the node back in IDLE
        TBCI_Status s = sn_running_drain_and_check_end(state, node->config, inputs);
        result_out->new_trigger = state->new_trigger;
        result_out->trigger     = state->pending_trigger;

        if (s == TBCI_OK) {
            state->synch_phase      = SYNC_IDLE;
            result_out->trial_ended = true;
            return TBCI_NODE_PENDING;
        }
        // trial still active — re-emit same window_start
        // segmentation will advance window_start internally
        result_out->window_start_us = state->window_start_us;
        result_out->signal       =  ctx->processed_signal;
        return TBCI_NODE_OK;
    }

    return TBCI_NODE_PENDING;
}


TBCI_Status sync_init(TBCI_SyncNode* node, TBCI_CoreConfig* config, struct TBCI_Context* ctx)
{
    if (node == NULL || config == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    node->config = config;
    node->base.enabled = true;
    node->base.name = "synchronization";
    node->base.type = TBCI_NODE_TYPE_CORE;
    node->base.instance_size = sizeof(TBCI_SyncNode);

    node->state.synch_phase = SYNC_IDLE;
    node->state.pre_frames = (size_t)roundf(node->config->pre_stimulus_ms / 1000.0f * ctx->config.target_srate);
    node->state.pending_trigger = (TBCI_Trigger){0};
    node->state.spacing_us = (uint64_t)(1000000.0f / ctx->config.target_srate);

    return TBCI_OK;
}

TBCI_NodeResult sync_process(TBCI_SyncNode* node, TBCI_Input* inputs, struct TBCI_Context* ctx,
    TBCI_SyncResult* result_out)
{
    if (node == NULL || inputs == NULL || result_out == NULL || ctx == NULL)
        return TBCI_NODE_ERROR;

    result_out->trial_ended = false;
    result_out->new_trigger  = false;  /* reset each tick */
    node->state.new_trigger  = false;  /* reset state too */
    if (node->config->mode == SEG_MODE_TRIGGERED)
    {
        return sn_sync_triggered(node, inputs, result_out, ctx);
    }

    if (node->config->mode == SEG_MODE_SLIDING)
    {
        return sn_sync_sliding(node, inputs, result_out, ctx);
    }

    return TBCI_NODE_ERROR;
}

TBCI_Status sync_reset(TBCI_SyncNode* node)
{
    if (node == NULL)
        return TBCI_ERR_INVALID_ARG;
    node->state.synch_phase = SYNC_IDLE;
    node->state.window_start    = 0;
    node->state.pending_trigger = (TBCI_Trigger){0};

    return TBCI_OK;
}

