/**
 * @file tbci_trial_averaging_node.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Trial averaging node implementation.
 */

#include "nodes/decoder/tbci_trial_averaging_node.h"
#include "tbci_context.h"

static void ta_compute_avg(TBCI_TrialAveragingNode *node, size_t n_classes)
{
    for (size_t c = 0; c < n_classes; c++)
        node->avg_probs[c] = node->acc[c] / (float)node->current_epochs;
}

static bool ta_should_stop_early(TBCI_TrialAveragingNode *node, size_t n_classes)
{
    int   winner     = tbci_argmax(node->avg_probs, n_classes);
    float confidence = node->avg_probs[winner];
    float saved      = node->avg_probs[winner];
    node->avg_probs[winner] = -1.0f;
    float second     = node->avg_probs[tbci_argmax(node->avg_probs, n_classes)];
    node->avg_probs[winner] = saved;
    float margin     = confidence - second;

    return confidence >= node->config.min_confidence &&
           margin     >= node->config.min_margin;
}

static TBCI_NodeResult ta_process_fn(TBCI_Node *self, void *data, struct TBCI_Context *ctx)
{
    return ta_process((TBCI_TrialAveragingNode *)self, (TBCI_Epoch *)data, ctx);
}

static TBCI_Status ta_reset_fn(TBCI_Node *self)
{
    return ta_reset((TBCI_TrialAveragingNode *)self);
}

TBCI_Status ta_init(TBCI_TrialAveragingNode *node, TBCI_TrialAveragingConfig *config)
{
    if (node == NULL || config == NULL) return TBCI_ERR_INVALID_ARG;

    if (config->n_reps == 0 || config->n_reps > TBCI_MAX_TRIAL_REPS) {
        fprintf(stderr, "ta_init: n_reps=%zu out of range [1, %d]\n",
                config->n_reps, TBCI_MAX_TRIAL_REPS);
        return TBCI_ERR_INVALID_ARG;
    }

    node->config         = *config;
    node->current_epochs = 0;
    memset(node->acc,       0, sizeof(node->acc));
    memset(node->avg_probs, 0, sizeof(node->avg_probs));

    node->base.name          = "trial_averaging";
    node->base.type          = TBCI_NODE_TYPE_DECODER;
    node->base.enabled       = true;
    node->base.instance_size = sizeof(TBCI_TrialAveragingNode);
    node->base.init_fn       = NULL;
    node->base.process_fn    = ta_process_fn;
    node->base.reset_fn      = ta_reset_fn;
    node->base.tick_fn       = NULL;

    return TBCI_OK;
}

TBCI_NodeResult ta_process(TBCI_TrialAveragingNode *node, TBCI_Epoch *epoch, struct TBCI_Context *ctx)
{
    if (node == NULL || epoch == NULL || ctx == NULL) return TBCI_NODE_ERROR;

    size_t n_classes    = epoch->n_channels;
    bool   should_output = false;

    /* accumulate into running sum */
    for (size_t c = 0; c < n_classes; c++)
        node->acc[c] += epoch->samples[c];
    node->current_epochs++;

    /* check if we have enough epochs */
    if (node->current_epochs >= node->config.n_reps) {
        should_output = true;
    } else if (node->config.early_stopping) {
        ta_compute_avg(node, n_classes);
        if (ta_should_stop_early(node, n_classes))
            should_output = true;
    }

    if (!should_output)
        return TBCI_NODE_PENDING;

    /* compute final average */
    ta_compute_avg(node, n_classes);

    memcpy(epoch->samples, node->avg_probs, n_classes * sizeof(float));
    epoch->predicted_label = (int16_t) tbci_argmax(node->avg_probs, n_classes);
    epoch->confidence      = node->avg_probs[epoch->predicted_label];

    ta_reset(node);
    return TBCI_NODE_OK;
}

TBCI_Status ta_reset(TBCI_TrialAveragingNode *node)
{
    if (node == NULL) return TBCI_ERR_INVALID_ARG;

    node->current_epochs = 0;
    memset(node->acc,       0, sizeof(node->acc));
    memset(node->avg_probs, 0, sizeof(node->avg_probs));

    return TBCI_OK;
}