/**
 * @file neuropawn_producer.c
 *
 * @brief NeuroPawn Knight EEG producer implementation.
 *
 * Startup sequence (np_connect):
 *   1. Open the serial port at 115200 baud.
 *   2. Send the per-channel configuration commands (chon_/choff_) and the
 *      optional right-leg-drive commands (rldadd_), pausing between each so the
 *      firmware has time to apply them.
 *   3. Auto-detect the board type (non-IMU vs IMU) from the packet stride.
 *   4. Throw the first (possibly partial) packet away so acquisition starts on
 *      a clean frame boundary.
 */

#include "neuropawn_producer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <sys/select.h>
#include <time.h>
#endif

/* --------------------------------------------------------------------------
 * Portable millisecond sleep
 * -------------------------------------------------------------------------- */
static void np_sleep_ms(uint32_t ms)
{
#if defined(_WIN32) || defined(_WIN64)
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec  = ms / 1000u;
    ts.tv_nsec = (long)(ms % 1000u) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

/* --------------------------------------------------------------------------
 * Platform serial port implementation
 * -------------------------------------------------------------------------- */

#if defined(_WIN32) || defined(_WIN64)

static TBCI_Status serial_open(NeuroPawnProducerState *state, const char *port)
{
    state->handle = CreateFileA(port, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (state->handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "neuropawn: cannot open %s\n", port);
        return TBCI_ERR_INVALID_STATE;
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    GetCommState(state->handle, &dcb);
    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity   = NOPARITY;
    SetCommState(state->handle, &dcb);

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout        = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant   = 50;  /* 50 ms read timeout */
    SetCommTimeouts(state->handle, &timeouts);

    return TBCI_OK;
}

static TBCI_Status serial_write(NeuroPawnProducerState *state,
                                const uint8_t *data, size_t len)
{
    DWORD written = 0;
    if (!WriteFile(state->handle, data, (DWORD)len, &written, NULL))
        return TBCI_ERR_INVALID_STATE;
    return TBCI_OK;
}

static int serial_read(NeuroPawnProducerState *state, uint8_t *buf, size_t len)
{
    DWORD read = 0;
    ReadFile(state->handle, buf, (DWORD)len, &read, NULL);
    return (int)read;
}

static void serial_close(NeuroPawnProducerState *state)
{
    CloseHandle(state->handle);
}

static void serial_flush_input(NeuroPawnProducerState *state)
{
    PurgeComm(state->handle, PURGE_RXCLEAR);
}

#else  /* macOS / Linux */

#ifdef CRTSCTS
#define TERMIOS_C_ANTIFLAGS PARENB | CSTOPB | CRTSCTS
#else
#define TERMIOS_C_ANTIFLAGS PARENB | CSTOPB
#endif

static TBCI_Status serial_open(NeuroPawnProducerState *state, const char *port)
{
    if (access(port, F_OK) != 0) {
        fprintf(stderr, "neuropawn: port %s does not exist\n", port);
        return TBCI_ERR_INVALID_STATE;
    }

    state->handle = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (state->handle == NEUROPAWN_INVALID_HANDLE) {
        fprintf(stderr, "neuropawn: cannot open %s — %s (errno=%d)\n",
                port, strerror(errno), errno);
        return TBCI_ERR_INVALID_STATE;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    tcgetattr(state->handle, &tty);

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tty.c_cflag  = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(TERMIOS_C_ANTIFLAGS);
    tty.c_iflag  = IGNBRK;
    tty.c_lflag  = 0;
    tty.c_oflag  = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    tcsetattr(state->handle, TCSANOW, &tty);
    return TBCI_OK;
}

static TBCI_Status serial_write(NeuroPawnProducerState *state,
                                const uint8_t *data, size_t len)
{
    ssize_t written = write(state->handle, data, len);
    if (written < 0) return TBCI_ERR_INVALID_STATE;
    return TBCI_OK;
}

static int serial_read(NeuroPawnProducerState *state, uint8_t *buf, size_t len)
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(state->handle, &readfds);

    struct timeval timeout = { .tv_sec = 0, .tv_usec = 50000 }; /* 50 ms */

    int ready = select(state->handle + 1, &readfds, NULL, NULL, &timeout);
    if (ready <= 0)
        return 0;  /* timeout or error — no data */

    ssize_t n = read(state->handle, buf, len);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        return 0;
    }
    return (int)n;
}

static void serial_close(NeuroPawnProducerState *state)
{
    close(state->handle);
}

static void serial_flush_input(NeuroPawnProducerState *state)
{
    tcflush(state->handle, TCIFLUSH);
}

#endif

/* --------------------------------------------------------------------------
 * Configuration commands
 * -------------------------------------------------------------------------- */

static void np_send_command(NeuroPawnProducerState *state,
                            const char *cmd, uint32_t pause_ms)
{
    serial_write(state, (const uint8_t *)cmd, strlen(cmd));
    printf("neuropawn: sent '%s'\n", cmd);
    /* give the firmware time to apply the command before sending the next */
    np_sleep_ms(pause_ms);
}

/* --------------------------------------------------------------------------
 * Board type auto-detection
 * -------------------------------------------------------------------------- */

/**
 * Return EEG / IMU / UNKNOWN by locating two consecutive frames.
 *
 * Non-IMU frames are 21 bytes (0xA0 + 20), IMU frames are 57 bytes
 * (0xA0 + 56). Requiring two consecutive start/end boundaries at the expected
 * stride makes detection robust against random byte matches. Mirrors the
 * _scan_frame_size logic in knight_lsl_gui.py.
 */
static NeuroPawnBoardType np_scan_frame_size(const uint8_t *buf, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (buf[i] != NEUROPAWN_START_BYTE)
            continue;

        /* non-IMU: stride 21 */
        if (i + 42 < n &&
            buf[i + 20] == NEUROPAWN_END_BYTE   &&
            buf[i + 21] == NEUROPAWN_START_BYTE &&
            buf[i + 41] == NEUROPAWN_END_BYTE   &&
            buf[i + 42] == NEUROPAWN_START_BYTE)
            return NEUROPAWN_BOARD_EEG;

        /* IMU: stride 57 */
        if (i + 114 < n &&
            buf[i + 56]  == NEUROPAWN_END_BYTE   &&
            buf[i + 57]  == NEUROPAWN_START_BYTE &&
            buf[i + 113] == NEUROPAWN_END_BYTE   &&
            buf[i + 114] == NEUROPAWN_START_BYTE)
            return NEUROPAWN_BOARD_IMU;
    }
    return NEUROPAWN_BOARD_UNKNOWN;
}

