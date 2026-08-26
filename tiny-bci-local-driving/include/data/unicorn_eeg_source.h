# pragma once

# define UNICORN_EEG_CHANNEL_COUNT     8
# define UNICORN_SAMPLE_RATE    250
# define UNICORN_PACKET_SIZE    45

# define UNICORN_START_BYTE0    0xC0
# define UNICORN_START_BYTE1    0x00
# define UNICORN_STOP_BYTE0     0x0D
# define UNICORN_STOP_BYTE1     0x0A

# define UNICORN_START_ACQUISITION_COMMAND (uint8_t[]){ 0x61, 0x7C, 0x87 }
# define UNICORN_STOP_ACQUISITION_COMMAND (uint8_t[]){ 0x63, 0x5C, 0xC5 }
# define UNICORN_COMMAND_LENGTH 3

# define UNICORN_ADC_REFERENCE_UV  4500000.0f  /**< ADC reference voltage in µV (4.5V) */
# define UNICORN_ADC_MAX_VALUE     50331642.0f /**< Max 24-bit ADC value with gain      */
# define UNICORN_EEG_SCALE         (UNICORN_ADC_REFERENCE_UV / UNICORN_ADC_MAX_VALUE)

void connectUnicornEEGSource(const char *port, uint32_t timeout);
void resetUnicornEEGSource();
void updateUnicornEEGSource();
void closeUnicornEEGSource();

bool isUnicornEEGSourceConnected();
uint8_t getUnicornEEGSourceChannelCount();
uint32_t getUnicornEEGSourceSampleRate();