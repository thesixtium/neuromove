/**
* @file tbci_cca_node.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief TinyBCI pipeline CCA Node implementation.
 */

#include "../../../include/nodes/features/tbci_cca_node.h"

/* --------------------------------------------------------------------------
 * Matrix utilities
 * -------------------------------------------------------------------------- */

/**
 * @brief Compute the auto-covariance matrix of a row-major matrix: C = X * X^T
 *
 * Avoids materializing X^T by computing each entry directly as a dot
 * product between rows of X. Does not scale by sample count — call
 * mat_scale_covariance afterwards.
 *
 * @param[in]  X     Row-major matrix (rows x cols), e.g. channels x frames.
 * @param[in]  rows  Number of rows in X.
 * @param[in]  cols  Number of columns in X.
 * @param[out] C     Output matrix (rows x rows).
 */
static void mat_self_outer(const float *X, size_t rows, size_t cols, float *C)
{
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < rows; j++) {
            float sum = 0.0f;
            for (size_t k = 0; k < cols; k++)
                sum += X[i * cols + k] * X[j * cols + k];
            C[i * rows + j] = sum;
        }
    }
}

/**
 * @brief Compute the cross-covariance matrix between two row-major matrices: C = X * Y^T
 *
 * Avoids materializing Y^T. Both X and Y must share the same number of columns.
 *
 * @param[in]  X      Row-major matrix (x_rows x cols).
 * @param[in]  Y      Row-major matrix (y_rows x cols).
 * @param[in]  x_rows Number of rows in X.
 * @param[in]  y_rows Number of rows in Y.
 * @param[in]  cols   Shared number of columns.
 * @param[out] C      Output matrix (x_rows x y_rows).
 */
static void mat_cross_outer(const float *X, const float *Y, size_t x_rows, size_t y_rows, size_t cols, float *C)
{
    for (size_t i = 0; i < x_rows; i++) {
        for (size_t j = 0; j < y_rows; j++) {
            float sum = 0.0f;
            for (size_t k = 0; k < cols; k++)
                sum += X[i * cols + k] * Y[j * cols + k];
            C[i * y_rows + j] = sum;
        }
    }
}

/**
 * @brief Multiply a square matrix by a vector: b = A * x
 *
 * @param[in]  A     Row-major matrix (size x size).
 * @param[in]  x     Input vector (size).
 * @param[in]  size  Matrix dimension.
 * @param[out] b     Output vector (size).
 */
static void mat_vec_mul(const float *A, const float *x, size_t size, float *b)
{
    for (size_t i = 0; i < size; i++) {
        b[i] = 0.0f;
        for (size_t j = 0; j < size; j++)
            b[i] += A[i * size + j] * x[j];
    }
}

/**
 * @brief Normalize a vector to unit length in-place.
 *
 * No-op if the norm is zero.
 *
 * @param[in,out] x     Vector to normalize.
 * @param[in]     size  Vector length.
 */
static void vec_normalize(float *x, size_t size)
{
    float norm = 0.0f;
    for (size_t i = 0; i < size; i++)
        norm += x[i] * x[i];
    norm = sqrtf(norm);
    if (norm > 0.0f)
        for (size_t i = 0; i < size; i++)
            x[i] /= norm;
}

/**
 * @brief Estimate the dominant eigenvalue and eigenvector via power iteration.
 *
 * Iterates until convergence (L1 change < TOL) or MAX_ITER is reached.
 * Returns the Rayleigh quotient as the eigenvalue estimate.
 *
 * @param[in]  A            Row-major square matrix (size x size).
 * @param[in]  size         Matrix dimension.
 * @param[out] eigenvector  Output dominant eigenvector (size).
 * @return Dominant eigenvalue estimate.
 */
