#ifdef TBCI_WITH_ONNX

#include "nodes/decoder/tbci_onnx_model.h"
#include "tbci_context.h"

static TBCI_Status ort_check(TBCI_ONNXModel *model, OrtStatus *status, const char *label)
{
    if (status == NULL) return TBCI_OK;
    fprintf(stderr, "%s failed: %s\n", label, model->ort->GetErrorMessage(status));
    model->ort->ReleaseStatus(status);
    onnx_model_close(model);
    return TBCI_ERR_INVALID_STATE;
}

static TBCI_NodeResult onnx_model_process_fn(TBCI_Node *self, void *data, struct TBCI_Context *ctx)
{
    TBCI_ONNXModel *model = (TBCI_ONNXModel *)self;
    TBCI_Epoch     *epoch = (TBCI_Epoch *)data;

    switch (ctx->state) {
    case TBCI_STATE_TRAINING: {
            TBCI_Status s = model->base_model.train(&model->base_model, epoch);
            return (s == TBCI_OK || s == TBCI_WARN_FULL_TRIALS) ? TBCI_NODE_OK : TBCI_NODE_ERROR;
    }
    case TBCI_STATE_INFERENCE: return model->base_model.infer(&model->base_model, epoch) == TBCI_OK ? TBCI_NODE_OK : TBCI_NODE_ERROR;
    case TBCI_STATE_IDLE:      return TBCI_NODE_PENDING;
    }
    return TBCI_NODE_ERROR;
}

TBCI_Status onnx_model_init(TBCI_ONNXModel *model, TBCI_ONNXModelConfig *config,
                             struct TBCI_Context *ctx)
{
    if (model == NULL || config == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (strlen(config->model_path) == 0 ||
        strlen(config->model_path) >= TBCI_ONNX_MAX_PATH_LEN) {
        fprintf(stderr, "onnx_model_init: invalid model_path length\n");
        return TBCI_ERR_INVALID_ARG;
    }

    model->config      = *config;
    model->train_count = 0;

    if (strlen(model->config.input_name)  == 0) strncpy(model->config.input_name,  "input",  63);
    if (strlen(model->config.output_name) == 0) strncpy(model->config.output_name, "output", 63);

    /* wire base_model */
    model->base_model.base.name       = "onnx_model";
    model->base_model.base.type       = TBCI_NODE_TYPE_DECODER;
    model->base_model.base.enabled    = true;
    model->base_model.base.init_fn    = NULL;
    model->base_model.base.process_fn = onnx_model_process_fn;
    model->base_model.base.reset_fn   = NULL;
    model->base_model.base.tick_fn    = NULL;
    model->base_model.base.instance_size = sizeof(TBCI_ONNXModel);
    model->base_model.type            = TBCI_ONNX_MODEL;
    model->base_model.train           = onnx_model_train;
    model->base_model.eval            = onnx_model_eval;
    model->base_model.infer           = onnx_model_infer;
    model->base_model.eval_score      = -1.0f;
    model->base_model.confidence      = -1.0f;
    model->base_model.predicted_class = -1;
    model->base_model.scorer = config->scorer;

    /* ORT API */
    model->ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    if (model->ort == NULL) {
        fprintf(stderr, "onnx_model_init: failed to get ORT API\n");
        return TBCI_ERR_INVALID_STATE;
    }

    TBCI_Status        s;
    OrtSessionOptions *opts      = NULL;
    OrtTypeInfo       *type_info = NULL;
    OrtTensorTypeAndShapeInfo *shape_info = NULL;
    size_t             n_dims    = 0;
    int64_t            dims[TBCI_MAX_ONNX_TENSOR_DIMS] = {0};

    /* environment */
    s = ort_check(model, model->ort->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "tbci", &model->env),
                  "CreateEnv");
    if (s != TBCI_OK) return s;

    /* session options */
    s = ort_check(model, model->ort->CreateSessionOptions(&opts), "CreateSessionOptions");
    if (s != TBCI_OK) return s;

    s = ort_check(model, model->ort->SetIntraOpNumThreads(opts, 1), "SetIntraOpNumThreads");
    if (s != TBCI_OK) { model->ort->ReleaseSessionOptions(opts); return s; }

    /* create session */
#ifdef _WIN32
    wchar_t wpath[TBCI_ONNX_MAX_PATH_LEN];
    mbstowcs(wpath, config->model_path, TBCI_ONNX_MAX_PATH_LEN);
    s = ort_check(model, model->ort->CreateSession(model->env, wpath, opts, &model->session), "CreateSession");
#else
    s = ort_check(model, model->ort->CreateSession(model->env, config->model_path, opts, &model->session), "CreateSession");
#endif
    model->ort->ReleaseSessionOptions(opts);
    if (s != TBCI_OK) return s;

