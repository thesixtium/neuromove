/**
 * @file tbci_signal_buffer.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Typed circular buffer for multichannel signal sample frames.
 *
 * Provides a statically-allocated, fixed-capacity ring buffer that stores
 * multichannel signal samples as @ref tbci_frame_t frames. No dynamic
 * memory allocation is performed — all storage is caller-provided, making
 * this suitable for bare-metal embedded targets.
 *
 * ## Ownership model
 *
 * The caller is responsible for allocating:
 * - The @ref tbci_signal_buffer_t handle (stack or static)
 * - The backing float array (capacity × n_channels floats)
 *
 * Example:
 * @code
 * float    storage[BUF_CAPACITY * N_CHANNELS];
 * uint64_t timestamps[BUF_CAPACITY];
 * uint32_t sample_indices[BUF_CAPACITY];
 * tbci_signal_buffer_t buf;

 * tbci_signal_buffer_init(&buf, storage, timestamps, sample_indices, BUF_CAPACITY, N_CHANNELS);
 * @endcode
 *
 * ## Overflow behaviour
 *
 * When the buffer is full, @ref tbci_signal_buffer_put overwrites the oldest
 * frame and increments the @ref tbci_signal_buffer_t::overflow_count counter.
 * This ensures the pipeline always holds the most recent data. Callers can
 * monitor overflow_count to detect sustained backpressure.
 *
 * ## Thread safety
 *
 * This implementation is NOT thread-safe. On bare-metal targets with a
 * single interrupt-driven producer and a single consumer in the main loop,
 * the caller is responsible for appropriate critical section protection
 * around put and get calls.
 */

#ifndef TBCI_SIGNAL_BUFFER_H
#define TBCI_SIGNAL_BUFFER_H

#include "../tbci_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */

/**
 * @brief A single multichannel signal sample with metadata.
 *
 * Represents one time point in the signal stream. The @p samples pointer
 * refers to @p n_channels consecutive floats owned by the buffer's backing
 * storage — callers must not hold this pointer across subsequent put calls,
 * as the underlying memory may be overwritten.
 */
typedef struct {
    uint64_t  timestamp_us; /**< Acquisition timestamp in microseconds. */
    uint32_t  sample_index; /**< Monotonic sample counter. Gaps indicate upstream loss. */
} TBCI_Frame;

/**
 * @brief Circular buffer handle for multichannel signal frames.
 *
 * All fields are internal. Callers should treat this as an opaque struct
 * and interact with it exclusively through the tbci_signal_buffer_* API.
 *
 * @note Declare this on the stack or as a static variable. Do not heap-
 *       allocate unless necessary — this struct contains no pointers that
 *       require deep copies.
 */
typedef struct {
    float   *storage;        /**< Caller-provided backing array (capacity * n_channels floats). */
    uint64_t *timestamps;    /**< Caller-provided timestamps array (capacity unsigned int64). */
    uint32_t *sample_indices;/**< Caller-provided indices array (capacity unsigned int32). */
    size_t   capacity;       /**< Maximum number of frames the buffer can hold. */
    size_t   n_channels;     /**< Number of signal channels per frame. */
    size_t   head;           /**< Write index (next frame will be written here). */
    size_t   tail;           /**< Read index (next frame will be read from here). */
    bool     full;           /**< True when head has caught up with tail. */
    uint32_t overflow_count;/**< Number of frames overwritten due to buffer being full. */
} TBCI_SignalBuffer;

/* --------------------------------------------------------------------------
 * Lifecycle
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialise a TinyBCI signal circular buffer.
 *
 * Associates the buffer handle with caller-provided storage and sets all
 * internal state to a clean empty condition. No memory is allocated.
 *
 * @param[out] buf        Pointer to an uninitialised buffer handle. Must not be NULL.
 * @param[in]  storage    Caller-allocated float array of at least capacity * n_channels elements.
 * @param[in]  timestamps Caller-allocated int array of at least capacity elements
 * @param[in]  sample_indices Caller-allocated int array of at least capacity elements
 * @param[in]  capacity   Maximum number of multichannel frames the buffer can hold. Must be > 0.
 * @param[in]  n_channels Number of signal channels per frame. Must be > 0.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL or any size parameter is zero.
 */
TBCI_API TBCI_Status sb_init(TBCI_SignalBuffer *buf,
                                    float              *storage,
                                    uint64_t           *timestamps,
                                    uint32_t           *sample_indices,
                                    size_t             capacity,
                                    size_t             n_channels);

/**
 * @brief Reset the buffer to an empty state without releasing storage.
 *
 * Resets head, tail, full flag, and overflow counter. The backing storage
 * is not zeroed. Useful for re-using a buffer across BCI trials or states.
 *
 * @param[in,out] buf  Pointer to an initialised buffer handle. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if buf is NULL.
 */
