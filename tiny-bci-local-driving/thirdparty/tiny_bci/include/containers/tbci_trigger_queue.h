/**
 * @file tbci_trigger_queue.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Typed FIFO queue for incoming trigger events.
 *
 * Provides a statically-allocated, fixed-capacity queue that stores
 * @ref tbci_trigger_t events. No dynamic memory allocation is performed —
 * all storage is caller-provided, making this suitable for bare-metal
 * embedded targets.
 *
 * ## Ownership model
 *
 * The caller is responsible for allocating:
 * - The @ref tbci_trigger_queue_t handle (stack or static)
 * - The backing array of @ref tbci_trigger_t (capacity elements)
 *
 * Example:
 * @code
 * #define TRIGGER_QUEUE_CAPACITY 32
 *
 * tbci_trigger_t        storage[TRIGGER_QUEUE_CAPACITY];
 * tbci_trigger_queue_t  queue;
 *
 * tbci_trigger_queue_init(&queue, storage, TRIGGER_QUEUE_CAPACITY);
 * @endcode
 *
 * ## Full behaviour
 *
 * Unlike the signal buffer, the trigger queue does NOT overwrite old events
 * when full. A full queue indicates a pipeline stall or bug — the push
 * operation returns @ref TBCI_ERR_FULL and the event is rejected. In a
 * healthy system the queue should never reach capacity.
 *
 * ## Thread safety
 *
 * This implementation is NOT thread-safe. The caller is responsible for
 * appropriate critical section protection around push and pop calls when
 * used across interrupt and main-loop contexts.
 */

#ifndef TBCI_TRIGGER_QUEUE_H
#define TBCI_TRIGGER_QUEUE_H

#include "../tbci_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */
typedef enum {
    TBCI_TRIGGER_DATA,    /**< Data class index 1-191. Fed to segmentation.   */
    TBCI_TRIGGER_COMMAND, /**< Command code 192-255. Handled by input node.   */
} TBCI_TriggerType;

/**
 * @brief A single trigger event with metadata.
 *
 * Represents one trigger received from the stimulus presentation layer.
 * The @p code identifies the trigger type (e.g. target vs non-target in P300,
 * or stimulus frequency in SSVEP). Interpretation of the code is
 * paradigm-specific and handled by the segmentation layer.
 */
typedef struct {
    uint64_t timestamp_us; /**< Timestamp of the trigger in microseconds. */
    uint16_t code;         /**< Paradigm-specific trigger code. */
    TBCI_TriggerType type;  /**< Set by input node after classification. */
} TBCI_Trigger;

/**
 * @brief FIFO queue handle for trigger events.
 *
 * All fields are internal. Callers should treat this as an opaque struct
 * and interact with it exclusively through the tbci_trigger_queue_* API.
 */
typedef struct {
    TBCI_Trigger *triggers; /**< Caller-provided backing array (capacity elements). */
    size_t          capacity; /**< Maximum number of triggers the queue can hold. */
    size_t          head;     /**< Write index. */
    size_t          tail;     /**< Read index. */
    bool            full;     /**< True when head has caught up with tail. */
} TBCI_TriggerQueue;

/* --------------------------------------------------------------------------
 * Lifecycle
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialise a TinyBCI trigger queue.
 *
 * Associates the queue handle with caller-provided storage and sets all
 * internal state to a clean empty condition. No memory is allocated.
 *
 * @param[out] queue     Pointer to an uninitialised queue handle. Must not be NULL.
 * @param[in]  triggers  Caller-allocated array of at least capacity elements. Must not be NULL.
 * @param[in]  capacity  Maximum number of trigger events the queue can hold. Must be > 0.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL or capacity is zero.
 */
TBCI_API TBCI_Status tq_init(TBCI_TriggerQueue *queue,
                                                TBCI_Trigger       *triggers,
                                                size_t                capacity);

/**
 * @brief Reset the queue to an empty state without releasing storage.
 *
 * Resets head, tail, and full flag. Existing trigger data in the backing
 * array is not cleared. Useful for re-using a queue across BCI trials.
 *
 * @param[in,out] queue  Pointer to an initialised queue handle. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if queue is NULL.
 */
TBCI_API TBCI_Status tq_reset(TBCI_TriggerQueue *queue);

/* --------------------------------------------------------------------------
 * Write / Read
 * -------------------------------------------------------------------------- */

/**
 * @brief Push a trigger event onto the back of the queue.
 *
 * Copies the trigger pointed to by @p trigger into the queue's backing
 * storage. If the queue is full, the event is rejected and the queue is
 * left unchanged.
 *
 * @param[in,out] queue    Pointer to an initialised queue. Must not be NULL.
 * @param[in]     trigger  Pointer to the trigger event to enqueue. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_FULL if the queue is at capacity. The trigger is not stored.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status tq_push(TBCI_TriggerQueue *queue, const TBCI_Trigger *trigger);

/**
 * @brief Pop the oldest trigger event from the front of the queue.
 *
 * Copies the oldest event into @p trigger_out and removes it from the queue.
 *
 * @param[in,out] queue        Pointer to an initialised queue. Must not be NULL.
 * @param[out]    trigger_out  Pointer to a trigger struct to receive the event. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_EMPTY if the queue contains no events.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status tq_pop(TBCI_TriggerQueue *queue, TBCI_Trigger *trigger_out);

/* --------------------------------------------------------------------------
 * Introspection
 * -------------------------------------------------------------------------- */
/**
 * @brief Peek at the oldest trigger without consuming it.
 *
 * Copies the front trigger into @p trigger_out without removing it
 * from the queue. The queue state is not modified.
 *
 * @param[in]  queue        Pointer to an initialised queue. Must not be NULL.
 * @param[out] trigger_out  Pointer to receive the trigger. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_EMPTY if the queue contains no events.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status tq_peek(const TBCI_TriggerQueue *queue, TBCI_Trigger *trigger_out);

/**
 * @brief Return the number of events currently in the queue.
 *
 * @param[in] queue  Pointer to an initialised queue. Must not be NULL.
 * @return Number of pending trigger events, or 0 if queue is NULL.
 */
TBCI_API size_t tq_size(const TBCI_TriggerQueue *queue);

/**
 * @brief Return the maximum number of events the queue can hold.
 *
 * @param[in] queue  Pointer to an initialised queue. Must not be NULL.
 * @return Capacity in events, or 0 if queue is NULL.
 */
TBCI_API size_t tq_capacity(const TBCI_TriggerQueue *queue);

/**
 * @brief Return true if the queue contains no events.
 *
 * @param[in] queue  Pointer to an initialised queue. Must not be NULL.
 * @return true if empty or queue is NULL, false otherwise.
 */
TBCI_API bool tq_is_empty(const TBCI_TriggerQueue *queue);

/**
 * @brief Return true if the queue is at full capacity.
 *
 * A full queue indicates a pipeline stall. In normal operation this
 * should never occur.
 *
 * @param[in] queue  Pointer to an initialised queue. Must not be NULL.
 * @return true if full, false otherwise or if queue is NULL.
 */
TBCI_API bool tq_is_full(const TBCI_TriggerQueue *queue);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_TRIGGER_QUEUE_H */