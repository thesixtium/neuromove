# pragma once
# ifndef LSL_TRIGGER_OUTLET
# define LSL_TRIGGER_OUTLET

# define TRIGGER_STREAM_NAME_DEFAULT "Tiny_BCI_Triggers"
# define TRIGGER_STREAM_TYPE "Triggers"
# define TRIGGER_STREAM_SOURCE_ID "tiny_bci_ssvep_experiment_triggers"

void openLslTriggerOutlet(const char *);
void pushLslTrigger(uint16_t);
void closeLslTriggerOutlet();

# endif