#ifndef TBCI_LSL_READER_H
#define TBCI_LSL_READER_H

#ifdef TBCI_WITH_LSL

#include <lsl_c.h>
#include "../ioutils/tbci_input.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    LSL_RESOLVE_BY_NAME,
    LSL_RESOLVE_BY_TYPE,
} LSLResolveMode;

/**
 * TBCILSLMarker: One marker event from the LSL marker stream.
 *
 * @timestamp  LSL network timestamp (seconds)
 * @label      Null-terminated marker string
 */
typedef struct {
    double timestamp;
    char   label[64];
} TBCILSLMarker;

/**
 * TBCI_LSLContext: State for two LSL streams (EEG data + markers).
 *
 * @data_inlet      LSL inlet for EEG data stream
 * @marker_inlet    LSL inlet for marker stream
 * @sample_buf      Caller-provided scratch buffer for one EEG frame (n_channels floats)
 * @max_channels    Size of sample_buf in number of floats (caller-declared capacity)
 * @n_channels      Number of channels discovered from stream metadata at connect time
 * @new_data        Flag set when an unread EEG sample is available
 * @new_marker      Flag set when an unread marker is available
 * @latest_ts       Timestamp of the most recent EEG sample (LSL seconds)
 * @latest_marker   Most recent marker event
 * @connected       True if both streams are connected
 */
typedef struct {
    lsl_inlet   data_inlet;
    lsl_inlet   marker_inlet;
    bool        connected;
    int         n_channels;
    uint32_t    sample_index;
    float      *temp_buf;
    double      start_timestamp;  /* LSL timestamp of first sample, for relative timing */
    TBCI_Input *inputs;
} TBCI_LSLContext;

/*  PUBLIC API  */
/**
 * @brief Initialise an LSL context for EEG data only.
 *
 * Resolves and opens the named data stream. Discovered channel count
 * is stored in ctx->n_channels — caller should propagate this to
 * TBCI_Input.n_channels after connect.
 *
 * @param[out] ctx           Pointer to an uninitialised LSL context. Must not be NULL.
 * @param[in]  data_name     Name of the LSL EEG stream to resolve. Must not be NULL.
 * @param[in]  temp_buf      Caller-allocated float array for raw sample staging.
 *                           Must hold at least max_channels floats. Must not be NULL.
 * @param[in]  max_channels  Capacity of temp_buf. Fails if stream has more channels.
 * @param[in]  resolve      Modality of resolving the LSL stream (name | type).
 * @return true on success, false if stream resolution or inlet creation failed.
 */
bool lsl_init_data(TBCI_LSLContext *ctx, const char *data_name, float *temp_buf, int max_channels, LSLResolveMode resolve);

/**
 * @brief Initialise an LSL context for markers only.
 *
 * Resolves and opens the named marker stream. No temp_buf needed
 * since markers are string-typed and handled internally.
 *
 * @param[out] ctx          Pointer to an uninitialised LSL context. Must not be NULL.
 * @param[in]  marker_stream  Name of the LSL marker stream to resolve. Must not be NULL.
 * @param[in]  resolve      Modality of resolving the LSL stream (name | type).
 * @return true on success, false if stream resolution or inlet creation failed.
 */
bool lsl_init_markers(TBCI_LSLContext *ctx, const char *marker_stream, LSLResolveMode resolve);

/**
 * @brief Initialise an LSL context for both EEG data and markers.
 *
 * Resolves and opens both streams. Equivalent to lsl_init_data followed
 * by marker stream resolution, but rolls back cleanly if either step fails.
 *
 * @param[out] ctx           Pointer to an uninitialised LSL context. Must not be NULL.
 * @param[in]  data_name     Name of the LSL EEG stream to resolve. Must not be NULL.
 * @param[in]  marker_name   Name of the LSL marker stream to resolve. Must not be NULL.
 * @param[in]  temp_buf      Caller-allocated float array for raw sample staging.
 *                           Must hold at least max_channels floats. Must not be NULL.
 * @param[in]  max_channels  Capacity of temp_buf. Fails if stream has more channels.
 * @param[in]  resolve      Modality of resolving the LSL stream (name | type).
 * @return true on success, false if either stream resolution or inlet creation failed.
 */
bool lsl_init_all(TBCI_LSLContext *ctx, const char *data_name, const char *marker_name, float *temp_buf, int max_channels, LSLResolveMode resolve);

/**
 * lsl_close(): Destroy LSL inlets and mark context as disconnected.
 *
 * @param ctx  Pointer to an initialised TBCILSLContext.
 */
void lsl_close( TBCI_LSLContext *ctx );

/**
 * lsl_update(): Poll both LSL inlets once (non-blocking).
 *
 * @param ctx  Pointer to an initialised TBCILSLContext.
 * @return true if a new EEG sample was received this call, false otherwise.
 */
bool lsl_update( TBCI_LSLContext *ctx );

#ifdef __cplusplus
}
#endif

#endif /* TBCI_WITH_LSL */

#endif /* TBCI_LSL_READER_H */