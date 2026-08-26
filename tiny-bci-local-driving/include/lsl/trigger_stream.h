# pragma once
# ifndef LSL_TRIGGER_OUTLET
# define LSL_TRIGGER_OUTLET

# define TRIGGER_STREAM_NAME "Tiny_BCI_Triggers"
# define TRIGGER_STREAM_TYPE "Triggers"
# define TRIGGER_STREAM_SOURCE_ID "tiny_bci_ssvep_experiment_triggers"

# define TRIGGER_STREAM_PREDICATE "type='" TRIGGER_STREAM_TYPE "'"

void openLslTriggerOutlet();
void pushLslTrigger(uint16_t value);
void closeLslTriggerOutlet();

bool doesLslTriggerOutletHaveConsumers();

typedef void (*TriggerCallback)(uint16_t value);

void initializeLslTriggerSource(TriggerCallback callback);
void updateLslTriggerSource();
void closeLslTriggerSource();

bool tryConnectLslTriggerSource();
bool isLslTriggerSourceConnected();

# endif