/**
 * @file tbci_config.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Top-level configuration for the TinyBCI pipeline.
 *
 * Holds all parameters needed to initialise and run the pipeline.
 * Some fields (n_channels, nominal_srate) can be overridden at runtime
 * by the producer (e.g. an LSL stream) once the stream connects. If
 * no stream is used, the values set here are used directly.
 *
 * ## Sampling rate
 *
 * @p nominal_srate is the hardware sampling rate as reported by the
 * acquisition device. @p target_srate is the rate the pipeline operates
 * at after optional resampling. If no resampling is needed, set both
 * to the same value.
 *
 * ## Channel count
 *
 * @p n_channels is used to pre-allocate buffers at init time. If the
 * producer discovers a different channel count at connect time it writes
 * the correct value into TBCIInputs.n_channels, which takes precedence.
 *
 * ## Node selection
 *
 * @p use_preprocessing and @p use_feature_extraction control which
 * optional nodes are enabled in the pipeline. Disabled nodes are
 * skipped by the runner with zero overhead.
 */

#ifndef TBCI_CONFIG_H
#define TBCI_CONFIG_H

#include "tbci_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* Pipeline globals */
    TBCI_Paradigm paradigm;              /**< BCI paradigm type. Drives segmentation logic.           */
    float        nominal_srate;          /**< Nominal hardware sampling rate in Hz.                   */
    float        target_srate;           /**< Target sampling rate after resampling in Hz.            */
    size_t       n_channels;             /**< Expected number of signal channels.                     */
    size_t       n_classes;              /**< Expected number of output classes.                     */
    uint32_t     window_length_ms;       /**< Epoch window length in milliseconds.                    */
    /* Stage toggles */
    bool         use_preprocessing;      /**< Enable the preprocessing nodes.                         */
    bool         use_feature_extraction; /**< Enable the feature extraction nodes.                    */
    bool         use_decoder;            /**< Enable the decoding nodes.                            */
    /* Core node (sync + segmentation)*/
    TBCI_SegmentationMode mode;     /**< Windowing mode. Drives epoch extraction strategy.     */
    uint32_t pre_stimulus_ms;       /**< Pre-stimulus window in ms. 0 = no baseline.  */
    uint32_t post_stimulus_ms;      /**< Post-stimulus window in ms. Must be > 0.      */
    uint32_t overlap_ms;            /** SEG_MODE_SLIDING only */
    uint16_t target_code;           /** trigger code that marks the target, e.g. for P300   */
    uint16_t trial_end_code;        /** trigger code that ends a trial   */
    /* Core node - raw output logging */
    bool     log_enabled;
    bool     log_commands;
    bool     log_processed;         /**< true = log processed_signal, false = log raw inputs->signal */
    char     log_subject[64];
    char     log_session[32];
} TBCI_Config;

#ifdef __cplusplus
}
#endif

#endif /* TBCI_CONFIG_H */
