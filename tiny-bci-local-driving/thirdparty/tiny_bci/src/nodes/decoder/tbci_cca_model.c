/**
* @file tbci_cca_model.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief CCA model implementation.
 */

#include "tbci_context.h"
#include "../../../include/nodes/decoder/tbci_cca_model.h"

static TBCI_NodeResult cca_model_process_fn(TBCI_Node *self, void *data, struct TBCI_Context *ctx)
{
    TBCI_CCAModel *model = (TBCI_CCAModel *)self;
    TBCI_Epoch    *epoch = (TBCI_Epoch *)data;

    switch (ctx->state) {
    case TBCI_STATE_TRAINING:  return model->base_model.train(&model->base_model, epoch) == TBCI_OK ? TBCI_NODE_OK : TBCI_NODE_ERROR;
    case TBCI_STATE_INFERENCE: return model->base_model.infer(&model->base_model, epoch) == TBCI_OK ? TBCI_NODE_OK : TBCI_NODE_ERROR;
    case TBCI_STATE_IDLE:      return TBCI_NODE_PENDING;
    }
    return TBCI_NODE_ERROR;
}

TBCI_Status cca_model_init(TBCI_CCAModel *model, TBCI_CCAModelConfig *config)
{
    if (model == NULL || config == NULL) return TBCI_ERR_INVALID_ARG;

    model->base_model.base.name       = "cca_model";
    model->base_model.base.type       = TBCI_NODE_TYPE_DECODER;
    model->base_model.base.enabled    = true;
    model->base_model.base.init_fn    = NULL;
    model->base_model.base.process_fn = cca_model_process_fn;
    model->base_model.base.reset_fn   = NULL;
    model->base_model.base.tick_fn    = NULL;
    model->base_model.type            = TBCI_CCA_MODEL;
    model->base_model.train           = cca_model_train;
    model->base_model.eval            = cca_model_eval;
    model->base_model.infer           = cca_model_infer;
    model->base_model.eval_score      = -1.0f;
    model->base_model.confidence      = -1.0f;
    model->base_model.predicted_class = -1;
    model->base_model.scorer = NULL;  /* CCA eval is always 1.0, scorer unused */
    model->config               = *config;
    model->base_model.base.instance_size = sizeof(TBCI_CCAModel);

    return TBCI_OK;
}

TBCI_Status cca_model_train(TBCI_Model *self, TBCI_Epoch *epoch)
{
    return TBCI_OK;
}

TBCI_Status cca_model_eval(TBCI_Model *self, float *accuracy_out)
{
    if (accuracy_out != NULL) *accuracy_out = 1.0f;
    return TBCI_OK;
}

TBCI_Status cca_model_infer(TBCI_Model *self, TBCI_Epoch *epoch)
{
    if (self == NULL || epoch == NULL) return TBCI_ERR_INVALID_ARG;

    TBCI_CCAModel *model = (TBCI_CCAModel *)self;

    size_t n = model->config.n_freqs;  /* n_freqs */
    float *outputs = epoch->samples;


    /* 1. z-score normalize correlations */
    tbci_normalize_zscore(outputs, n);
    /* 2. softmax with temperature */
    tbci_softmax(outputs, n, model->config.temperature);
    /* 3. argmax → predicted_class */
    model->base_model.predicted_class = tbci_argmax(outputs, n);
    model->base_model.confidence = outputs[model->base_model.predicted_class];
    /* 4. store predicted class in epoch label for downstream consumers */
    epoch->predicted_label = (int16_t)model->base_model.predicted_class;
    epoch->confidence = model->base_model.confidence;

    return TBCI_OK;
}