static NeuroPawnBoardType np_detect_board_type(NeuroPawnProducerState *state)
{
    static uint8_t buf[8192];
    size_t len      = 0;
    int    attempts = 200;  /* ~10 s at 50 ms per read */

    serial_flush_input(state);

    while (attempts-- > 0) {
        if (len >= sizeof(buf)) {
            /* keep only the tail so detection stays bounded */
            memmove(buf, buf + len - 1024, 1024);
            len = 1024;
        }
        int n = serial_read(state, buf + len, sizeof(buf) - len);
        if (n > 0) {
            len += (size_t)n;
            NeuroPawnBoardType bt = np_scan_frame_size(buf, len);
            if (bt != NEUROPAWN_BOARD_UNKNOWN)
                return bt;
        }
    }
    return NEUROPAWN_BOARD_UNKNOWN;
}

/* --------------------------------------------------------------------------
 * Frame reading / parsing
 * -------------------------------------------------------------------------- */

/* Read one aligned frame into state->payload. Finds the 0xA0 start byte, reads
 * payload_len bytes, and validates the trailing 0xC0 end byte. */
static TBCI_Status np_read_frame(NeuroPawnProducerState *state)
{
    uint8_t b     = 0;
    bool    found = false;
    int     scan  = 8192;  /* bytes to scan for a start byte */

    while (scan-- > 0) {
        int n = serial_read(state, &b, 1);
        if (n == 1 && b == NEUROPAWN_START_BYTE) {
            found = true;
            break;
        }
    }
    if (!found)
        return TBCI_ERR_EMPTY;

    size_t total = 0;
    int    tries = 200;
    while (total < state->payload_len && tries-- > 0) {
        int n = serial_read(state, state->payload + total,
                            state->payload_len - total);
        if (n > 0) total += (size_t)n;
    }
    if (total < state->payload_len)
        return TBCI_ERR_EMPTY;

    if (state->payload[state->payload_len - 1] != NEUROPAWN_END_BYTE)
        return TBCI_ERR_INVALID_STATE;

    return TBCI_OK;
}

