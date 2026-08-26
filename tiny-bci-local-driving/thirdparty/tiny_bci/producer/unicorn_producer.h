/**
 * @file unicorn_producer.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unicorn Hybrid Black EEG producer for TinyBCI.
 *
 * Reads raw EEG data from a gtec Unicorn Hybrid Black via serial port
 * and pushes it directly into the TinyBCI pipeline via in_push_signal.
 *
 * No LSL dependency — data flows directly from hardware into the pipeline.
 *
 * ## Supported platforms
 *   macOS  — /dev/cu.UN-XXXXXXXX
 *   Linux  — /dev/ttyUSB0
 *   Windows — COM3
 *
 * ## Unicorn packet format (45 bytes)
 *   [0:2]   start sequence  0xC0 0x00
 *   [2]     battery level
 *   [3:27]  8 EEG channels, 3 bytes each, big-endian signed 24-bit
 *   [27:39] accelerometer + gyroscope (ignored)
 *   [39:43] sample counter, little-endian uint32
 *   [43:45] stop sequence  0x0D 0x0A
 */

#ifndef TBCI_UNICORN_PRODUCER_H
#define TBCI_UNICORN_PRODUCER_H

#include <stdint.h>
#include <stdbool.h>
#include "../include/ioutils/tbci_input.h"
#include "tbci_context.h"
#include "../include/ioutils/tbci_producer.h"
#include "tbci_trigger_generator.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Platform serial port includes
 * -------------------------------------------------------------------------- */
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
typedef HANDLE UnicornSerialHandle;
#else
#include <termios.h>
#include <sys/select.h>
typedef int UnicornSerialHandle;
#define UNICORN_INVALID_HANDLE (-1)
#endif

/* --------------------------------------------------------------------------
 * Constants
 * -------------------------------------------------------------------------- */
#define UNICORN_N_CHANNELS     8
#define UNICORN_SRATE          250.0f
#define UNICORN_PACKET_SIZE    45
#define UNICORN_START_BYTE0    0xC0
#define UNICORN_START_BYTE1    0x00
#define UNICORN_STOP_BYTE0     0x0D
#define UNICORN_STOP_BYTE1     0x0A
#define UNICORN_ADC_REFERENCE_UV  4500000.0f  /**< ADC reference voltage in µV (4.5V) */
#define UNICORN_ADC_MAX_VALUE     50331642.0f /**< Max 24-bit ADC value with gain      */
#define UNICORN_EEG_SCALE         (UNICORN_ADC_REFERENCE_UV / UNICORN_ADC_MAX_VALUE)

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */

typedef struct {
    const char *port;           /**< Serial port path. e.g. /dev/cu.UN-20230805 */
    float       srate;          /**< Sampling rate. Default 250 Hz.              */
    size_t      n_channels;     /**< Number of EEG channels. Default 8.          */
} UnicornProducerConfig;

typedef struct {
    UnicornSerialHandle handle;         /**< Serial port handle.                 */
    bool                connected;      /**< True if serial port is open.        */
    uint32_t            sample_index;   /**< Monotonic sample counter.           */
    uint64_t            timestamp_us;   /**< Current timestamp in microseconds.  */
    uint32_t            spacing_us;     /**< Sample spacing. Computed at init.   */
    uint8_t             packet[UNICORN_PACKET_SIZE]; /**< Raw packet buffer.     */
} UnicornProducerState;

typedef struct {
    TBCI_Producer          base;
    UnicornProducerConfig  *config;
    UnicornProducerState   state;
    TBCI_TriggerGenerator  *trigger_gen;  /**< Optional. NULL = no fake triggers. */
} UnicornProducer;

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialise the Unicorn producer and open the serial port.
 *
 * Opens the serial port, configures baud rate (115200) and starts
 * acquisition. Must be called before up_tick.
 *
 * @param[out] producer  Pointer to an uninitialised producer. Must not be NULL.
 * @param[in]  config    Caller-owned configuration. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 * @return TBCI_ERR_INVALID_STATE if serial port cannot be opened.
 */
TBCI_API TBCI_Status up_init(UnicornProducer *producer,UnicornProducerConfig *config);

/**
 * @brief Read one sample from the Unicorn and push it into the pipeline.
 *
 * Non-blocking — returns TBCI_ERR_EMPTY if no data is available yet.
 * Validates packet start/stop bytes and discards invalid packets.
 *
 * @param[in,out] producer  Pointer to an initialised producer. Must not be NULL.
 * @param[in,out] inputs    Pipeline inputs to write into. Must not be NULL.
 * @param[in,out] ctx       Pipeline context. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_EMPTY if no full packet available yet.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 * @return TBCI_ERR_INVALID_STATE if packet validation fails.
 */
TBCI_API TBCI_Status up_tick(UnicornProducer *producer, TBCI_Input *inputs, TBCI_Context *ctx);

/**
 * @brief Re-initializes the producer.
 *
 * @param[in,out] producer  Pointer to an initialised producer. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if producer is NULL.
 */
TBCI_API TBCI_Status up_reset(UnicornProducer *producer);

/**
 * @brief Stop acquisition and close the serial port.
 *
 * @param[in,out] producer  Pointer to an initialised producer. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if producer is NULL.
 */
TBCI_API TBCI_Status up_close(UnicornProducer *producer);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_UNICORN_PRODUCER_H */