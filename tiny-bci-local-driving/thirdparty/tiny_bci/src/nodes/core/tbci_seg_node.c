/**
* @file tbci_seg_node.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief TinyBCI pipeline SegmentationNode implementation.
 */
#include "../../../include/nodes/core/tbci_seg_node.h"
#include "tbci_context.h"
/* --------------------------------------------------------------------------
 * sn_process_triggered helpers
 * -------------------------------------------------------------------------- */

/* Check pre- and post-stimulus frames are available around frame_index.
 * Returns TBCI_OK if window is ready, TBCI_ERR_NOT_YET otherwise. */
static TBCI_Status sn_waiting_check_window(const TBCI_SegmentationState* state, const TBCI_SyncResult* inputs,
                                           size_t frame_index, TBCI_Context *ctx)
{
    size_t available;
    sb_frames_available_from(ctx->processed_signal, frame_index, &available);
    if (available < state->total_frames)
        return TBCI_ERR_NOT_YET;
    return TBCI_OK;
}

/* Extract epoch from signal buffer and push to epoch queue.
 * Returns TBCI_OK on success, TBCI_ERR_FULL if epoch queue full. */
static TBCI_Status sn_waiting_extract_epoch(TBCI_SegmentationState* state, const TBCI_SyncResult* inputs,
                                            TBCI_EpochQueue* epoch_queue, TBCI_Context* ctx, size_t frame_index)
{
    TBCI_Epoch epoch;
    size_t total_frames = state->total_frames;
    float* slot = eq_next_slot(epoch_queue);
    if (slot == NULL)
        return TBCI_ERR_INVALID_STATE;

    sb_read_from( ctx->processed_signal, frame_index, total_frames, slot, NULL);
    epoch.timestamp_us = state->pending_trigger.timestamp_us;
    epoch.label = state->pending_trigger.code;
    epoch.samples = slot;
    epoch.n_channels = ctx->config.n_channels;
    epoch.n_frames = total_frames;

    TBCI_Status push_status = eq_push(epoch_queue, &epoch);
    return push_status;
}

/* --------------------------------------------------------------------------
 * sn_process_sliding helpers
 * -------------------------------------------------------------------------- */

/* Check total_frames available from window_start.
 * Returns TBCI_OK if enough data, TBCI_ERR_NOT_YET otherwise. */
static TBCI_Status sn_running_check_data(const TBCI_SegmentationState *state, const TBCI_SyncResult *inputs, TBCI_Context *ctx)
{
    TBCI_MatchType match;
    size_t frame_index;
    TBCI_Status s = sb_find_timestamp( ctx->processed_signal, state->window_start_us,
                                       &frame_index, &match);
    if (s != TBCI_OK) return TBCI_ERR_NOT_YET;

    size_t available;
    sb_frames_available_from( ctx->processed_signal, frame_index, &available);
    if (available < state->total_frames)
        return TBCI_ERR_NOT_YET;
    return TBCI_OK;
}

/* Extract epoch from window_start and push to epoch queue.
 * Returns TBCI_OK on success, TBCI_ERR_FULL if epoch queue full. */
static TBCI_Status sn_running_extract_epoch(TBCI_SegmentationState* state, const TBCI_SyncResult* inputs,
                                            TBCI_EpochQueue* epoch_queue, TBCI_Context* ctx)
{
    TBCI_MatchType match;
    size_t frame_index;
    TBCI_Status s = sb_find_timestamp( ctx->processed_signal, state->window_start_us,
                                       &frame_index, &match);
    if (s != TBCI_OK) return TBCI_ERR_INVALID_STATE;

    float *slot = eq_next_slot(epoch_queue);
    if (slot == NULL) return TBCI_ERR_INVALID_ARG;

    sb_read_from( ctx->processed_signal, frame_index, state->total_frames, slot, NULL);

    TBCI_Epoch epoch;
    epoch.timestamp_us = state->window_start_us;
    epoch.label        = state->pending_trigger.code;
    epoch.samples      = slot;
    epoch.n_channels   = ctx->config.n_channels;
    epoch.n_frames     = state->total_frames;

    return eq_push(epoch_queue, &epoch);
}



/* Advance window_start by step_frames. */
static void sn_running_advance_window(TBCI_SegmentationState* state)
{
    state->window_start_us += state->step_frames * state->spacing_us;
}


/* --------------------------------------------------------------------------
 * Segmentation Node Helper Methods
 * -------------------------------------------------------------------------- */

