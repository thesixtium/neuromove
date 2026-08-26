# pragma once

# define NEUROPAWN_N_IMU             9      /**< Number of IMU channels (IMU board).       */
# define NEUROPAWN_EEG_CHANNEL_COUNT 8
# define NEUROPAWN_SAMPLE_RATE       125    /**< Sampling rate in Hz.                      */
# define NEUROPAWN_START_BYTE        0xA0   /**< Frame start byte.                         */
# define NEUROPAWN_END_BYTE          0xC0   /**< Frame end byte.                           */
# define NEUROPAWN_EEG_PAYLOAD_LEN   21     /**< Payload bytes including 0xA0, non-IMU.    */
# define NEUROPAWN_IMU_PAYLOAD_LEN   57     /**< Payload bytes including 0xA0, IMU board.  */
# define NEUROPAWN_DEFAULT_GAIN      12     /**< Default channel gain.                     */
# define NEUROPAWN_CMD_PAUSE_MS      200u   /**< Delay between config commands (ms).       */

typedef enum {
    NEUROPAWN_BOARD_UNKNOWN = 0,
    NEUROPAWN_BOARD_EEG,
    NEUROPAWN_BOARD_IMU
} NeuroPawnBoardType;

typedef struct {
    uint8_t gain;
    uint32_t timeout;
    bool activateChannel[NEUROPAWN_EEG_CHANNEL_COUNT];
    bool activateRightLegDrive[NEUROPAWN_EEG_CHANNEL_COUNT];
} NeuropawnConfiguration;

# define TRUE_8_ARRAY {true, true, true, true, true, true, true, true}
# define FALSE_8_ARRAY {false, false, false, false, false, false, false, false}
# define NEUROPAWN_DEFAULT_CONFIGURATION (NeuropawnConfiguration) { 12, 50, TRUE_8_ARRAY, FALSE_8_ARRAY }

void connectNeuropawnEEGSource(const char *, NeuropawnConfiguration);
void resetNeuropawnEEGSource();
void updateNeuropawnEEGSource();
void closeNeuropawnEEGSource();

bool isNeuropawnEEGSourceConnected();
uint8_t getNeuropawnEEGSourceChannelCount();
uint32_t getNeuropawnEEGSourceSampleRate();