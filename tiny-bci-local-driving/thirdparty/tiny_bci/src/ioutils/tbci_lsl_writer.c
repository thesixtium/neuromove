/**
* @file tbci_lsl_writer.c
 *
 * LSL writer implementation — no dynamic allocation.
 * All storage is caller-provided via lsl_init().
 */
#ifdef TBCI_WITH_LSL
#include "../../include/ioutils/tbci_lsl_writer.h"

bool lsl_writer_init(TBCI_LSLWriter *writer, const char *stream_name,
                     const char *stream_type, int n_channels, float srate)
{
    if (!writer || !stream_name || !stream_type || n_channels <= 0)
        return false;

    memset(writer, 0, sizeof(*writer));

    n_channels += 3; // including prediction value, confidence and eval score
    lsl_streaminfo info = lsl_create_streaminfo(stream_name, stream_type, n_channels, srate,cft_float32, stream_name);
    if (!info) {
        fprintf(stderr, "lsl_writer_init: failed to create stream info\n");
        return false;
    }

    writer->outlet = lsl_create_outlet(info, 0, 360);
    lsl_destroy_streaminfo(info);

    if (!writer->outlet) {
        fprintf(stderr, "lsl_writer_init: failed to create outlet\n");
        return false;
    }

    writer->n_channels = n_channels;
    writer->connected  = true;

    printf("lsl_writer_init: outlet '%s' (%s, %d channels) created\n",
           stream_name, stream_type, n_channels);
    return true;
}

bool lsl_writer_push_epoch(TBCI_LSLWriter *writer, const TBCI_Epoch *epoch)
{
    if (!writer || !writer->connected || !writer->outlet)
        return false;
    if (!epoch || !epoch->samples)
        return false;

    /* prepend predicted_label to samples */
    float buf[writer->n_channels];
    buf[0] = (float)epoch->predicted_label;
    buf[1] = (float)epoch->confidence;
    buf[2] = (float)epoch->eval_score;
    memcpy(buf + 2, epoch->samples, (writer->n_channels) * sizeof(float));

    int errcode = lsl_push_sample_f(writer->outlet, buf);
    return (errcode == 0);
}

void lsl_writer_close(TBCI_LSLWriter *writer)
{
    if (!writer) return;
    if (writer->outlet) {
        lsl_destroy_outlet(writer->outlet);
        writer->outlet = NULL;
    }
    writer->connected = false;
    printf("lsl_writer_close: outlet closed\n");
}

#endif