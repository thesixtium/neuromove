/**
 * @file tbci_signal_buffer.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 */
#include "tbci_common.h"
#include "../../include/containers/tbci_signal_buffer.h"

static size_t advance(size_t index, size_t capacity) {
    return (index + 1 == capacity) ? 0 : index + 1;
}

TBCI_Status sb_init(TBCI_SignalBuffer *buf, float *storage, uint64_t *timestamps,
                                               uint32_t *sample_indices, size_t capacity, size_t n_channels) {

    if (buf == NULL || storage == NULL || timestamps == NULL || sample_indices == NULL) {
        return TBCI_ERR_INVALID_ARG;
    }
    if (capacity == 0 || n_channels == 0) {
        return TBCI_ERR_INVALID_ARG;
    }

    buf->head           = 0;
    buf->tail           = 0;
    buf->full           = false;
    buf->overflow_count = 0;
    buf->storage = storage;
    buf->timestamps = timestamps;
    buf->sample_indices = sample_indices;
    buf->capacity = capacity;
    buf->n_channels = n_channels;

    return TBCI_OK;
}

TBCI_Status sb_reset(TBCI_SignalBuffer *buf) {
    if (buf == NULL)
    {
        return TBCI_ERR_INVALID_ARG;
    }
    buf->head           = 0;
    buf->tail           = 0;
    buf->full           = false;
    buf->overflow_count = 0;

    return TBCI_OK;
}

TBCI_Status sb_put(TBCI_SignalBuffer *buf, const float *samples, uint64_t timestamp, uint32_t sample_index) {
    if (buf == NULL || samples == NULL)
        return TBCI_ERR_INVALID_ARG;

    TBCI_Status status = TBCI_OK;

    if (sb_is_full(buf)) {
        buf->tail = advance(buf->tail, buf->capacity);
        buf->overflow_count++;
        status = TBCI_ERR_OVERFLOW;
    }

    memcpy(&buf->storage[buf->head * buf->n_channels],
           samples,
           buf->n_channels * sizeof(float));

    buf->timestamps[buf->head]     = timestamp;
    buf->sample_indices[buf->head] = sample_index;

    buf->head = advance(buf->head, buf->capacity);
    buf->full = (buf->head == buf->tail);

    return status;
}

