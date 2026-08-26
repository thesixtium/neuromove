# include "data/trigger_source.h"

# ifdef USE_LSL_TIMESTAMPS
#   include "lsl_c.h"
uint64_t getTimestamp() { return (uint64_t)(lsl_local_clock() * 1000000); }
# else
#   include "microsecond_timer.h"
uint64_t getTimestamp() { return getCurrentMicrosecondTimestamp(); }
# endif

void pushTrigger(uint16_t value)
{
    uint64_t timestamp = getTimestamp();
    printf("%" PRIu64 " | Pushing trigger : %u\n", timestamp, value);

    TBCI_Trigger trigger = {
        .timestamp_us = timestamp,
        .code = value,
        .type = TBCI_TRIGGER_DATA
    };
    in_push_trigger(&tbciInputs, &trigger, &tbciContext);
}

void pushTrialEndCode()
{
    pushTrigger(TRIAL_END_CODE);
}