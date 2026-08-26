# include "data/dsi_eeg_source.h"
# include "pipeline.h"
# include "microsecond_timer.h"
# include "DSI.h"

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/mman.h>
# include <sys/stat.h>
# include <fcntl.h>
#include <unistd.h>
 
static DSI_Headset headset = NULL;
static DSI_Channel *channels = NULL;
static uint8_t channelCount = 0;
static uint32_t sampleRate = 0;
 
static float *samples = NULL;
static uint32_t sampleIndex = 0;
 
static bool isConnected = false;

#define DSI_IMPEDANCE_SHM_NAME "/dsi_impedance"
#define DSI_IMPEDANCE_CTRL_NAME "/dsi_impedance_ctrl"

#define DSI_IMPEDANCE_SHM_SIZE 256

#define DSI_IMPEDANCE_POLL_SEC 0.75
#define DSI_IMPEDANCE_TIMEOUT_SEC 300.0



 
// ---
 
/* Called on DSI's own background thread the instant a new sample is
 * ready -- this is what actually feeds the pipeline, so the main
 * render loop never has to block waiting on serial data. */
static void dsiSampleCallback(DSI_Headset h, double packetTime, void *userData)
{
    (void)h;
    (void)packetTime;
    (void)userData;
 
    for (uint8_t ch = 0; ch < channelCount; ch++)
    {
        samples[ch] = (float)DSI_Channel_ReadBuffered(channels[ch]);
    }
 
    uint64_t timestamp = getCurrentMicrosecondTimestamp();
    in_push_signal(&tbciInputs, samples, timestamp, sampleIndex++);
}
 
// ---
 
void connectDsiEEGSource(const char *port, const char *montage)
{
    /* dlopen only searches LD_LIBRARY_PATH / ldconfig's cache / standard
     * system dirs -- it never checks the current working directory, so a
     * bare "libDSI.so" won't be found sitting in the source tree. Passing
     * a path containing a '/' makes dlopen load that exact file instead
     * of searching for it. */
    if (Load_DSI_API("/home/pi/Documents/tiny-bci-ssvep-experiment/src/data/libDSI.so") != 0)
    {
        fprintf(stderr, "dsi: failed to load DSI API\n");
        exit(EXIT_FAILURE);
    }
 
    headset = DSI_Headset_New(NULL);
    DSI_Headset_Connect(headset, port);
    if (!headset || DSI_Error())
    {
        fprintf(stderr, "dsi: connection error: %s\n", DSI_ClearError());
        exit(EXIT_FAILURE);
    }
    printf("dsi: connected - %s\n", DSI_Headset_GetInfoString(headset));
 
    /* "" reference = default linked-ears reference for this headset model */
    DSI_Headset_ChooseChannels(headset, montage, "", 1);
    if (DSI_Error())
    {
        fprintf(stderr, "dsi: channel setup error: %s\n", DSI_ClearError());
        DSI_Headset_Delete(headset);
        exit(EXIT_FAILURE);
    }
    
    /* NEW impedance setup stage*/
    runDsiImpedanceCheck();
    
 
    channelCount = (uint8_t)DSI_Headset_GetNumberOfChannels(headset);
    sampleRate = (uint32_t)DSI_Headset_GetSamplingRate(headset);
 
    channels = malloc(channelCount * sizeof(DSI_Channel));
    for (uint8_t ch = 0; ch < channelCount; ch++)
    {
        channels[ch] = DSI_Headset_GetChannelByIndex(headset, ch);
    }
    samples = malloc(channelCount * sizeof(float));
 
    printf("dsi: %d channels @ %u Hz\n", channelCount, sampleRate);
 
    DSI_Headset_SetSampleCallback(headset, dsiSampleCallback, NULL);
 
    if (!DSI_Headset_StartBackgroundAcquisition(headset) || DSI_Error())
    {
        fprintf(stderr, "dsi: failed to start background acquisition: %s\n", DSI_ClearError());
        exit(EXIT_FAILURE);
    }
 
    isConnected = true;
}
 
// ---
 
void updateDsiEEGSource()
{
    if (!isConnected) return;
 
    /* Data itself arrives via dsiSampleCallback on the background
     * thread. This just gives the main loop a chance each frame to
     * notice dropped samples or device alarms. */
    int overflow = DSI_Headset_GetNumberOfOverflowedSamples(headset);
    if (overflow > 0)
    {
        fprintf(stderr, "dsi: %d samples overflowed\n", overflow);
    }
 
    while (DSI_Headset_GetNumberOfAlarms(headset) > 0)
    {
        int alarm = DSI_Headset_GetAlarm(headset, 1); /* 1 = remove from queue */
        fprintf(stderr, "dsi: alarm %d\n", alarm);
    }
}
 
// ---
 
void disconnectDsiEEGSource()
{
    if (isConnected)
    {
        DSI_Headset_StopBackgroundAcquisition(headset);
        DSI_Headset_Delete(headset);
        free(channels);
        free(samples);
    }
    isConnected = false;
}
 
// ---
 
uint8_t getDsiEEGSourceChannelCount() { return channelCount; }
uint32_t getDsiEEGSourceSampleRate() { return sampleRate; }
