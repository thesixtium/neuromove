/**
 * @file tbci_epoch_queue.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 */

#include "tbci_common.h"
#include "../../include/containers/tbci_epoch_queue.h"

static size_t advance(size_t index, size_t capacity) {
    return (index + 1 == capacity) ? 0 : index + 1;
}

TBCI_Status eq_init(TBCI_EpochQueue* queue, TBCI_Epoch* epochs, size_t capacity, size_t n_frames) {
    if (queue == NULL || epochs == NULL || capacity == 0 || n_frames == 0)
        return TBCI_ERR_INVALID_ARG;

    queue->head = 0;
    queue->tail = 0;
    queue->full = false;
    queue->epochs = epochs;
    queue->capacity = capacity;
    queue->n_frames = n_frames;

    return TBCI_OK;
}

TBCI_Status eq_configure(TBCI_EpochQueue* queue, float* sample_pool, size_t n_channels) {

    if (queue == NULL || sample_pool == NULL || n_channels == 0)
        return TBCI_ERR_INVALID_ARG;

    queue->sample_pool = sample_pool;
    queue->n_channels = n_channels;

    return TBCI_OK;
}

TBCI_Status eq_reset(TBCI_EpochQueue* queue) {
    if (queue == NULL)
        return TBCI_ERR_INVALID_ARG;
    queue->head = 0;
    queue->tail = 0;
    queue->full = false;

    return TBCI_OK;
}

TBCI_Status eq_push(TBCI_EpochQueue* queue, const TBCI_Epoch* epoch) {
    if (queue == NULL || epoch == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (queue->n_channels == 0 || queue->sample_pool == NULL)
        return TBCI_ERR_INVALID_STATE;

    if (eq_is_full(queue))
        return TBCI_ERR_FULL;

    // compute the slot in the pool for this head position
    float *slot = &queue->sample_pool[queue->head * queue->n_frames * queue->n_channels];

    // copy sample data into the pool
    if (epoch->samples != NULL && epoch->samples != slot) {
        // samples are external — copy into pool
        memcpy(slot, epoch->samples, epoch->n_frames * epoch->n_channels * sizeof(float));
    }
    queue->epochs[queue->head] = *epoch;
    queue->epochs[queue->head].samples = slot;
    queue->head = advance(queue->head, queue->capacity);
    queue->full = queue->head == queue->tail;

    return TBCI_OK;
}

TBCI_Status eq_pop(TBCI_EpochQueue* queue, TBCI_Epoch* epoch_out) {
    if (queue == NULL || epoch_out == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (eq_is_empty(queue))
        return TBCI_ERR_EMPTY;

    *epoch_out = queue->epochs[queue->tail];
    queue->tail = advance(queue->tail, queue->capacity);
    queue->full = false;

    return TBCI_OK;
}

float *eq_next_slot(const TBCI_EpochQueue *queue) {
    if (queue == NULL || queue->sample_pool == NULL || queue->n_channels == 0)
        return NULL;

    if (eq_is_full(queue))
        return NULL;

    return &queue->sample_pool[queue->head * queue->n_frames * queue->n_channels];
}

size_t eq_size(const TBCI_EpochQueue* queue) {
    if (queue == NULL)
        return 0;
    if (eq_is_full(queue))
        return queue->capacity;
    if (queue->head >= queue->tail)
        return queue->head - queue->tail;
    return queue->capacity - queue->tail + queue->head;
}

size_t eq_capacity(const TBCI_EpochQueue* queue) {
    if (queue == NULL)
        return 0;
    return queue->capacity;
}

bool eq_is_empty(const TBCI_EpochQueue* queue) {
    if (queue == NULL)
        return true;
    return !queue->full && (queue->head == queue->tail);
}

bool eq_is_full(const TBCI_EpochQueue* queue) {
    if (queue == NULL)
        return false;
    return queue->full;
}