static float dominant_eigenvalue(const float *A, size_t size, float *eigenvector)
{
    float v[TBCI_MAX_CHANNELS];
    float v_next[TBCI_MAX_CHANNELS];

    for (size_t i = 0; i < size; i++) v[i] = 1.0f;

    for (size_t iter = 0; iter < MAX_ITER; iter++) {
        mat_vec_mul(A, v, size, v_next);
        vec_normalize(v_next, size);

        float diff = 0.0f;
        for (size_t i = 0; i < size; i++)
            diff += fabsf(v_next[i] - v[i]);

        memcpy(v, v_next, size * sizeof(float));
        if (diff < TOL) break;
    }

    /* Rayleigh quotient: λ = v^T A v */
    mat_vec_mul(A, v, size, v_next);
    float lambda = 0.0f;
    for (size_t i = 0; i < size; i++)
        lambda += v[i] * v_next[i];

    memcpy(eigenvector, v, size * sizeof(float));
    return lambda;
}

/**
 * @brief Remove the row-wise mean from a matrix in-place.
 *
 * Each row is centered independently so that its mean becomes zero.
 * Equivalent to removing the DC component from each signal.
 *
 * @param[in,out] X     Row-major matrix (rows x cols).
 * @param[in]     rows  Number of rows.
 * @param[in]     cols  Number of columns.
 */
static void mat_center_rows(float *X, size_t rows, size_t cols)
{
    for (size_t i = 0; i < rows; i++) {
        float sum = 0.0f;
        for (size_t j = 0; j < cols; j++)
            sum += X[i * cols + j];
        float mean = sum / (float)cols;
        for (size_t j = 0; j < cols; j++)
            X[i * cols + j] -= mean;
    }
}

/**
 * @brief Scale a matrix in-place by 1/(n_samples - 1) to obtain the covariance.
 *
 * @param[in,out] X          Row-major matrix (rows x cols).
 * @param[in]     rows       Number of rows.
 * @param[in]     cols       Number of columns.
 * @param[in]     n_samples  Number of samples used to compute the covariance.
 */
static void mat_scale_covariance(float *X, size_t rows, size_t cols, size_t n_samples)
{
    float scale = 1.0f / (float)(n_samples - 1);
    for (size_t i = 0; i < rows; i++)
        for (size_t j = 0; j < cols; j++)
            X[i * cols + j] *= scale;
}

/**
 * @brief General matrix multiplication: C = A * B
 *
 * @param[in]  A  Row-major matrix (m x n).
 * @param[in]  B  Row-major matrix (n x p).
 * @param[out] C  Row-major output matrix (m x p). Must not alias A or B.
 * @param[in]  m  Rows in A.
 * @param[in]  n  Columns in A / rows in B.
 * @param[in]  p  Columns in B.
 */
static void mat_mul(const float *A, const float *B, float *C, size_t m, size_t n, size_t p)
{
    memset(C, 0, m * p * sizeof(float));
    for (size_t i = 0; i < m; i++)
        for (size_t k = 0; k < n; k++) {
            float a = A[i * n + k];
            for (size_t j = 0; j < p; j++)
                C[i * p + j] += a * B[k * p + j];
        }
}

/**
 * @brief Transpose a matrix: B = A^T
 *
 * @param[in]  A    Row-major input matrix (rows x cols).
 * @param[in]  rows Number of rows in A.
 * @param[in]  cols Number of columns in A.
 * @param[out] B    Row-major output matrix (cols x rows). Must not alias A.
 */
static void mat_transpose(const float *A, size_t rows, size_t cols, float *B)
{
    for (size_t i = 0; i < rows; i++)
        for (size_t j = 0; j < cols; j++)
            B[j * rows + i] = A[i * cols + j];
}

/**
 * @brief Invert a square matrix in-place using Gauss-Jordan elimination.
 *
 * Uses partial pivoting for numerical stability. The augmented matrix
 * workspace is caller-provided to avoid stack allocation.
 *
 * @param[in]  A    Row-major input matrix (n x n).
 * @param[in]  n    Matrix dimension.
 * @param[out] inv  Row-major output inverse matrix (n x n).
 * @param[in,out] aug  Caller-provided workspace, must be n x 2n floats.
 * @return  0 on success, -1 if the matrix is singular.
 */
