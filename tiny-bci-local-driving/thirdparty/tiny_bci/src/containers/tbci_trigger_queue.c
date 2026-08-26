/**
 * @file tbci_trigger_queue.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 */
#include "tbci_common.h"
#include "../../include/containers/tbci_trigger_queue.h"

static size_t advance(size_t index, size_t capacity) {
    return (index + 1 == capacity) ? 0 : index + 1;
}

TBCI_Status tq_init(TBCI_TriggerQueue* queue, TBCI_Trigger* triggers, size_t capacity) {
    if (queue == NULL || triggers == NULL || capacity == 0)
    {
        return TBCI_ERR_INVALID_ARG;
    }

    queue->head = 0;
    queue->tail = 0;
    queue->full = false;
    queue->triggers = triggers;
    queue->capacity = capacity;

    return TBCI_OK;
}

TBCI_Status tq_reset(TBCI_TriggerQueue* queue) {
    if (queue == NULL)
        return TBCI_ERR_INVALID_ARG;
    queue->head = 0;
    queue->tail = 0;
    queue->full = false;

    return TBCI_OK;
}

TBCI_Status tq_push(TBCI_TriggerQueue* queue, const TBCI_Trigger* trigger) {
    if (queue == NULL || trigger == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (tq_is_full(queue))
        return TBCI_ERR_FULL;

    queue->triggers[queue->head] = *trigger;
    queue->head = advance(queue->head, queue->capacity);
    queue->full = queue->head == queue->tail;

    return TBCI_OK;
}

TBCI_Status tq_pop(TBCI_TriggerQueue* queue, TBCI_Trigger* trigger_out) {
    if (queue == NULL || trigger_out == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (tq_is_empty(queue))
        return TBCI_ERR_EMPTY;

    *trigger_out = queue->triggers[queue->tail];
    queue->tail = advance(queue->tail, queue->capacity);
    queue->full = false;

    return TBCI_OK;
}

TBCI_Status tq_peek(const TBCI_TriggerQueue *queue, TBCI_Trigger *trigger_out)
{
    if (queue == NULL || trigger_out == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (tq_is_empty(queue))
        return TBCI_ERR_EMPTY;

    *trigger_out = queue->triggers[queue->tail];
    return TBCI_OK;
}

size_t tq_size(const TBCI_TriggerQueue* queue)
{
    if (queue == NULL)
        return 0;
    if (tq_is_full(queue))
        return queue->capacity;
    if (queue->head >= queue->tail)
        return queue->head - queue->tail;
    return queue->capacity - queue->tail + queue->head;
}

size_t tq_capacity(const TBCI_TriggerQueue* queue)
{
    if (queue == NULL)
        return 0;
    return queue->capacity;
}

bool tq_is_empty(const TBCI_TriggerQueue* queue)
{
    if (queue == NULL)
        return true;
    return !queue->full && (queue->head == queue->tail);
}

bool tq_is_full(const TBCI_TriggerQueue* queue)
{
    if (queue == NULL)
        return false;
    return queue->full;
}
