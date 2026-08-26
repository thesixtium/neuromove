/**
 * @file tbci_input.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Shared input context passed to all pipeline nodes.
 *
 * Owns pointers to the two input streams that feed the pipeline — the
 * continuous signal buffer and the trigger event queue. Both are written
 * by the producer (e.g. the LSL reader) and read by pipeline nodes.
 * No node owns these buffers; they are created by the application and
 * passed in at pipeline initialisation time.
 *
 * The @p n_channels field is set by the producer once the stream is
 * connected and the channel count is known. This may come from an LSL
 * stream, a hardware driver, a configuration file, or a synthetic
 * generator — the pipeline does not care about the source. All downstream
 * components that need channel count read it from here rather than storing
 * their own copy, ensuring a single source of truth.
 *
 * On embedded targets this struct is typically allocated statically and
 * lives for the duration of the program. On desktop it is owned by the
 * top-level TBCIContext.
 */

#ifndef TINY_BCI_TBCI_INPUT_H
#define TINY_BCI_TBCI_INPUT_H

#include "../containers/tbci_signal_buffer.h"
#include "../containers/tbci_trigger_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TBCI_Context;


typedef struct {
    TBCI_SignalBuffer *signal;    /**< Continuous signal stream. Written by the producer.               */
    TBCI_TriggerQueue *triggers;  /**< Trigger event queue. Written by the producer.                    */
    size_t            n_channels;/**< Number of signal channels. Set by the producer at connect time.  */
} TBCI_Input;

TBCI_API TBCI_Status in_push_signal(TBCI_Input *input, float *samples, uint64_t timestamp_us, uint32_t index);

TBCI_API TBCI_Status in_push_trigger(TBCI_Input *input, TBCI_Trigger *trigger, struct TBCI_Context *ctx);

#ifdef __cplusplus
}
#endif
#endif //TINY_BCI_TBCI_INPUT_H