static TBCI_NodeResult sn_process_triggered(TBCI_SegNode* node, const TBCI_SyncResult* sync,
                                            TBCI_EpochQueue* epoch_queue, TBCI_Context* ctx)
{
    TBCI_SegmentationState* state = &node->state;
    state->pending_trigger = sync->trigger;
    // We check frames starting from the trigger frame_index, in triggered mode that's our clock
    if (sn_waiting_check_window(state, sync, sync->window_start_us, ctx) != TBCI_OK) return TBCI_NODE_PENDING;
    // if enough data → extract epoch, push to epoch queue
    if (sn_waiting_extract_epoch(state, sync, epoch_queue, ctx, sync->window_start_us) != TBCI_OK) return TBCI_NODE_ERROR;

    return TBCI_NODE_OK;
}

TBCI_NodeResult sn_process_sliding(TBCI_SegNode* node, TBCI_SyncResult* sync, TBCI_EpochQueue* epoch_queue,
                                   TBCI_Context* ctx)
{
    TBCI_SegmentationState* state = &node->state;

    if (state->phase == SEG_IDLE) {
        state->window_start_us    = sync->window_start_us;
        state->pending_trigger = sync->trigger;
        state->phase           = SEG_RUNNING;
    }

    // step 2: check if enough data available from window_start
    if (sn_running_check_data(state, sync, ctx) != TBCI_OK) return TBCI_NODE_PENDING;
    // step 3: extract epoch from window_start
    if (sn_running_extract_epoch(state, sync, epoch_queue, ctx) != TBCI_OK) return TBCI_NODE_ERROR;
    // step 4: advance window_start by step_frames
    sn_running_advance_window(state);

    return TBCI_NODE_OK;
}

/* --------------------------------------------------------------------------
 * Segmentation Node Methods
 * -------------------------------------------------------------------------- */

TBCI_Status seg_init(TBCI_SegNode *node, TBCI_CoreConfig *config,
                     struct TBCI_Context *ctx)
{
    if (node == NULL || config == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (config->post_stimulus_ms == 0)
        return TBCI_ERR_INVALID_ARG;

    node->base.enabled = true;
    node->base.name    = "segmentation";
    node->base.type    = TBCI_NODE_TYPE_CORE;
    node->base.instance_size = sizeof(TBCI_SegNode);
    node->config       = config;

    TBCI_SegmentationState *state = &node->state;

    state->phase           = SEG_IDLE;
    state->pending_trigger = (TBCI_Trigger){0};
    state->spacing_us      = (uint64_t)(1000000.0f / ctx->config.target_srate);
    state->window_start_us = 0;
    state->pre_frames      = (size_t)roundf(config->pre_stimulus_ms  / 1000.0f * ctx->config.target_srate);
    state->post_frames     = (size_t)roundf(config->post_stimulus_ms / 1000.0f * ctx->config.target_srate);
    state->total_frames    = state->pre_frames + state->post_frames;
    state->overlap_frames  = 0;
    state->step_frames     = 0;


    if (config->mode == SEG_MODE_SLIDING)
    {
        if (config->overlap_ms >= config->post_stimulus_ms + config->pre_stimulus_ms)
            return TBCI_ERR_INVALID_ARG;
        state->overlap_frames = (size_t)(config->overlap_ms / 1000.0f * ctx->config.target_srate);
        state->step_frames    = state->total_frames - state->overlap_frames;
        if (ctx->config.paradigm == TBCI_PARADIGM_P300)
            return TBCI_WARN_PARADIGM_MODE_MISMATCH;
    }

    if (config->mode == SEG_MODE_TRIGGERED)
    {
        if (ctx->config.paradigm == TBCI_PARADIGM_MI ||
            ctx->config.paradigm == TBCI_PARADIGM_SSVEP)
            return TBCI_WARN_PARADIGM_MODE_MISMATCH;
    }

    return TBCI_OK;
}

TBCI_NodeResult seg_process(TBCI_SegNode* node, TBCI_SyncResult* sync, TBCI_EpochQueue* epoch_queue, struct TBCI_Context* ctx)
{
    if (node == NULL || sync == NULL || epoch_queue == NULL || ctx == NULL)
        return TBCI_NODE_ERROR;

    if (node->config->mode == SEG_MODE_TRIGGERED)
    {
        return sn_process_triggered(node, sync, epoch_queue, ctx);
    }

    if (node->config->mode == SEG_MODE_SLIDING)
    {
        return sn_process_sliding(node, sync, epoch_queue, ctx);
    }

    return TBCI_NODE_ERROR;
}

TBCI_Status seg_reset(TBCI_SegNode *node)
{
    if (node == NULL)
        return TBCI_ERR_INVALID_ARG;

    node->state.phase           = SEG_IDLE;
    node->state.window_start_us    = 0;
    node->state.pending_trigger = (TBCI_Trigger){0};

    return TBCI_OK;
}

