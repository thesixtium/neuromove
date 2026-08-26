# include "lsl/helpers.h"
# include "lsl/constants.h"

lsl_inlet connectAndOpenLslInlet(lsl_streaminfo targetStream)
{
    if (targetStream == NULL) return NULL;

    lsl_inlet inlet = lsl_create_inlet(
        targetStream, 360,
        LSL_NO_PREFERENCE, LSL_RECOVER
    );

    if (inlet == NULL)
    {
        fprintf(stderr, "Failed to create LSL inlet\n");
        return NULL;
    }

    int32_t openError = 0;
    lsl_open_stream(inlet, LSL_CONNECT_TIMEOUT, &openError);

    if (openError != lsl_no_error)
    {
        lsl_destroy_inlet(inlet);
        fprintf(stderr, "Failed to connect to LSL stream\n");
        return NULL;
    }

    return inlet;
}

void closeLslInlet(lsl_inlet *inlet)
{
    if (*inlet == NULL) return;

    lsl_close_stream(*inlet);
    lsl_destroy_inlet(*inlet);
    *inlet = NULL;
}

lsl_continuous_resolver createLslResolver(const char* predicate)
{
    return lsl_create_continuous_resolver_bypred(predicate, LSL_FORGET_AFTER);
}

void closeLslResolver(lsl_continuous_resolver *resolver)
{
    if (*resolver == NULL) return;
    lsl_destroy_continuous_resolver(*resolver);
    *resolver = NULL;
}