TBCI_API TBCI_Status sb_reset(TBCI_SignalBuffer *buf);

/* --------------------------------------------------------------------------
 * Write / Read
 * -------------------------------------------------------------------------- */

/**
 * @brief Write one multichannel frame into the buffer.
 *
 * Copies n_channels floats from @p samples into the buffer's backing storage,
 * storing @p timestamp and @p sample_index as frame metadata.
 *
 * If the buffer is full, the oldest frame is silently overwritten and
 * @ref tbci_signal_buffer_t::overflow_count is incremented.
 *
 * @param[in,out] buf           Pointer to an initialised buffer. Must not be NULL.
 * @param[in]     samples       Array of at least n_channels floats. Must not be NULL.
 * @param[in]     timestamp  Acquisition timestamp in microseconds.
 * @param[in]     sample_index  Monotonic index assigned by the acquisition layer.
 * @return TBCI_OK if the frame was written without overwriting existing data.
 * @return TBCI_ERR_OVERFLOW if the buffer was full and an old frame was overwritten.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status sb_put(TBCI_SignalBuffer *buf, const float *samples, uint64_t timestamp, uint32_t sample_index);

/**
 * @brief Read and remove the oldest frame from the buffer.
 *
 * Copies n_channels floats into @p samples_out and populates @p frame_out
 * with the frame metadata. The frame is consumed and the slot is freed.
 *
 * @param[in,out] buf          Pointer to an initialised buffer. Must not be NULL.
 * @param[out]    samples_out  Caller-allocated array of at least n_channels floats. Must not be NULL.
 * @param[out]    frame_out    Pointer to a frame struct to receive metadata. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_EMPTY if the buffer contains no frames.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status sb_pop(TBCI_SignalBuffer *buf, float *samples_out, TBCI_Frame  *frame_out);

/* --------------------------------------------------------------------------
 * Non-consuming reads
 * -------------------------------------------------------------------------- */

/**
 * @brief Search for a frame by timestamp using a single linear scan.
 *
 * Scans from tail to head in chronological order. Stops early on exact
 * match or on the first frame strictly after the target (nearest-after).
 * If the target timestamp is ahead of all frames currently in the buffer,
 * returns TBCI_MATCH_NOT_YET and the caller should retry on the next tick.
 *
 * @param[in]  buf           Pointer to an initialised buffer. Must not be NULL.
 * @param[in]  timestamp_us  Timestamp to search for in microseconds.
 * @param[out] frame_index   Logical index of the matched frame. Must not be NULL.
 * @param[out] match_type    How the match was resolved. Must not be NULL.
 * @return TBCI_OK on success (exact or nearest).
 * @return TBCI_ERR_NOT_YET if target is ahead of all buffered frames.
 * @return TBCI_ERR_EMPTY if the buffer contains no frames.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status sb_find_timestamp(const TBCI_SignalBuffer *buf,
                                       uint64_t               timestamp_us,
                                       size_t                *frame_index,
                                       TBCI_MatchType         *match_type);

/**
 * @brief Count frames available from a given logical frame index onwards.
 *
 * Used by the segmentation node to check whether enough post-stimulus
 * data has accumulated before extracting an epoch. Call after
 * sb_find_timestamp to check readiness.
 *
 * @param[in]  buf          Pointer to an initialised buffer. Must not be NULL.
 * @param[in]  frame_index  Logical frame index returned by sb_find_timestamp.
 * @param[out] count        Number of frames from frame_index to head. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status sb_frames_available_from(const TBCI_SignalBuffer *buf,
                                              size_t                 frame_index,
                                              size_t                *count);

/**
 * @brief Copy n_frames starting from a logical frame index without consuming them.
 *
 * Copies data in time-major order (samples[frame * n_channels + channel]).
 * The segmentation node is responsible for transposing to channel-major
 * when building a TBCIEpoch.
 * Optionally copies per-frame metadata into @p frames_out if non-NULL. The buffer
 * state is not modified — head, tail, and full are unchanged.
 *
 * Intended to be called by the segmentation node after sb_find_timestamp
 * and sb_frames_available_from confirm the window is ready.
 *
 * @param[in]  buf          Pointer to an initialised buffer. Must not be NULL.
 * @param[in]  frame_index  Logical frame index to start reading from.
 * @param[in]  n_frames     Number of frames to copy. Must be > 0.
 * @param[out] samples_out  Caller-allocated array of n_frames * n_channels floats. Must not be NULL.
 * @param[out] frames_out   Caller-allocated array of n_frames TBCIFrame structs, or NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL or n_frames is zero.
 * @return TBCI_ERR_EMPTY if the buffer contains no frames.
 */
TBCI_API TBCI_Status sb_read_from(const TBCI_SignalBuffer *buf,
                                  size_t                 frame_index,
                                  size_t                 n_frames,
                                  float                 *samples_out,
                                  TBCI_Frame             *frames_out);
