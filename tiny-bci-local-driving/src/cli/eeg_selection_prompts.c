# include "cli/eeg_source_selection.h"
# include "cli/helpers.h"
# include "data/serial_port_enumeration.h"

EEGSourceType promptEEGSourceSelection()
{
    printf("Select EEG Source:\n");
    printf("\t%u - LSL Stream\n", LSLSource);
    printf("\t%u - Neuropawn over USB\n", NeuropawnSource);
    printf("\t%u - Unicorn over USB\n", UnicornSource);
    printf("\t%u - DSI-7 over USB\n", DSI7Source);
    printf("\t%u - Synthetic test data\n", SyntheticSource);

    return (EEGSourceType)getIntegerSelection(SyntheticSource);
}

const char* promptSerialPortSelection()
{
    while (true)
    {
        uint32_t deviceCount = enumerateSerialPorts();
        printf("%u Available Ports:\n", deviceCount);
        printf("\t0 : rescan\n");

        for (uint32_t i = 0; i < deviceCount; i++)
        {
            printf("\t%u : %s\n", i + 1, getSerialPortName(i));
        }
        printf("Select one of options [0-%u]\t", deviceCount + 1);
        uint32_t selection = getIntegerSelection(deviceCount);

        if (selection == 0) continue;
        return getSerialPortName(selection - 1);
    }
}