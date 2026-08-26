/**
 * @file neuropawn_producer.h
 *
 * @brief NeuroPawn Knight EEG producer for TinyBCI.
 *
 * Reads raw EEG data from a NeuroPawn Knight board via serial port and pushes
 * it directly into the TinyBCI pipeline via in_push_signal. No LSL dependency —
 * data flows directly from hardware into the pipeline.
 *
 * The board is configured at connect time by sending ASCII commands over the
 * serial port (enable/disable channels, right-leg-drive). The board type
 * (non-IMU vs IMU) is auto-detected from the incoming packet stride, exactly
 * like the reference knight_lsl_gui.py: two consecutive frames must line up at
 * the expected stride, then the first (possibly partial) packet is discarded so
 * acquisition starts on a clean frame boundary.
 *
 * ## Supported platforms
 *   Windows — COM3
 *   macOS   — /dev/cu.usbserial-XXXX
 *   Linux   — /dev/ttyUSB0
 *
 * ## Packet format
 * Every frame starts with 0xA0 and ends with 0xC0.
 *
 * Non-IMU board — 21 bytes total (0xA0 + 20 payload bytes):
 *   payload[0]      sample counter
 *   payload[1..16]  8 EXG channels, 16-bit big-endian signed (2 bytes each)
 *   payload[17]     LOFF STATP
 *   payload[18]     LOFF STATN
 *   payload[19]     0xC0 end byte
 *
 * IMU board — 57 bytes total (0xA0 + 56 payload bytes):
 *   payload[0]      sample counter
 *   payload[1..16]  8 EXG channels, 16-bit big-endian signed
 *   payload[17]     LOFF STATP
 *   payload[18]     LOFF STATN
 *   payload[19..54] 9 IMU channels, 32-bit little-endian float (4 bytes each)
 *   payload[55]     0xC0 end byte
 */

#ifndef TBCI_NEUROPAWN_PRODUCER_H
#define TBCI_NEUROPAWN_PRODUCER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "ioutils/tbci_input.h"
#include "tbci_context.h"
#include "ioutils/tbci_producer.h"
#include "tbci_trigger_generator.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Platform serial port includes
 * -------------------------------------------------------------------------- */
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
typedef HANDLE NeuroPawnSerialHandle;
#else
#include <termios.h>
typedef int NeuroPawnSerialHandle;
#define NEUROPAWN_INVALID_HANDLE (-1)
#endif

/* --------------------------------------------------------------------------
 * Constants
 * -------------------------------------------------------------------------- */
#define NEUROPAWN_N_CHANNELS        8       /**< Number of EXG (EEG) channels.        */
#define NEUROPAWN_N_IMU             9       /**< Number of IMU channels (IMU board).  */
#define NEUROPAWN_SRATE             125.0f  /**< Sampling rate in Hz.                 */
#define NEUROPAWN_START_BYTE        0xA0    /**< Frame start byte.                    */
#define NEUROPAWN_END_BYTE          0xC0    /**< Frame end byte.                      */
#define NEUROPAWN_EEG_PAYLOAD_LEN   20      /**< Payload bytes after 0xA0, non-IMU.   */
#define NEUROPAWN_IMU_PAYLOAD_LEN   56      /**< Payload bytes after 0xA0, IMU board. */
#define NEUROPAWN_DEFAULT_GAIN      12      /**< Default channel gain.                */
#define NEUROPAWN_CMD_PAUSE_MS      2000u   /**< Delay between config commands (ms).  */

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */

/** Detected board variant. */
typedef enum {
    NEUROPAWN_BOARD_UNKNOWN = 0,  /**< Not yet detected.                 */
    NEUROPAWN_BOARD_EEG,          /**< Non-IMU board (21-byte frames).   */
    NEUROPAWN_BOARD_IMU           /**< IMU board (57-byte frames).       */
} NeuroPawnBoardType;

typedef struct {
    const char *port;        /**< Serial port path. e.g. "COM3" or "/dev/ttyUSB0". */
    float       srate;       /**< Sampling rate. Default 125 Hz.                   */
    size_t      n_channels;  /**< Number of EXG channels. Default 8.               */
    uint8_t     gain;        /**< Channel gain (1,2,3,4,6,8,12). Default 12.       */
    uint32_t    cmd_pause_ms;/**< Delay between config commands. 0 = default 2 s.  */
    bool        channel_enabled[NEUROPAWN_N_CHANNELS]; /**< Per-channel enable.    */
    bool        rld_enabled[NEUROPAWN_N_CHANNELS];     /**< Per-channel RLD enable.*/
} NeuroPawnProducerConfig;

typedef struct {
    NeuroPawnSerialHandle handle;        /**< Serial port handle.                  */
    bool                  connected;     /**< True if serial port is open.         */
    uint32_t              sample_index;  /**< Monotonic sample counter.            */
    uint64_t              timestamp_us;  /**< Current timestamp in microseconds.   */
    uint32_t              spacing_us;    /**< Sample spacing. Computed at init.    */
    float                 eeg_scale;     /**< µV per ADC count. Computed at init.  */
    NeuroPawnBoardType    board_type;    /**< Auto-detected board variant.         */
    size_t                payload_len;   /**< Payload length for the detected board.*/
    size_t                n_total_channels; /**< 8 (EEG) or 17 (EEG + IMU).        */
    float                 imu[NEUROPAWN_N_IMU];           /**< Latest IMU sample.   */
    uint8_t               payload[NEUROPAWN_IMU_PAYLOAD_LEN]; /**< Frame payload.   */
} NeuroPawnProducerState;

typedef struct {
    TBCI_Producer            base;        /**< Must be first member.               */
    NeuroPawnProducerConfig *config;
    NeuroPawnProducerState   state;
    TBCI_TriggerGenerator   *trigger_gen; /**< Optional. NULL = no fake triggers.  */
} NeuroPawnProducer;

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialise the NeuroPawn producer (wires function pointers, defaults).
 *
 * Does not open the serial port — call producer->init (np_connect) for that,
 * or use tbci_producer_create_neuropawn.
 *
 * @param[out] producer  Pointer to an uninitialised producer. Must not be NULL.
 * @param[in]  config    Caller-owned configuration. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status np_init(NeuroPawnProducer *producer, NeuroPawnProducerConfig *config);

/**
 * @brief Read one frame from the board and push it into the pipeline.
 *
 * @param[in,out] producer  Pointer to an initialised producer. Must not be NULL.
 * @param[in,out] inputs    Pipeline inputs to write into. Must not be NULL.
 * @param[in,out] ctx       Pipeline context. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_EMPTY if no full frame available yet.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 * @return TBCI_ERR_INVALID_STATE if frame validation fails.
 */
TBCI_API TBCI_Status np_tick(NeuroPawnProducer *producer, TBCI_Input *inputs, TBCI_Context *ctx);

/**
 * @brief Reset sample counters and the optional trigger generator.
 *
 * @param[in,out] producer  Pointer to an initialised producer. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if producer is NULL.
 */
TBCI_API TBCI_Status np_reset(NeuroPawnProducer *producer);

/**
 * @brief Close the serial port.
 *
 * @param[in,out] producer  Pointer to an initialised producer. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if producer is NULL.
 */
TBCI_API TBCI_Status np_close(NeuroPawnProducer *producer);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_NEUROPAWN_PRODUCER_H */
