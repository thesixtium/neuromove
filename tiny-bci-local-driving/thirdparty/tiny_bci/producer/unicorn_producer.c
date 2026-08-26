/**
 * @file unicorn_producer.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unicorn Hybrid Black EEG producer implementation.
 */
#include "../include/ioutils/tbci_input.h"
#include "tbci_context.h"
#include "unicorn_producer.h"
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>


/* --------------------------------------------------------------------------
 * Platform serial port implementation
 * -------------------------------------------------------------------------- */

#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)

static TBCI_Status serial_open(UnicornProducerState *state, const char *port)
{
    state->handle = CreateFileA(port, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (state->handle == INVALID_HANDLE_VALUE)
        return TBCI_ERR_INVALID_STATE;

    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    GetCommState(state->handle, &dcb);
    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity   = NOPARITY;
    SetCommState(state->handle, &dcb);

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout         = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier  = 0;
    timeouts.ReadTotalTimeoutConstant    = 0;
    SetCommTimeouts(state->handle, &timeouts);

    return TBCI_OK;
}

static TBCI_Status serial_write(UnicornProducerState *state,
                                 const uint8_t *data, size_t len)
{
    DWORD written;
    if (!WriteFile(state->handle, data, (DWORD)len, &written, NULL))
        return TBCI_ERR_INVALID_STATE;
    return TBCI_OK;
}

static int serial_read(UnicornProducerState *state,
                        uint8_t *buf, size_t len)
{
    DWORD read = 0;
    ReadFile(state->handle, buf, (DWORD)len, &read, NULL);
    return (int)read;
}

static void serial_close(UnicornProducerState *state)
{
    CloseHandle(state->handle);
}

#else  /* macOS / Linux */

#include <unistd.h>

#ifdef CRTSCTS
#define TERMIOS_C_ANTIFLAGS PARENB | CSTOPB | CRTSCTS
#else
#define TERMIOS_C_ANTIFLAGS (PARENB | CSTOPB)
#endif

static TBCI_Status serial_open(UnicornProducerState *state, const char *port)
{
    if (access(port, F_OK) != 0) {
        fprintf(stderr, "unicorn: port %s does not exist\n", port);
        return TBCI_ERR_INVALID_STATE;
    }

    state->handle = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (state->handle == UNICORN_INVALID_HANDLE) {
        fprintf(stderr, "unicorn: cannot open %s — %s (errno=%d)\n",
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

static TBCI_Status serial_write(UnicornProducerState *state,
                                 const uint8_t *data, size_t len)
{
    ssize_t written = write(state->handle, data, len);
    if (written < 0) return TBCI_ERR_INVALID_STATE;
    return TBCI_OK;
}

static int serial_read(UnicornProducerState *state,
                        uint8_t *buf, size_t len)
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(state->handle, &readfds);

    struct timeval timeout = { .tv_sec = 0, .tv_usec = 50000 }; // 50ms

    int ready = select(state->handle + 1, &readfds, NULL, NULL, &timeout);
    if (ready <= 0)
        return 0;  // timeout or error — no data

    ssize_t n = read(state->handle, buf, len);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        fprintf(stderr, "serial_read error: %s (errno=%d)\n", strerror(errno), errno);
        return 0;
    }
    return (int)n;
}

static void serial_close(UnicornProducerState *state)
{
    close(state->handle);
}

#endif

/* --------------------------------------------------------------------------
 * Packet parsing
 * -------------------------------------------------------------------------- */

static void parse_eeg(const uint8_t *payload, float *samples)
{
    for (size_t ch = 0; ch < UNICORN_N_CHANNELS; ch++) {
        /* 3 bytes big-endian signed 24-bit */
        int32_t raw = ((int32_t)payload[3 + ch * 3] << 16)
                    | ((int32_t)payload[4 + ch * 3] << 8)
                    | ((int32_t)payload[5 + ch * 3]);

        /* two's complement for 24-bit */
        if (raw & 0x00800000)
        {
            raw |= 0xFF000000;
            raw -= 0x01000000;
        }
        //printf("ch%zu raw=%d scaled=%.2f uV\n", ch, raw, (float)raw * UNICORN_EEG_SCALE);
        samples[ch] = (float)raw * UNICORN_EEG_SCALE;
    }
}

static void reset_bluetooth(void)
{
    printf("unicorn: bluetooth daemon appears stuck.\n");
    printf("unicorn: attempting to reset (requires sudo password)...\n");

    int ret = system("sudo -S pkill bluetoothd");
    if (ret != 0) {
        fprintf(stderr, "unicorn: failed to reset bluetooth daemon (exit=%d)\n", ret);
    } else {
        printf("unicorn: bluetooth daemon reset, waiting...\n");
    }
    printf("unicorn: bluetooth daemon resetting, wait 20 seconds.\n");
    sleep(20);
    printf("unicorn: done!\n");
}

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

static TBCI_Status up_connect(TBCI_Producer       *producer,
                               TBCI_Input          *inputs,
                               struct TBCI_Context *ctx)
{
    (void)inputs;
    (void)ctx;

    UnicornProducer *up = (UnicornProducer *)producer;
    static const uint8_t start_acq[] = { 0x61, 0x7C, 0x87 };

    #if defined(__APPLE__)
        reset_bluetooth();
    #endif

    for (int retry = 0; retry < 2; retry++) {
        TBCI_Status s = serial_open(&up->state, up->config->port);
        if (s != TBCI_OK) return s;

        serial_write(&up->state, start_acq, sizeof(start_acq));

        uint8_t response[3] = {0};
        int     attempts    = 100;
        while (attempts-- > 0) {
            if (serial_read(&up->state, response, 3) == 3) break;
        }

        if (response[0] == 0x00 && response[1] == 0x00 && response[2] == 0x00) {
            up->state.connected    = true;
            producer->connected     = true;
            printf("unicorn: connected on %s\n", up->config->port);
            return TBCI_OK;
        }

        fprintf(stderr, "unicorn: unexpected start response (attempt %d/2)\n", retry + 1);
        serial_close(&up->state);

        if (retry == 0) {
            reset_bluetooth();
        }
    }

    return TBCI_ERR_INVALID_STATE;
}

TBCI_Status up_init(UnicornProducer *producer, UnicornProducerConfig *config)
{
    if (producer == NULL || config == NULL)
        return TBCI_ERR_INVALID_ARG;

    /* wire up base — must be done first */
    producer->base.name      = "unicorn";
    producer->base.connected = false;
    producer->base.init      = up_connect;
    producer->base.tick      = (TBCI_Status(*)(TBCI_Producer*, TBCI_Input*, struct TBCI_Context*)) up_tick;
    producer->base.reset     = (TBCI_Status(*)(TBCI_Producer*)) up_reset;
    producer->base.close     = (TBCI_Status(*)(TBCI_Producer*)) up_close;

    producer->config = config;
    producer->state = (UnicornProducerState){0};
    producer->state.spacing_us = (uint32_t)(1000000.0f / config->srate);
    producer->state.connected    = false;

    return TBCI_OK;
}

TBCI_Status up_tick(UnicornProducer *producer, TBCI_Input *inputs, TBCI_Context *ctx)
{
    if (producer == NULL || inputs == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    UnicornProducerState  *state  = &producer->state;

    /* try to read one full packet */
    size_t total = 0;
    int    attempts = 200; // ~10 seconds at 50ms each
    while (total < UNICORN_PACKET_SIZE && attempts-- > 0) {
        int n = serial_read(state, state->packet + total, UNICORN_PACKET_SIZE - total);
        if (n > 0) total += n;
    }

    if (total < UNICORN_PACKET_SIZE) {
        fprintf(stderr, "unicorn: timeout waiting for packet (got %zu/%d bytes)\n",
                total, UNICORN_PACKET_SIZE);
        return TBCI_ERR_EMPTY;
    }

    /* validate packet */
    if (state->packet[0]  != UNICORN_START_BYTE0 ||
        state->packet[1]  != UNICORN_START_BYTE1 ||
        state->packet[43] != UNICORN_STOP_BYTE0  ||
        state->packet[44] != UNICORN_STOP_BYTE1) {
        fprintf(stderr, "unicorn: invalid packet\n");
        return TBCI_ERR_INVALID_STATE;
    }

    /* parse EEG */
    float samples[UNICORN_N_CHANNELS];
    parse_eeg(state->packet, samples);

    /* push into pipeline */
    in_push_signal(inputs, samples, state->timestamp_us, state->sample_index);

    // in up_tick after pushing signal
    if (producer->trigger_gen != NULL) {
        //printf("unicorn: trigger started\n");
        tg_tick(producer->trigger_gen, inputs, ctx, state->timestamp_us);
    }

    /* advance time */
    state->timestamp_us += state->spacing_us;
    state->sample_index++;

    return TBCI_OK;
}

TBCI_Status up_reset(UnicornProducer *producer)
{
    if (producer == NULL) return TBCI_ERR_INVALID_ARG;

    producer->state.sample_index = 0;
    producer->state.timestamp_us = 0;

    if (producer->trigger_gen != NULL)
        tg_reset(producer->trigger_gen);

    return TBCI_OK;
}

TBCI_Status up_close(UnicornProducer *producer)
{
    if (producer == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (producer->state.connected) {
        static const uint8_t stop_acq[] = { 0x63, 0x5C, 0xC5 };
        serial_write(&producer->state, stop_acq, sizeof(stop_acq));
        serial_close(&producer->state);
        producer->state.connected = false;
        producer->base.connected = false;
        printf("unicorn: disconnected\n");
    }

    return TBCI_OK;
}

