# include "cli/helpers.h"

uint32_t getIntegerSelection(uint32_t maximum)
{
    unsigned int selection = 0;
    scanf("%u", &selection);

    while (selection > maximum)
    {
        printf(" - invalid selection\n");
        scanf("%u", &selection);
    }
    
    return selection;
}