static void np_parse_exg(const uint8_t *payload, float *samples, float scale)
{
    for (size_t i = 0; i < NEUROPAWN_N_CHANNELS; i++) {
        /* 16-bit big-endian signed, starting at payload[1] */
        int16_t raw = (int16_t)(((uint16_t)payload[1 + 2 * i] << 8) |
                                 (uint16_t)payload[2 + 2 * i]);
        samples[i] = (float)raw * scale;
    }
}

static void np_parse_imu(const uint8_t *payload, float *imu)
{
    /* 9x 32-bit little-endian float, starting at payload[19] */
    for (size_t i = 0; i < NEUROPAWN_N_IMU; i++) {
        float f;
        memcpy(&f, &payload[19 + 4 * i], sizeof(float));
        imu[i] = f;
    }
}

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

static TBCI_Status np_connect(TBCI_Producer       *producer,
                              TBCI_Input          *inputs,
                              struct TBCI_Context *ctx)
{
    (void)inputs;
    (void)ctx;

    NeuroPawnProducer       *np  = (NeuroPawnProducer *)producer;
    NeuroPawnProducerConfig *cfg = np->config;

    TBCI_Status s = serial_open(&np->state, cfg->port);
    if (s != TBCI_OK)
        return s;

    uint32_t pause = (cfg->cmd_pause_ms != 0u) ? cfg->cmd_pause_ms
                                               : NEUROPAWN_CMD_PAUSE_MS;
    char cmd[32];

    /* 1. per-channel enable / disable */
    printf("neuropawn: configuring channels (gain %u)...\n", cfg->gain);
    for (size_t i = 0; i < NEUROPAWN_N_CHANNELS; i++) {
        int ch = (int)i + 1;
        if (cfg->channel_enabled[i])
            snprintf(cmd, sizeof cmd, "chon_%d_%u", ch, cfg->gain);
        else
            snprintf(cmd, sizeof cmd, "choff_%d", ch);
        np_sleep_ms(150); // give small delay for first command to be processed
        np_send_command(&np->state, cmd, pause);
    }

    /* 2. optional right-leg-drive */
    for (size_t i = 0; i < NEUROPAWN_N_CHANNELS; i++) {
        if (cfg->rld_enabled[i]) {
            int ch = (int)i + 1;
            snprintf(cmd, sizeof cmd, "rldadd_%d", ch);
            np_send_command(&np->state, cmd, pause);
        }
    }

    /* 3. auto-detect board type from packet stride */
    printf("neuropawn: detecting board type from incoming packets...\n");
    NeuroPawnBoardType bt = np_detect_board_type(&np->state);
    if (bt == NEUROPAWN_BOARD_UNKNOWN) {
        fprintf(stderr, "neuropawn: detection failed — no valid packets. "
                        "Enable at least one channel and retry.\n");
        serial_close(&np->state);
        return TBCI_ERR_INVALID_STATE;
    }

    np->state.board_type = bt;
    if (bt == NEUROPAWN_BOARD_IMU) {
        np->state.payload_len      = NEUROPAWN_IMU_PAYLOAD_LEN;
        np->state.n_total_channels = NEUROPAWN_N_CHANNELS + NEUROPAWN_N_IMU;
    } else {
        np->state.payload_len      = NEUROPAWN_EEG_PAYLOAD_LEN;
        np->state.n_total_channels = NEUROPAWN_N_CHANNELS;
    }