static int mat_inverse(const float *A, size_t n, float *inv, float *aug)
{
    size_t cols = 2 * n;
    memset(aug, 0, n * cols * sizeof(float));

    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++)
            aug[i * cols + j] = A[i * n + j];
        aug[i * cols + n + i] = 1.0f;
    }

    for (size_t i = 0; i < n; i++) {
        /* partial pivot */
        size_t max_row = i;
        for (size_t k = i + 1; k < n; k++)
            if (fabsf(aug[k * cols + i]) > fabsf(aug[max_row * cols + i]))
                max_row = k;

        if (fabsf(aug[max_row * cols + i]) < 1e-12f)
            return -1;

        if (max_row != i)
            for (size_t j = 0; j < cols; j++) {
                float tmp = aug[i * cols + j];
                aug[i * cols + j] = aug[max_row * cols + j];
                aug[max_row * cols + j] = tmp;
            }

        float pivot = aug[i * cols + i];
        for (size_t j = i; j < cols; j++)
            aug[i * cols + j] /= pivot;

        for (size_t k = 0; k < n; k++) {
            if (k == i) continue;
            float factor = aug[k * cols + i];
            for (size_t j = i; j < cols; j++)
                aug[k * cols + j] -= factor * aug[i * cols + j];
        }
    }

    for (size_t i = 0; i < n; i++)
        for (size_t j = 0; j < n; j++)
            inv[i * n + j] = aug[i * cols + n + j];

    return 0;
}


