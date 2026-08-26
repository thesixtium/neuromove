/**
 * @file tbci_epoch_queue.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief A fixed-capacity FIFO queue of segmented signal epochs.
 *
 * Serves as the handoff point between the segmentation node and the
 * decoder node. On single-threaded embedded targets it acts as a
 * small buffer between the two stages. On more capable systems
 * (Raspberry Pi, desktop) it enables segmentation and classification
 * to run concurrently on separate threads.
 *
 * ## Initialisation and configuration
 *
 * Initialisation is a two-step process to handle the case where the
 * channel count is not known until the input stream connects:
 *
 * @code
 * // Step 1 — at startup, channel count unknown
 * TBCIEpoch epochs[EPOCH_CAPACITY];
 * TBCIEpochQueue queue;
 * eq_init(&queue, epochs, pool, EPOCH_CAPACITY, EPOCH_N_FRAMES);
 *
 * // Step 2 — after stream connects and TBCIInputs.n_channels is set
 * float pool[EPOCH_CAPACITY * EPOCH_N_FRAMES * inputs.n_channels];
 * eq_configure(&queue, pool, inputs.n_channels);
 * @endcode
 *
 * Attempting to push before eq_configure returns TBCI_ERR_INVALID_STATE.
 *
 * ## Memory layout
 *
 * The sample pool is divided into capacity equal slots, each holding
 * n_frames * n_channels floats. When an epoch is pushed, eq_push copies
 * the caller's sample data into the next available slot and sets the
 * stored epoch's samples pointer accordingly. The caller does not manage
 * pool memory directly.
 *
 * ## Overflow behaviour
 *
 * The queue rejects new epochs when full, returning TBCI_ERR_FULL.
 * A full queue indicates the decoder is not keeping up with
 * segmentation. In a healthy system this should never occur.
 *
 * ## Thread safety
 *
 * This implementation is NOT thread-safe. On multi-threaded targets
 * the caller is responsible for protecting push and pop calls with
 * appropriate synchronization primitives.
 */

#ifndef TINY_BCI_TBCI_EPOCH_QUEUE_H
#define TINY_BCI_TBCI_EPOCH_QUEUE_H

#include "../tbci_common.h"

#ifdef __cplusplus
extern "C" {
#endif
/* --------------------------------------------------------------------------
 * Epoch queue
 * -------------------------------------------------------------------------- */


typedef struct {
    TBCI_Epoch *epochs;     /**< Caller-provided array of capacity TBCIEpoch structs.              */
    float     *sample_pool; /**< Caller-provided flat float array. Set by eq_configure.             */
    size_t     capacity;    /**< Maximum number of epochs the queue can hold.                       */
    size_t     n_frames;    /**< Number of frames per epoch. Fixed at init time.                    */
    size_t     n_channels;  /**< Channels per epoch. Set by eq_configure. 0 until configured.      */
    size_t     head;        /**< Write index.                                                       */
    size_t     tail;        /**< Read index.                                                        */
    bool       full;        /**< True when head has caught up with tail.                            */
} TBCI_EpochQueue;

/* Lifecycle */
TBCI_API TBCI_Status eq_init(TBCI_EpochQueue *queue, TBCI_Epoch *epochs, size_t capacity, size_t n_frames);

TBCI_API TBCI_Status eq_configure(TBCI_EpochQueue *queue, float *sample_pool, size_t n_channels);

TBCI_API TBCI_Status eq_reset(TBCI_EpochQueue *queue);

/* Write / Read */
TBCI_API TBCI_Status eq_push(TBCI_EpochQueue  *queue, const TBCI_Epoch *epoch);
TBCI_API TBCI_Status eq_pop(TBCI_EpochQueue   *queue, TBCI_Epoch       *epoch_out);

/**
 * @brief Return a pointer to the next available sample slot in the pool.
 *
 * Returns a pointer to the float storage that eq_push would write into
 * for the current head position. Allows the caller to write sample data
 * directly into the pool without an intermediate copy.
 *
 * ## Usage
 *
 * @code
 * float *slot = eq_next_slot(&epoch_queue);
 * sb_read_from(&signal_buf, read_start, n_frames, slot, NULL);
 * epoch.samples = slot;
 * eq_push(&epoch_queue, &epoch);  // no copy — samples already in pool
 * @endcode
 *
 * @warning The pointer is only valid until the next eq_push call.
 *          Do not hold it across push operations.
 * @warning Returns NULL if the queue is full or unconfigured.
 *
 * @param[in] queue  Pointer to an initialised and configured queue. Must not be NULL.
 * @return Pointer to the next available slot, or NULL if full/unconfigured/NULL input.
 */
TBCI_API float *eq_next_slot(const TBCI_EpochQueue *queue);

/* Introspection */
TBCI_API size_t     eq_size(const TBCI_EpochQueue *queue);
TBCI_API size_t     eq_capacity(const TBCI_EpochQueue *queue);
TBCI_API bool       eq_is_empty(const TBCI_EpochQueue *queue);
TBCI_API bool       eq_is_full(const TBCI_EpochQueue *queue);

#ifdef __cplusplus
}
#endif
#endif //TINY_BCI_TBCI_EPOCH_QUEUE_H