    /* memory info */
    s = ort_check(model, model->ort->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &model->memory_info),"CreateCpuMemoryInfo");
    if (s != TBCI_OK) return s;

    /* input shape */
    s = ort_check(model, model->ort->SessionGetInputTypeInfo(model->session, 0, &type_info), "SessionGetInputTypeInfo");
    if (s != TBCI_OK) return s;

    s = ort_check(model, model->ort->CastTypeInfoToTensorInfo(type_info, (const OrtTensorTypeAndShapeInfo **)&shape_info), "CastTypeInfoToTensorInfo");
    if (s != TBCI_OK) return s;

    s = ort_check(model, model->ort->GetDimensionsCount(shape_info, &model->n_input_dims), "GetDimensionsCount");
    if (s != TBCI_OK) return s;

    s = ort_check(model, model->ort->GetDimensions(shape_info, model->input_dims, model->n_input_dims), "GetDimensions");
    if (s != TBCI_OK) return s;
    model->ort->ReleaseTypeInfo(type_info);

    /* flat product — skip dynamic dims (reported as -1 or 0) */
    model->input_size = 1;
    for (size_t i = 0; i < model->n_input_dims; i++) {
        if (model->input_dims[i] <= 0) continue;  /* dynamic batch dim */
        model->input_size *= (size_t)model->input_dims[i];
    }

    /* output shape */
    s = ort_check(model, model->ort->SessionGetOutputTypeInfo(model->session, 0, &type_info),
                  "SessionGetOutputTypeInfo");
    if (s != TBCI_OK) return s;

    s = ort_check(model, model->ort->CastTypeInfoToTensorInfo(type_info,
                  (const OrtTensorTypeAndShapeInfo **)&shape_info),
                  "CastTypeInfoToTensorInfo");
    if (s != TBCI_OK) return s;

    s = ort_check(model, model->ort->GetDimensionsCount(shape_info, &n_dims), "GetDimensionsCount");
    if (s != TBCI_OK) return s;

    s = ort_check(model, model->ort->GetDimensions(shape_info, dims, n_dims), "GetDimensions");
    if (s != TBCI_OK) return s;
    model->ort->ReleaseTypeInfo(type_info);

    model->output_size = (size_t)dims[n_dims - 1];

    /* validate buffer sizes */
    if (model->input_size > TBCI_MAX_ONNX_INPUT_SIZE) {
        fprintf(stderr, "onnx_model_init: input_size %zu exceeds TBCI_MAX_ONNX_INPUT_SIZE %d\n",
                model->input_size, TBCI_MAX_ONNX_INPUT_SIZE);
        onnx_model_close(model);
        return TBCI_ERR_INVALID_STATE;
    }
    if (model->output_size > TBCI_MAX_ONNX_OUTPUT_SIZE) {
        fprintf(stderr, "onnx_model_init: output_size %zu exceeds TBCI_MAX_ONNX_OUTPUT_SIZE %d\n",
                model->output_size, TBCI_MAX_ONNX_OUTPUT_SIZE);
        onnx_model_close(model);
        return TBCI_ERR_INVALID_STATE;
    }

    /* validate output mode against actual output size — must happen after dims are read */
    if (model->config.output_mode == TBCI_OUTPUT_SIGMOID && model->output_size != 1) {
        fprintf(stderr, "onnx_model_init: sigmoid mode requires output_size=1, got %zu\n",
                model->output_size);
        onnx_model_close(model);
        return TBCI_ERR_INVALID_STATE;
    }
    if (model->config.output_mode == TBCI_OUTPUT_SOFTMAX && model->output_size < 2) {
        fprintf(stderr, "onnx_model_init: softmax mode requires output_size>=2, got %zu\n",
                model->output_size);
        onnx_model_close(model);
        return TBCI_ERR_INVALID_STATE;
    }

    printf("onnx_model_init: loaded '%s' input=%zu output=%zu\n",
           config->model_path, model->input_size, model->output_size);

    return TBCI_OK;
}

TBCI_Status onnx_model_train(TBCI_Model *self, TBCI_Epoch *epoch)
{
    if (self == NULL || epoch == NULL) return TBCI_ERR_INVALID_ARG;

    TBCI_ONNXModel *model = (TBCI_ONNXModel *)self;

    if (model->config.train_trials == NULL || model->config.train_labels == NULL)
        return TBCI_ERR_INVALID_STATE;

    if (model->train_count >= model->config.train_capacity) {
        fprintf(stderr, "onnx_model_train: buffer full (capacity=%zu), skipping epoch\n",
                model->config.train_capacity);
        return TBCI_WARN_FULL_TRIALS;
    }

    /* copy flat samples into train_trials at offset */
    float *dst = model->config.train_trials + model->train_count * model->input_size;
    memcpy(dst, epoch->samples, model->input_size * sizeof(float));
    model->config.train_labels[model->train_count] = epoch->label;
    model->train_count++;

    return TBCI_OK;
}