TBCI_Status sb_pop(TBCI_SignalBuffer *buf, float *samples_out, TBCI_Frame  *frame_out) {
    if (buf == NULL || samples_out == NULL || frame_out == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (sb_is_empty(buf))
        return TBCI_ERR_EMPTY;

    memcpy(samples_out,
       &buf->storage[buf->tail * buf->n_channels],
       buf->n_channels * sizeof(float));

    frame_out->timestamp_us  = buf->timestamps[buf->tail];
    frame_out->sample_index  = buf->sample_indices[buf->tail];

    buf->tail = advance(buf->tail, buf->capacity);
    buf->full = false;

    return TBCI_OK;
}

TBCI_Status sb_find_timestamp(const TBCI_SignalBuffer* buf, uint64_t timestamp_us, size_t* frame_index,
    TBCI_MatchType* match_type) {

    if (buf == NULL || frame_index == NULL || match_type == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (sb_is_empty(buf))
        return TBCI_ERR_EMPTY;

    size_t n = sb_size(buf);

    for (size_t i = 0; i < n; i++) {
        // Compute the physical index in memory from the logical index
        size_t physical = (buf->tail + i) % buf->capacity;

        if (buf->timestamps[physical] == timestamp_us) {
            *frame_index = i;
            *match_type  = TBCI_MATCH_EXACT;
            return TBCI_OK;
        }

        if (buf->timestamps[physical] > timestamp_us) {
            *frame_index = i;
            *match_type  = TBCI_MATCH_NEAREST;
            return TBCI_OK;
        }
    }

    return TBCI_ERR_NOT_YET;
}

TBCI_Status sb_frames_available_from(const TBCI_SignalBuffer* buf, size_t frame_index, size_t* count) {

    if (buf == NULL || count == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (frame_index >= sb_size(buf))
        return TBCI_ERR_INVALID_ARG;

    if (sb_is_empty(buf))
        return TBCI_ERR_EMPTY;

    *count = sb_size(buf) - frame_index;
    return TBCI_OK;
}

TBCI_Status sb_read_from(const TBCI_SignalBuffer* buf, size_t frame_index, size_t n_frames, float* samples_out,
    TBCI_Frame* frames_out) {

    if (buf == NULL || samples_out == NULL)
        return TBCI_ERR_INVALID_ARG;
    if (frame_index > sb_size(buf))
        return TBCI_ERR_INVALID_ARG;
    if (n_frames == 0)
        return TBCI_ERR_INVALID_ARG;
    if (sb_is_empty(buf))
        return TBCI_ERR_EMPTY;

    for (size_t i = 0; i < n_frames; i++)
    {
        size_t physical = (buf->tail + frame_index + i) % buf->capacity;

        memcpy(&samples_out[i * buf->n_channels],
           &buf->storage[physical * buf->n_channels],
           buf->n_channels * sizeof(float));

        if (frames_out != NULL) {
            frames_out[i].timestamp_us = buf->timestamps[physical];
            frames_out[i].sample_index = buf->sample_indices[physical];
        }
    }

    return TBCI_OK;
}

TBCI_Status sb_peek_latest(const TBCI_SignalBuffer *buf, float *samples_out, TBCI_Frame *frame_out)
{
    if (buf == NULL || samples_out == NULL || frame_out == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (sb_is_empty(buf))
        return TBCI_ERR_EMPTY;

    /* most recent frame is at head-1, wrapping if head == 0 */
    size_t latest = (buf->head == 0) ? buf->capacity - 1 : buf->head - 1;

    memcpy(samples_out,
           &buf->storage[latest * buf->n_channels],
           buf->n_channels * sizeof(float));

    frame_out->timestamp_us  = buf->timestamps[latest];
    frame_out->sample_index  = buf->sample_indices[latest];

    return TBCI_OK;
}

TBCI_Status sb_read_since(const TBCI_SignalBuffer *buf, uint64_t since_ts, TBCI_FrameCallback callback, void *caller_node)
{
    if (buf == NULL || callback == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (sb_is_empty(buf))
        return TBCI_ERR_EMPTY;

    size_t n = sb_size(buf);

    for (size_t i = 0; i < n; i++) {
        size_t physical = (buf->tail + i) % buf->capacity;

        if (buf->timestamps[physical] <= since_ts)
            continue;

        TBCI_Frame frame = {
            .timestamp_us = buf->timestamps[physical],
            .sample_index = buf->sample_indices[physical],
        };

        callback(&buf->storage[physical * buf->n_channels], &frame, caller_node);
    }

    return TBCI_OK;
}

size_t sb_size(const TBCI_SignalBuffer *buf) {
    if (buf == NULL)
        return 0;
    if (sb_is_full(buf))
        return buf->capacity;
    if (buf->head >= buf->tail)
        return buf->head - buf->tail;
    return buf->capacity - buf->tail + buf->head;
}

size_t sb_capacity(const TBCI_SignalBuffer *buf) {
    if (buf == NULL)
        return 0;
    return buf->capacity;
}

uint32_t sb_overflow_count(const TBCI_SignalBuffer *buf) {
    if (buf == NULL)
        return 0;
    return buf->overflow_count;
}


bool sb_is_full(const TBCI_SignalBuffer *buf) {
    if (buf == NULL)
        return false;
    return buf->full;
}

bool sb_is_empty(const TBCI_SignalBuffer *buf) {
    if (buf == NULL)
        return true;
    return !buf->full && (buf->head == buf->tail);
}
