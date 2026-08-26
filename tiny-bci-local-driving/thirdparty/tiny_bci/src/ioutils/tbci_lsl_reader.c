/**
 * @file tbci_lsl_reader.c
 *
 * LSL reader implementation — no dynamic allocation.
 * All storage is caller-provided via lsl_init().
 */
#ifdef TBCI_WITH_LSL
#include "../../include/ioutils/tbci_lsl_reader.h"

/* --------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

static bool resolve_stream(const char *prop, const char *value, lsl_streaminfo *out_info)
{
    printf("lsl: waiting for stream where %s='%s'\n", prop, value);
    int result = lsl_resolve_byprop(out_info, 1, prop, value, 1, LSL_FOREVER);
    if (result < 1) {
        fprintf(stderr, "lsl: failed to resolve stream where %s='%s'\n", prop, value);
        return false;
    }
    return true;
}

/* Tear down whatever has been opened so far and reset ctx to a safe state. */
static void cleanup( TBCI_LSLContext *ctx )
{
    if ( ctx->data_inlet )   { lsl_destroy_inlet( ctx->data_inlet );   ctx->data_inlet   = NULL; }
    if ( ctx->marker_inlet ) { lsl_destroy_inlet( ctx->marker_inlet ); ctx->marker_inlet = NULL; }
    ctx->connected  = false;
}


/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

bool lsl_init_data(TBCI_LSLContext *ctx, const char *data_stream, float *temp_buf, int max_channels, LSLResolveMode resolve)
{
    if (!ctx || !data_stream || !temp_buf || max_channels <= 0)
        return false;

    memset(ctx, 0, sizeof(*ctx));
    ctx->temp_buf = temp_buf;

    const char *prop  = (resolve == LSL_RESOLVE_BY_TYPE) ? "type" : "name";

    lsl_streaminfo info;
    if (!resolve_stream(prop, data_stream, &info)) return false;

    ctx->n_channels = lsl_get_channel_count(info);
    if (ctx->n_channels <= 0 || ctx->n_channels > max_channels) {
        fprintf(stderr, "lsl_init_data: invalid channel count %d\n", ctx->n_channels);
        return false;
    }

    ctx->data_inlet = lsl_create_inlet(info, 300, LSL_NO_PREFERENCE, 1);
    if (!ctx->data_inlet) return false;

    int errcode;
    lsl_open_stream(ctx->data_inlet, LSL_FOREVER, &errcode);
    if (errcode != 0) { cleanup(ctx); return false; }

    ctx->connected = true;
    printf("lsl_init_data: connected '%s' (%d channels)\n", data_stream, ctx->n_channels);
    return true;
}

bool lsl_init_markers(TBCI_LSLContext *ctx, const char *marker_stream, LSLResolveMode resolve)
{
    if (!ctx || !marker_stream) return false;

    memset(ctx, 0, sizeof(*ctx));
    const char *prop = (resolve == LSL_RESOLVE_BY_TYPE) ? "type" : "name";

    lsl_streaminfo info;
    if (!resolve_stream(prop, marker_stream, &info)) return false;

    ctx->marker_inlet = lsl_create_inlet(info, 300, LSL_NO_PREFERENCE, 1);
    if (!ctx->marker_inlet) return false;

    int errcode;
    lsl_open_stream(ctx->marker_inlet, LSL_FOREVER, &errcode);
    if (errcode != 0) { cleanup(ctx); return false; }

    printf("lsl_init_markers: connected '%s'\n", marker_stream);
    return true;
}

bool lsl_init_all(TBCI_LSLContext *ctx, const char *data_stream,
                  const char *marker_stream, float *temp_buf, int max_channels, LSLResolveMode resolve)
{
    if (!lsl_init_data(ctx, data_stream, temp_buf, max_channels, resolve))
        return false;

    const char *prop = (resolve == LSL_RESOLVE_BY_TYPE) ? "type" : "name";
    lsl_streaminfo info;
    if (!resolve_stream(prop, marker_stream, &info)) { cleanup(ctx); return false; }

    ctx->marker_inlet = lsl_create_inlet(info, 300, LSL_NO_PREFERENCE, 1);
    if (!ctx->marker_inlet) { cleanup(ctx); return false; }

    int errcode;
    lsl_open_stream(ctx->marker_inlet, LSL_FOREVER, &errcode);
    if (errcode != 0) { cleanup(ctx); return false; }

    printf("lsl_init_all: marker stream '%s' connected\n", marker_stream);
    return true;
}

bool lsl_update(TBCI_LSLContext *ctx)
{
    if (!ctx || !ctx->connected || !ctx->inputs) return false;

    int    errcode;
    double timestamp;
    bool   got_data = false;

    /* drain all available EEG samples */
    if (ctx->data_inlet) {
        while (1) {
            errcode   = 0;
            timestamp = lsl_pull_sample_f(
                ctx->data_inlet,
                ctx->temp_buf,
                ctx->n_channels,
                0.0,
                &errcode
            );
            if (errcode != 0) {
                fprintf(stderr, "lsl: data inlet error (errcode=%d), marking disconnected\n", errcode);
                ctx->connected = false;
                return false;
            }
            if (timestamp == 0.0) break;

            uint64_t ts_us = (uint64_t)(timestamp * 1e6);
            sb_put(ctx->inputs->signal, ctx->temp_buf, ts_us, ctx->sample_index++);
            got_data = true;
        }
    }

    /* drain all available markers */
    if (ctx->marker_inlet) {
        while (1) {
            char *marker_ptr = NULL;
            errcode = 0;
            timestamp = lsl_pull_sample_str(
                ctx->marker_inlet,
                &marker_ptr,
                1,
                0.0,
                &errcode
            );
            if (errcode != 0) {
                fprintf(stderr, "lsl: trigger inlet error (errcode=%d), marking disconnected\n", errcode);
                ctx->connected = false;
                return false;
            }
            if (timestamp == 0.0 || marker_ptr == NULL) break;

            char label[64] = {0};
            strncpy(label, marker_ptr, sizeof(label) - 1);
            lsl_destroy_string(marker_ptr);

            TBCI_Trigger trigger = {
                .timestamp_us = (uint64_t)(timestamp * 1e6),
                .code         = (uint16_t)strtol(label, NULL, 10),
                .type         = TBCI_TRIGGER_DATA,
            };
            tq_push(ctx->inputs->triggers, &trigger);
        }
    }

    /* note: returns false when no EEG data arrived this tick,
    * even if markers were received — not a disconnect signal */
    return got_data;
}

void lsl_close(TBCI_LSLContext *ctx)
{
    if (!ctx) return;
    cleanup(ctx);
}
#endif