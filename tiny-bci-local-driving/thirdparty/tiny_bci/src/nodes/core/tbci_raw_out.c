/**
 * @file tbci_raw_out.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Raw EEG logger node implementation.
 */
#include "tbci_context.h"
#include "nodes/core/tbci_raw_out.h"
#include <time.h>
#include "../../../include/containers/tbci_signal_buffer.h"

/* --------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

static void ro_write_header(TBCI_RawOutNode *node, size_t n_channels)
{
    fprintf(node->file, "sample_idx,timestamp_us,trigger_val");
    for (size_t ch = 0; ch < n_channels; ch++)
        fprintf(node->file, ",ch%zu", ch);
    fprintf(node->file, "\n");
    node->header_written = true;
}

static bool ro_is_command(uint16_t code)
{
    return code >= 192;
}

static void ro_tap(TBCI_RawOutNode *node, const float *samples, size_t n_channels, const TBCI_Frame *frame, uint16_t trigger_val)
{
    if (node->on_frame != NULL)
        node->on_frame(samples, n_channels, frame, trigger_val, node->user_data);
}

static void ro_log(TBCI_RawOutNode *node, const float *samples, size_t n_channels, const TBCI_Frame *frame, uint16_t trigger_val)
{
    if (!node->base.enabled || node->file == NULL)
        return;

    if (!node->header_written)
        ro_write_header(node, n_channels);

    fprintf(node->file, "%llu,%llu,%u",
            (unsigned long long)node->sample_index,
            (unsigned long long)frame->timestamp_us,
            trigger_val);

    for (size_t ch = 0; ch < n_channels; ch++)
        fprintf(node->file, ",%.6f", samples[ch]);

    fprintf(node->file, "\n");
}

static void ro_write_cb(const float *samples, const TBCI_Frame *frame, void *user_data)
{
    RoWriteCtx *wctx = (RoWriteCtx *)user_data;

    /* only attach trigger to the frame closest to the trigger timestamp */
    uint16_t frame_trigger = 0;
    if (wctx->trigger_val != 0 && !wctx->trigger_fired && frame->timestamp_us >= wctx->sync_result->trigger.timestamp_us)
    {
        frame_trigger        = wctx->trigger_val;
        wctx->trigger_fired  = true;  /* only fire once */
    }

    ro_tap(wctx->node, samples, wctx->n_channels, frame, frame_trigger);
    ro_log(wctx->node, samples, wctx->n_channels, frame, frame_trigger);
    wctx->node->sample_index++;
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

TBCI_Status ro_init(TBCI_RawOutNode *node, TBCI_CoreConfig *config,
                    struct TBCI_Context *ctx)
{
    if (node == NULL || config == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    node->config         = *config;
    node->sample_index   = 0;
    node->last_written_ts = 0;
    node->header_written = false;
    node->file           = NULL;
    node->on_frame  = NULL;
    node->user_data = NULL;

    node->base.name    = "raw_out";
    node->base.type    = TBCI_NODE_TYPE_DATA_OUT;
    node->base.enabled = config->log_enabled;
    node->base.instance_size = sizeof(TBCI_RawOutNode);

    if (!config->log_enabled)
        return TBCI_OK;  /* logging disabled — no file opened */
    printf("Logging %s data\n", config->log_processed ? "preprocessed" : "raw");
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", t);
    snprintf(node->filepath, sizeof(node->filepath),
             "tbci_out_%s_%s_%s.csv",
             config->log_subject, config->log_session, timestamp);

    node->file = fopen(node->filepath, "w");
    if (node->file == NULL) {
        fprintf(stderr, "Raw Logger: failed to open '%s' — logging disabled\n", node->filepath);
        node->base.enabled = false;
        return TBCI_OK;  /* non-fatal */
    }

    printf("Raw Logger: logging to '%s'\n", node->filepath);
    return TBCI_OK;
}

TBCI_NodeResult ro_write(TBCI_RawOutNode *node, const TBCI_SyncResult *sync_result, struct TBCI_Context *ctx)
{
    if (node == NULL || sync_result == NULL || ctx == NULL)
        return TBCI_NODE_ERROR;

    size_t n_channels = ctx->config.n_channels;

    uint16_t trigger_val = 0;
    if (sync_result->new_trigger && sync_result->trigger.code != 0) {
        bool is_cmd = ro_is_command(sync_result->trigger.code);
        if (!is_cmd || node->config.log_commands)
            trigger_val = sync_result->trigger.code;
    }

    if (trigger_val != 0 && node->file != NULL)
        fflush(node->file);

    RoWriteCtx wctx = {
        .node        = node,
        .n_channels  = n_channels,
        .trigger_val = trigger_val,
        .sync_result = sync_result,
        .trigger_fired = false
    };
    TBCI_SignalBuffer *src = node->config.log_processed ? ctx->processed_signal : ctx->inputs->signal;
    sb_read_since(src, node->last_written_ts, ro_write_cb, &wctx);

    /* update last_written_ts to latest frame */
    TBCI_Frame latest;
    float tmp[TBCI_MAX_CHANNELS];
    if (sb_peek_latest(src, tmp, &latest) == TBCI_OK)
        node->last_written_ts = latest.timestamp_us;

    return TBCI_NODE_OK;
}

TBCI_Status ro_close(TBCI_RawOutNode *node)
{
    if (node == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (node->file != NULL) {
        fflush(node->file);
        fclose(node->file);
        node->file = NULL;
        printf("ro_close: file '%s' saved\n", node->filepath);
    }

    return TBCI_OK;
}