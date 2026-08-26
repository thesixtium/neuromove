/**
 * @file tbci_math.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Math support functions for TinyBCI.
 */

#include "../../include/mathutils/tbci_math.h"

void tbci_softmax(float *x, size_t n, float temperature)
{
    if (x == NULL || n == 0 || temperature <= 0.0f) return;

    /* find max */
    float max = x[0];
    for (size_t i = 0; i < n; i++)
        if (x[i] > max) max = x[i];

    /* subtract max then divide by temperature for numerical stability */
    float sum = 0.0f;
    for (size_t i = 0; i < n; i++) {
        x[i] = expf((x[i] - max) / temperature);
        sum  += x[i];
    }

    for (size_t i = 0; i < n; i++)
        x[i] /= sum;
}

void tbci_normalize_zscore(float *x, size_t n)
{
    if (x == NULL || n == 0) return;

    float mean = 0.0f;
    for (size_t i = 0; i < n; i++)
        mean += x[i];
    mean /= (float)n;

    float var = 0.0f;
    for (size_t i = 0; i < n; i++)
        var += (x[i] - mean) * (x[i] - mean);
    var /= (float)n;

    float std = sqrtf(var);

    for (size_t i = 0; i < n; i++)
        x[i] = (x[i] - mean) / (std + TBCI_MATH_EPSILON);

}

void tbci_normalize_minmax(float *x, size_t n)
{
    if (x == NULL || n == 0) return;

    float min = x[0];
    float max = x[0];

    for (size_t i = 0; i < n; i++)
    {
        if (x[i] < min) min = x[i];
        if (x[i] > max) max = x[i];
    }

    float range = max - min;

    if (range == 0.0f)
        for (size_t i = 0; i < n; i++)
            x[i] = min;
    else
        for (size_t i = 0; i < n; i++)
            x[i] = (x[i] - min) / range;
}

int tbci_argmax(const float *x, size_t n)
{
    if (x == NULL || n == 0) return -1;

    float max = x[0];
    int index = 0;
    for (size_t i = 0; i < n; i++)
    {
        if (x[i] > max)
        {
            max = x[i];
            index = (int) i;
        }
    }
    return index;
}

float tbci_score_accuracy(const size_t confusion[][TBCI_MAX_CLASSES], size_t n_classes)
{
    size_t correct = 0;
    size_t total   = 0;

    for (size_t i = 0; i < n_classes; i++) {
        for (size_t j = 0; j < n_classes; j++) {
            if (i == j) correct += confusion[i][j];
            total += confusion[i][j];
        }
    }

    return total > 0 ? (float)correct / (float)total : 0.0f;
}

float tbci_score_f1(const size_t confusion[][TBCI_MAX_CLASSES], size_t n_classes)
{
    float f1_sum = 0.0f;

    for (size_t c = 0; c < n_classes; c++) {
        size_t tp = confusion[c][c];

        size_t fp = 0;
        for (size_t i = 0; i < n_classes; i++)
            if (i != c) fp += confusion[i][c];

        size_t fn = 0;
        for (size_t j = 0; j < n_classes; j++)
            if (j != c) fn += confusion[c][j];

        float precision = (tp + fp) > 0 ? (float)tp / (float)(tp + fp) : 0.0f;
        float recall    = (tp + fn) > 0 ? (float)tp / (float)(tp + fn) : 0.0f;

        float denom = precision + recall;
        f1_sum += denom > 0.0f ? 2.0f * precision * recall / denom : 0.0f;
    }

    return n_classes > 0 ? f1_sum / (float)n_classes : 0.0f;
}

float tbci_score_mcc(const size_t confusion[][TBCI_MAX_CLASSES], size_t n_classes)
{
    /* multiclass MCC via generalized formula */
    size_t total    = 0;
    size_t sum_diag = 0;
    size_t sum_pk2  = 0;  /* sum of squared column sums (predicted per class) */
    size_t sum_tk2  = 0;  /* sum of squared row sums (true per class)         */

    for (size_t c = 0; c < n_classes; c++) {
        size_t row_sum = 0;
        size_t col_sum = 0;

        for (size_t j = 0; j < n_classes; j++) row_sum += confusion[c][j];
        for (size_t i = 0; i < n_classes; i++) col_sum += confusion[i][c];

        sum_diag += confusion[c][c];
        sum_pk2  += col_sum * col_sum;
        sum_tk2  += row_sum * row_sum;
        total    += row_sum;
    }

    float num   = (float)(total * sum_diag) - (float)(sum_pk2);  /* simplified — see below */
    float denom = sqrtf((float)(total * total - sum_pk2) *
                        (float)(total * total - sum_tk2));

    return denom > 0.0f ? num / denom : 0.0f;
}