/* --------------------------------------------------------------------------
 * Introspection
 * -------------------------------------------------------------------------- */
/**
 * @brief Callback invoked by sb_read_since for each matching frame.
 *
 * @param[in] samples    Pointer to n_channels floats for this frame. Read-only.
 * @param[in] frame      Frame metadata (timestamp_us, sample_index). Read-only.
 * @param[in] user_data  Caller-provided context passed through from sb_read_since.
 */
typedef void (*TBCI_FrameCallback)(const float *samples, const TBCI_Frame *frame, void *user_data);

/**
 * @brief Iterate all frames in the buffer with timestamp strictly greater than since_ts.
 *
 * Calls callback once per matching frame in chronological order (oldest first).
 * Frames with timestamp_us <= since_ts are skipped — this allows the caller
 * to track the last processed timestamp and resume from there on the next call.
 *
 * Designed for continuous logging and tap scenarios where multiple frames
 * may accumulate between processing ticks and all must be visited without gaps.
 *
 * ## Usage
 *
 * @code
 * static void my_callback(const float *samples, const TBCI_Frame *frame, void *user_data)
 * {
 *     MyContext *ctx = (MyContext *)user_data;
 *     // process samples and frame metadata
 * }
 *
 * uint64_t last_ts = 0;
 * sb_read_since(&signal_buf, last_ts, my_callback, &my_ctx);
 * // update last_ts to latest frame timestamp after the call
 * @endcode
 *
 * @param[in] buf        Signal buffer to read from. Must not be NULL.
 * @param[in] since_ts   Only frames with timestamp_us strictly greater than
 *                       this value are visited. Pass 0 to visit all frames.
 * @param[in] callback   Called once per matching frame. Must not be NULL.
 * @param[in] caller_node  Passed through to callback unchanged. May be NULL.
 * @return TBCI_OK on success (even if no frames matched).
 * @return TBCI_ERR_INVALID_ARG if buf or callback is NULL.
 * @return TBCI_ERR_EMPTY if the buffer contains no frames.
 */
TBCI_Status sb_read_since(const TBCI_SignalBuffer *buf, uint64_t since_ts, TBCI_FrameCallback callback, void *caller_node);

/**
 * @brief Read the most recently written frame without consuming it.
 *
 * Copies n_channels floats from the frame at head-1 into @p samples_out
 * and populates @p frame_out with its metadata. The buffer state is not
 * modified — head, tail, and full are unchanged.
 *
 * Used by PreprocessingGroup to read the newest raw frame each tick
 * before writing the (possibly filtered) result to processed_buf.
 *
 * @param[in]  buf          Pointer to an initialised buffer. Must not be NULL.
 * @param[out] samples_out  Caller-allocated array of n_channels floats. Must not be NULL.
 * @param[out] frame_out    Pointer to a frame struct to receive metadata. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_EMPTY if the buffer contains no frames.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status sb_peek_latest(const TBCI_SignalBuffer *buf, float *samples_out, TBCI_Frame *frame_out);

/**
 * @brief Return the number of frames currently stored in the buffer.
 *
 * @param[in] buf  Pointer to an initialised buffer. Must not be NULL.
 * @return Number of readable frames, or 0 if buf is NULL.
 */
TBCI_API size_t sb_size(const TBCI_SignalBuffer *buf);

/**
 * @brief Return the maximum number of frames the buffer can hold.
 *
 * @param[in] buf  Pointer to an initialised buffer. Must not be NULL.
 * @return Capacity in frames, or 0 if buf is NULL.
 */
TBCI_API size_t sb_capacity(const TBCI_SignalBuffer *buf);

/**
 * @brief Return true if the buffer contains no frames.
 *
 * @param[in] buf  Pointer to an initialised buffer. Must not be NULL.
 * @return true if empty or buf is NULL, false otherwise.
 */
TBCI_API bool sb_is_empty(const TBCI_SignalBuffer *buf);

/**
 * @brief Return true if the buffer is at full capacity.
 *
 * @param[in] buf  Pointer to an initialised buffer. Must not be NULL.
 * @return true if full, false otherwise or if buf is NULL.
 */
TBCI_API bool sb_is_full(const TBCI_SignalBuffer *buf);

/**
 * @brief Return the number of frames overwritten since last reset.
 *
 * A non-zero value indicates the consumer is not keeping up with the
 * producer. Monitor this value to detect sustained backpressure.
 *
 * @param[in] buf  Pointer to an initialised buffer. Must not be NULL.
 * @return Overflow count, or 0 if buf is NULL.
 */
TBCI_API uint32_t sb_overflow_count(const TBCI_SignalBuffer *buf);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_SIGNAL_BUFFER_H */