TBCI_Status onnx_model_eval(TBCI_Model *self, float *score_out)
{
    if (self == NULL || score_out == NULL) return TBCI_ERR_INVALID_ARG;

    TBCI_ONNXModel *model = (TBCI_ONNXModel *)self;

    if (model->train_count < model->config.n_folds) {
        fprintf(stderr, "onnx_model_eval: not enough trials (%zu) for %zu folds\n",
                model->train_count, model->config.n_folds);
        return TBCI_ERR_INVALID_STATE;
    }

    /* confusion matrix — rows: true label, cols: predicted */
    size_t confusion[TBCI_MAX_CLASSES][TBCI_MAX_CLASSES] = {0};

    for (size_t fold = 0; fold < model->config.n_folds; fold++) {

        for (size_t i = fold; i < model->train_count; i += model->config.n_folds) {

            TBCI_Epoch tmp = {
                .samples    = model->config.train_trials + i * model->input_size,
                .n_channels = model->input_size,
                .n_frames   = 1,
                .label      = model->config.train_labels[i],
            };

            TBCI_Status s = onnx_model_infer(self, &tmp);
            if (s != TBCI_OK) return s;

            uint16_t true_label = model->config.train_labels[i];
            int      predicted  = self->predicted_class;

            if (true_label < TBCI_MAX_CLASSES && predicted >= 0 && predicted < TBCI_MAX_CLASSES)
                confusion[true_label][predicted]++;
        }
    }

    // Default scorer to accuracy if NULL
    TBCI_ScorerFn score_fn = model->base_model.scorer != NULL ? model->base_model.scorer : tbci_score_accuracy;

    *score_out       = score_fn(confusion, model->output_size);
    self->eval_score = *score_out;

    printf("onnx_model_eval: eval_score=%.4f\n", *score_out);
    return TBCI_OK;
}

TBCI_Status onnx_model_infer(TBCI_Model *self, TBCI_Epoch *epoch)
{
    if (self == NULL || epoch == NULL) return TBCI_ERR_INVALID_ARG;

    TBCI_ONNXModel *model = (TBCI_ONNXModel *)self;

    /* validate input size matches epoch dims */
    size_t epoch_size = epoch->n_channels * epoch->n_frames;
    if (epoch_size != model->input_size) {
        fprintf(stderr, "onnx_model_infer: input_size mismatch — expected %zu got %zu\n",
                model->input_size, epoch_size);
        return TBCI_ERR_INVALID_STATE;
    }

    /* copy epoch samples into flat input buffer */
    memcpy(model->input_buf, epoch->samples, model->input_size * sizeof(float));

    /* create input tensor wrapping input_buf — no copy, ORT reads directly */
    OrtValue   *input_tensor  = NULL;
    OrtValue   *output_tensor = NULL;
    int64_t input_shape[TBCI_MAX_ONNX_TENSOR_DIMS];
    for (size_t i = 0; i < model->n_input_dims; i++)
        input_shape[i] = model->input_dims[i] <= 0 ? 1 : model->input_dims[i];

    TBCI_Status s;

    s = ort_check(model,
    model->ort->CreateTensorWithDataAsOrtValue(
        model->memory_info,
        model->input_buf,
        model->input_size * sizeof(float),
        input_shape, model->n_input_dims,
        ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
        &input_tensor),
    "CreateTensorWithDataAsOrtValue");
    if (s != TBCI_OK) return s;

    /* run inference */
    const char *input_names[]  = { model->config.input_name };
    const char *output_names[] = { model->config.output_name };

    s = ort_check(model,
        model->ort->Run(
            model->session, NULL,
            input_names,
            (const OrtValue *const *)&input_tensor,  1,
            output_names, 1, &output_tensor),
        "Run");
    if (s != TBCI_OK) {
        model->ort->ReleaseValue(input_tensor);
        return s;
    }

    /* get pointer to output buffer — ORT owns this memory */
    float *out_ptr = NULL;
    s = ort_check(model, model->ort->GetTensorMutableData(output_tensor, (void **)&out_ptr), "GetTensorMutableData");

    /* copy output into our fixed buffer */
    memcpy(model->output_buf, out_ptr, model->output_size * sizeof(float));

    int predicted_class = -1;
    float confidence = -1.0f;
    if (model->config.output_mode == TBCI_OUTPUT_SIGMOID)
    {
        /* sigmoid: converts raw logit to probability */
        float prob = 1.0f / (1.0f + expf(-model->output_buf[0]));
        predicted_class = (prob >= model->config.sigmoid_threshold) ? 1 : 0;
        confidence      = prob;
    } else if (model->config.output_mode == TBCI_OUTPUT_SOFTMAX)
    {
        tbci_softmax(model->output_buf, epoch->n_channels, model->config.temperature);
        predicted_class = tbci_argmax(model->output_buf, epoch->n_channels);
        confidence = model->output_buf[predicted_class];
    }

    self->predicted_class = predicted_class;
    self->confidence = confidence;
    epoch->predicted_label = (int16_t)predicted_class;
    epoch->confidence = confidence;
    epoch->eval_score = self->eval_score;

    model->ort->ReleaseValue(input_tensor);
    model->ort->ReleaseValue(output_tensor);

    return TBCI_OK;
}

TBCI_Status onnx_model_close(TBCI_ONNXModel *model)
{
    return TBCI_ERR_NOT_IMPLEMENTED;
}

#endif /* TBCI_WITH_ONNX */
