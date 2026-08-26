/**
 * @file tbci_raw_out.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Optional raw EEG logger node, sits inside CoreNode between SyncNode and SegNode.
 *
 * Writes one CSV row per signal frame for the entire session. The trigger
 * column is non-zero only on frames where a trigger was received this tick.
 * Commands are written optionally based on config.
 *
 * ## File format
 *
 * ```
 * sample_idx,timestamp_us,trigger_val,ch0,ch1,...,chN
 * 0,1000000,0,0.12,0.34,...
 * 1,1003906,1,0.11,0.33,...
 * ```
 *
 * ## File naming
 *
 * tbci_out_<subject>_<session>_<unix_timestamp>.csv
 *
 * ## Lifecycle
 *
 * - File opens at ro_init
 * - Header written on first row
 * - One row appended per frame via ro_write
 * - File flushed and closed at ro_close, called by tbci_context_stop
 *
 * ## Usage
 *
 * @code
 * TBCI_RawOutConfig cfg = { .subject = "S01", .session = "001", .log_commands = false };
 * TBCI_RawOutNode raw_out;
 * ro_init(&raw_out, &cfg, &ctx);
 * ctx.core_node.raw_out = &raw_out;
 * @endcode
 */

#ifndef TBCI_RAW_OUT_H
#define TBCI_RAW_OUT_H

#include <stdio.h>
#include "tbci_common.h"
#include "../tbci_node.h"
#include "tbci_sync_node.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*TBCI_RawOutFn)(const float *samples, size_t n_samples, const TBCI_Frame *frame, uint16_t trigger_val, void *user_data);

/**
 * @brief Raw EEG logger node.
 *
 * Owned by TBCI_Core. Caller initialises and passes a pointer to
 * ctx.core_node.raw_out — NULL means disabled.
 */
typedef struct {
    TBCI_Node         base;            /**< Node base. MUST be first member.              */
    TBCI_CoreConfig   config;          /**< Copy of caller config.                        */
    FILE             *file;            /**< Open CSV file handle. NULL if not open.       */
    uint64_t          sample_index;    /**< Global frame counter, increments each write.  */
    bool              header_written;  /**< True after first row written.                 */
    char              filepath[128];   /**< Full path of the output file.                 */
    TBCI_RawOutFn     on_frame;        /**< optional custom output callback               */
    uint64_t          last_written_ts; /**< Timestamp of last written frame               */
    void              *user_data;      /**< passed through to on_frame                    */
} TBCI_RawOutNode;

/**
 * @brief Per-tick context bundled for the sb_read_since callback.
 *
 * Stack-allocated in ro_write for the duration of one pipeline tick.
 * Bundles all state needed by ro_write_cb to log and tap each frame
 * without requiring global state or extra fields on TBCI_RawOutNode.
 *
 * trigger_val and sync_result are tick-level — shared across all frames
 * visited in one sb_read_since call. ro_write_cb is responsible for
 * attaching the trigger only to the frame whose timestamp matches the
 * trigger event, leaving all other frames with trigger_val = 0.
 */
typedef struct {
    TBCI_RawOutNode       *node;         /**< Raw output node — owns file, tap callback, counters.  */
    size_t                 n_channels;   /**< Number of EEG channels, from ctx->config.             */
    uint16_t               trigger_val;  /**< Trigger code for this tick, 0 if none.                */
    const TBCI_SyncResult *sync_result;  /**< Sync result for this tick, used to match trigger
                                              timestamp to the correct frame.                       */
    bool                   trigger_fired;/**< ensures trigger attached to exactly one frame         */

} RoWriteCtx;


/**
 * @brief Initialise the raw output node and open the CSV file.
 *
 * Generates filename from subject, session, and current Unix timestamp.
 * Writes CSV header on first call to ro_write.
 *
 * @param[out] node   Pointer to an uninitialised node. Must not be NULL.
 * @param[in]  config Node configuration. Must not be NULL.
 * @param[in]  ctx    Pipeline context, used for n_channels. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 * @return TBCI_ERR_INVALID_STATE if file could not be opened.
 */
TBCI_API TBCI_Status ro_init(TBCI_RawOutNode *node, TBCI_CoreConfig *config, struct TBCI_Context *ctx);

/**
 * @brief Write one row to the CSV file for the current frame.
 *
 * Reads the latest frame from ctx->inputs->signal. Writes trigger_val
 * from sync_result if a trigger fired this tick, 0 otherwise.
 * Commands are written only if config.log_commands is true.
 *
 * @param[in,out] node         Pointer to an initialised node. Must not be NULL.
 * @param[in]     sync_result  Result from sync_process this tick. Must not be NULL.
 * @param[in]     ctx          Pipeline context. Must not be NULL.
 * @return TBCI_NODE_OK on success.
 * @return TBCI_NODE_ERROR if node or ctx is NULL or file is not open.
 */
TBCI_API TBCI_NodeResult ro_write(TBCI_RawOutNode *node, const TBCI_SyncResult *sync_result,
                                   struct TBCI_Context *ctx);

/**
 * @brief Flush and close the CSV file.
 *
 * Called automatically by tbci_context_stop. Safe to call if file is
 * already closed.
 *
 * @param[in,out] node  Pointer to an initialised node. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if node is NULL.
 */
TBCI_API TBCI_Status ro_close(TBCI_RawOutNode *node);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_RAW_OUT_H */