static TBCI_Status cca_init_fn(TBCI_Node *self, struct TBCI_Context *ctx)
{
    TBCI_CCANode *node = (TBCI_CCANode *)self;

    if (ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    size_t n_frames    = ctx->total_frames;
    size_t n_freqs     = node->config.n_freqs;
    size_t n_harmonics = node->config.n_harmonics;
    float  srate       = ctx->config.target_srate;
    size_t n_components = n_harmonics * 2;

    /* guard: check caller-provided buffer is large enough */
    size_t required = n_freqs * n_components * n_frames;
    if (node->ref_signals_capacity < required)
        return TBCI_ERR_INVALID_ARG;

    /* generate reference signals */
    for (size_t fi = 0; fi < n_freqs; fi++) {
        float freq = node->config.freqs[fi];
        for (size_t h = 1; h <= n_harmonics; h++) {
            size_t sin_comp = (h - 1) * 2;
            size_t cos_comp = (h - 1) * 2 + 1;
            for (size_t fr = 0; fr < n_frames; fr++) {
                node->ref_signals[fi * (n_components * n_frames) + sin_comp * n_frames + fr] = sinf(2 * TBCI_M_PI * h * freq * fr / srate);
                node->ref_signals[fi * (n_components * n_frames) + cos_comp * n_frames + fr] = cosf(2 * TBCI_M_PI * h * freq * fr / srate);
            }
        }
        /* center reference signals once at init — sine/cosine are theoretically
        *  zero-mean over full periods but numerical drift may introduce small offsets */
        float *Y = node->ref_signals + fi * n_components * n_frames;
        mat_center_rows(Y, n_components, n_frames);
    }

    return TBCI_OK;
}

TBCI_API TBCI_Status cca_init(TBCI_CCANode *node, TBCI_CCAConfig *config, float *ref_signals, size_t ref_signals_capacity)
{
    if (node == NULL || config == NULL || ref_signals == NULL || ref_signals_capacity == 0)
        return TBCI_ERR_INVALID_ARG;

    node->base.name = "cca";
    node->base.type = TBCI_NODE_TYPE_FEATURE_EXTRACTION;
    node->base.enabled = true;
    node->base.init_fn = (TBCI_NodeInitFn) cca_init_fn;
    node->base.process_fn = (TBCI_NodeProcessFn) cca_process;
    node->base.reset_fn = (TBCI_NodeResetFn) cca_reset;
    node->base.instance_size = sizeof(TBCI_CCANode);

    node->config = *config;
    node->ref_signals = ref_signals;
    node->ref_signals_capacity = ref_signals_capacity;

    return TBCI_OK;

}

TBCI_NodeResult cca_process(TBCI_CCANode *node, void *data, struct TBCI_Context *ctx)
{
    TBCI_Epoch *epoch = (TBCI_Epoch *)data;

    if (node == NULL || epoch == NULL || ctx == NULL)
        return TBCI_NODE_ERROR;

    size_t n_ch         = epoch->n_channels;
    size_t n_fr         = epoch->n_frames;
    size_t n_freqs      = node->config.n_freqs;
    size_t n_components = node->config.n_harmonics * 2;

    float max_corr      = 0.0f;
    node->best_freq_idx = -1;

    /* 1. center EEG — remove mean per channel (in-place) */
    mat_center_rows(epoch->samples, n_ch, n_fr);

    /* 2. compute Cxx = X * X^T, then scale to covariance, then regularize */
    mat_self_outer(epoch->samples, n_ch, n_fr, &node->Cxx[0][0]);
    mat_scale_covariance(&node->Cxx[0][0], n_ch, n_ch, n_fr);
    for (size_t i = 0; i < n_ch; i++)
        node->Cxx[i][i] += REGULARIZATION;

    /* 3. invert Cxx */
    if (mat_inverse(&node->Cxx[0][0], n_ch, &node->inv_Cxx[0][0], node->aug) != 0)
        return TBCI_NODE_ERROR;

    /* for each target frequency */
    for (size_t fi = 0; fi < n_freqs; fi++) {

        /* get pointer to Y for this frequency */
        float *Y = node->ref_signals + fi * n_components * n_fr;

        /* 5. compute Cyy = Y * Y^T, scale, regularize */
        mat_self_outer(Y, n_components, n_fr, &node->Cyy[0][0]);
        mat_scale_covariance(&node->Cyy[0][0], n_components, n_components, n_fr);
        for (size_t i = 0; i < n_components; i++)
            node->Cyy[i][i] += REGULARIZATION;

        /* 6. invert Cyy */
        if (mat_inverse(&node->Cyy[0][0], n_components, &node->inv_Cyy[0][0], node->aug) != 0) {
            printf("Cyy inversion failed for freq_idx %zu\n", fi);
            continue;
        }
        /* 7. compute Cxy = X * Y^T, scale */
        mat_cross_outer(epoch->samples, Y, n_ch, n_components, n_fr, &node->Cxy[0][0]);
        mat_scale_covariance(&node->Cxy[0][0], n_ch, n_components, n_fr);

        /* 8. compute M = inv_Cxx * Cxy * inv_Cyy * Cxy^T */
        /* use node->M as output, use temp buffers for intermediates */
        mat_mul(&node->inv_Cxx[0][0], &node->Cxy[0][0], &node->temp1[0][0], n_ch, n_ch, n_components);
        mat_mul(&node->temp1[0][0], &node->inv_Cyy[0][0], &node->temp2[0][0], n_ch, n_components, n_components);

        mat_transpose(&node->Cxy[0][0], n_ch, n_components, &node->Cxy_T[0][0]);
        mat_mul(&node->temp2[0][0], &node->Cxy_T[0][0], &node->M[0][0], n_ch, n_components, n_ch);


        /* 9. dominant eigenvalue → canonical correlation */
        float lambda = dominant_eigenvalue(&node->M[0][0], n_ch, &node->eigenvectors[0]);
        float canonical_corr = (lambda > 0.0f) ? sqrtf(lambda) : 0.0f;

        node->correlations[fi] = canonical_corr;

        if (canonical_corr > max_corr) {
            max_corr = canonical_corr;
            node->best_freq_idx = (int)fi;
        }
    }

    /* 10. pack correlations into epoch as feature vector */
    epoch->n_channels = n_freqs;
    epoch->n_frames   = 1;
    for (size_t fi = 0; fi < n_freqs; fi++)
        epoch->samples[fi] = node->correlations[fi];

    return TBCI_NODE_OK;
}

TBCI_API TBCI_Status cca_reset(TBCI_CCANode *node)
{
    if (node == NULL)
        return TBCI_ERR_INVALID_ARG;

    memset(node->correlations, 0, sizeof(node->correlations));
    node->best_freq_idx = -1;
    return TBCI_OK;
}