    /* 4. throw the first packet away so we start on a clean frame boundary */
    serial_flush_input(&np->state);
    np_read_frame(&np->state);

    np->state.connected = true;
    producer->connected = true;
    printf("neuropawn: connected on %s (%s board, %zu ch @ %.0f Hz)\n",
           cfg->port,
           (bt == NEUROPAWN_BOARD_IMU) ? "IMU" : "non-IMU",
           np->state.n_total_channels,
           (double)cfg->srate);
    return TBCI_OK;
}

TBCI_Status np_init(NeuroPawnProducer *producer, NeuroPawnProducerConfig *config)
{
    if (producer == NULL || config == NULL)
        return TBCI_ERR_INVALID_ARG;

    /* wire up base — must be done first */
    producer->base.name      = "neuropawn";
    producer->base.connected = false;
    producer->base.init      = np_connect;
    producer->base.tick      = (TBCI_Status(*)(TBCI_Producer*, TBCI_Input*, struct TBCI_Context*)) np_tick;
    producer->base.reset     = (TBCI_Status(*)(TBCI_Producer*)) np_reset;
    producer->base.close     = (TBCI_Status(*)(TBCI_Producer*)) np_close;

    if (config->srate <= 0.0f)      config->srate      = NEUROPAWN_SRATE;
    if (config->gain == 0)          config->gain       = NEUROPAWN_DEFAULT_GAIN;
    if (config->n_channels == 0)    config->n_channels = NEUROPAWN_N_CHANNELS;

    producer->config = config;
    producer->state  = (NeuroPawnProducerState){0};
    producer->state.spacing_us       = (uint32_t)(1000000.0f / config->srate);
    producer->state.eeg_scale        = 4.0f / 32767.0f / (float)config->gain * 1000000.0f;
    producer->state.board_type       = NEUROPAWN_BOARD_UNKNOWN;
    producer->state.payload_len      = NEUROPAWN_EEG_PAYLOAD_LEN;
    producer->state.n_total_channels = NEUROPAWN_N_CHANNELS;
    producer->state.connected        = false;

    return TBCI_OK;
}

TBCI_Status np_tick(NeuroPawnProducer *producer, TBCI_Input *inputs, TBCI_Context *ctx)
{
    if (producer == NULL || inputs == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    NeuroPawnProducerState *state = &producer->state;

    TBCI_Status s = np_read_frame(state);
    if (s != TBCI_OK)
        return s;

    /* parse the 8 EXG channels (pushed into the pipeline) */
    float samples[NEUROPAWN_N_CHANNELS];
    np_parse_exg(state->payload, samples, state->eeg_scale);

    /* parse IMU into state for inspection (not pushed into the EEG pipeline) */
    if (state->board_type == NEUROPAWN_BOARD_IMU)
        np_parse_imu(state->payload, state->imu);

    in_push_signal(inputs, samples, state->timestamp_us, state->sample_index);

    if (producer->trigger_gen != NULL)
        tg_tick(producer->trigger_gen, inputs, ctx, state->timestamp_us);

    state->timestamp_us += state->spacing_us;
    state->sample_index++;

    return TBCI_OK;
}

TBCI_Status np_reset(NeuroPawnProducer *producer)
{
    if (producer == NULL)
        return TBCI_ERR_INVALID_ARG;

    producer->state.sample_index = 0;
    producer->state.timestamp_us = 0;

    if (producer->trigger_gen != NULL)
        tg_reset(producer->trigger_gen);

    return TBCI_OK;
}

TBCI_Status np_close(NeuroPawnProducer *producer)
{
    if (producer == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (producer->state.connected) {
        serial_close(&producer->state);
        producer->state.connected = false;
        producer->base.connected  = false;
        printf("neuropawn: disconnected\n");
    }

    return TBCI_OK;
}
