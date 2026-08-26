/**
* @file tbci_lsl_writer.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief A simple LSL wrapper to send outputs to a single outlet.
 *
 */

#ifndef TBCI_LSL_WRITER_H
#define TBCI_LSL_WRITER_H

#ifdef TBCI_WITH_LSL

#include <lsl_c.h>
#include "../tbci_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * TBCI_LSLWriter: State for one LSL output stream.
 *
 * @outlet       LSL outlet for pushing results
 * @n_channels   Number of channels in the output stream
 * @connected    True if outlet is open
 */
typedef struct {
    lsl_outlet  outlet;
    int         n_channels;
    bool        connected;
} TBCI_LSLWriter;

/**
 * @brief Initialise an LSL outlet for pipeline results.
 *
 * Creates a float-typed LSL stream with the given name and channel count.
 *
 * @param[out] writer      Pointer to an uninitialised writer. Must not be NULL.
 * @param[in]  stream_name Name of the LSL output stream. Must not be NULL.
 * @param[in]  stream_type Type string (e.g. "Predictions"). Must not be NULL.
 * @param[in]  n_channels  Number of float channels per sample.
 * @param[in]  srate       Nominal sampling rate. Use 0.0 for irregular streams.
 * @return true on success, false if outlet creation failed.
 */
bool lsl_writer_init(TBCI_LSLWriter *writer, const char *stream_name,
                     const char *stream_type, int n_channels, float srate);

/**
 * @brief Push one epoch's samples to the LSL outlet.
 *
 * @param[in] writer  Pointer to an initialised writer. Must not be NULL.
 * @param[in] epoch   Epoch to push. samples must not be NULL.
 * @return true on success, false if not connected or push failed.
 */
bool lsl_writer_push_epoch(TBCI_LSLWriter *writer, const TBCI_Epoch *epoch);

/**
 * @brief Close the LSL outlet and mark writer as disconnected.
 *
 * @param[in,out] writer  Pointer to an initialised writer. Must not be NULL.
 */
void lsl_writer_close(TBCI_LSLWriter *writer);


#endif /* TBCI_WITH_LSL */

#endif //TBCI_LSL_WRITER_H