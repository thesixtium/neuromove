# include "data/synthetic_eeg_source.h"
# include "microsecond_timer.h"

static MicrosecondTimer timer;
static float* samples = NULL;
static uint32_t sampleIndex = 0;

static uint8_t channelCount = 0;
static uint32_t sampleRate = 0;

static float tau = (float)(2 * TBCI_M_PI);


void initializeSyntheticEEGSource(uint8_t pChannelCount, uint32_t pSampleRate)
{
    channelCount = pChannelCount;
    sampleRate = pSampleRate;
    timer = createMicrosecondTimer(1.0f / sampleRate);
    samples = malloc(channelCount * sizeof(float));
}

void updateSyntheticEEGSource()
{
    if (checkMicrosecondTimer(&timer))
    {
        uint64_t now = getCurrentMicrosecondTimestamp();

        float currentSeconds = (float)now / 1000000.0f;
        for (uint16_t channelIndex = 0; channelIndex < channelCount; channelIndex++)
        {
            float phaseOffset = channelIndex * (tau / channelCount);
            float sineInput = tau * SIGNAL_FREQUENCY * currentSeconds + phaseOffset;
            samples[channelIndex] = SIGNAL_AMPLITUDE * (float)sin(sineInput);

            samples[channelIndex] += NOISE_60HZ_AMPLITUDE * (float)sin(tau * 60.0f * currentSeconds);
            if (NOISE_AMPLITUDE > 0)
            {
                float noise = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
                samples[channelIndex] += NOISE_AMPLITUDE * noise;
            }
        }

        in_push_signal(&tbciInputs, samples, now, sampleIndex++);
    }
}

void closeSyntheticEEGSource() { free(samples); }

bool isSyntheticEEGSourceReady()
{
    return channelCount > 0
        && sampleRate > 0
        && samples != NULL;
}

uint8_t getSyntheticEEGSourceChannelCount() { return channelCount; }
uint32_t getSyntheticEEGSourceSampleRate() { return sampleRate; }