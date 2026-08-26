/**
 * @file test_tbci_math.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for tbci_math.c
 */

#include "unity/unity.h"
#include "../include/mathutils/tbci_math.h"
#include <math.h>

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))
#define FLOAT_TOL 1e-5f

/* --------------------------------------------------------------------------
 * setUp / tearDown
 * -------------------------------------------------------------------------- */

void setUp(void) {}
void tearDown(void) {}

/* ============================================================
 * GROUP 1 — tbci_softmax
 * ============================================================ */

void test_softmax_output_sums_to_one(void)
{
    float x[] = {1.0f, 2.0f, 3.0f, 4.0f};
    tbci_softmax(x, ARRAY_LEN(x), 1.0f);
    float sum = 0.0f;
    for (size_t i = 0; i < ARRAY_LEN(x); i++) sum += x[i];
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 1.0f, sum);
}

void test_softmax_all_equal_input_uniform_output(void)
{
    float x[] = {2.0f, 2.0f, 2.0f, 2.0f};
    tbci_softmax(x, ARRAY_LEN(x), 1.0f);
    for (size_t i = 0; i < ARRAY_LEN(x); i++)
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.25f, x[i]);
}

void test_softmax_higher_input_higher_output(void)
{
    float x[] = {1.0f, 2.0f, 3.0f};
    tbci_softmax(x, ARRAY_LEN(x), 1.0f);
    TEST_ASSERT_TRUE(x[0] < x[1]);
    TEST_ASSERT_TRUE(x[1] < x[2]);
}

void test_softmax_temperature_one_standard(void)
{
    float x[] = {1.0f, 2.0f, 3.0f};
    float y[] = {1.0f, 2.0f, 3.0f};
    tbci_softmax(x, ARRAY_LEN(x), 1.0f);
    /* manually compute expected */
    float e0 = expf(1.0f - 3.0f);
    float e1 = expf(2.0f - 3.0f);
    float e2 = expf(3.0f - 3.0f);
    float s  = e0 + e1 + e2;
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, e0/s, x[0]);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, e1/s, x[1]);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, e2/s, x[2]);
    (void)y;
}

void test_softmax_high_temperature_flattens_distribution(void)
{
    float x_hot[] = {1.0f, 2.0f, 3.0f};
    float x_cold[] = {1.0f, 2.0f, 3.0f};
    tbci_softmax(x_hot,  ARRAY_LEN(x_hot),  10.0f);
    tbci_softmax(x_cold, ARRAY_LEN(x_cold),  1.0f);
    /* high temp → winner has lower probability than with low temp */
    TEST_ASSERT_TRUE(x_hot[2] < x_cold[2]);
}

void test_softmax_low_temperature_sharpens_distribution(void)
{
    float x_sharp[] = {1.0f, 2.0f, 3.0f};
    float x_soft[]  = {1.0f, 2.0f, 3.0f};
    tbci_softmax(x_sharp, ARRAY_LEN(x_sharp), 0.1f);
    tbci_softmax(x_soft,  ARRAY_LEN(x_soft),  1.0f);
    /* low temp → winner has higher probability */
    TEST_ASSERT_TRUE(x_sharp[2] > x_soft[2]);
}

void test_softmax_null_input_no_crash(void)
{
    tbci_softmax(NULL, 4, 1.0f);  /* must not crash */
}

void test_softmax_zero_n_no_crash(void)
{
    float x[] = {1.0f, 2.0f};
    tbci_softmax(x, 0, 1.0f);  /* must not crash */
}

void test_softmax_zero_temperature_no_crash(void)
{
    float x[] = {1.0f, 2.0f};
    tbci_softmax(x, ARRAY_LEN(x), 0.0f);  /* must not crash */
}

/* ============================================================
 * GROUP 2 — tbci_normalize_zscore
 * ============================================================ */

void test_zscore_mean_is_zero_after_normalization(void)
{
    float x[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    tbci_normalize_zscore(x, ARRAY_LEN(x));
    float mean = 0.0f;
    for (size_t i = 0; i < ARRAY_LEN(x); i++) mean += x[i];
    mean /= (float)ARRAY_LEN(x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, mean);
}

void test_zscore_std_is_one_after_normalization(void)
{
    float x[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    tbci_normalize_zscore(x, ARRAY_LEN(x));
    float mean = 0.0f;
    for (size_t i = 0; i < ARRAY_LEN(x); i++) mean += x[i];
    mean /= (float)ARRAY_LEN(x);
    float var = 0.0f;
    for (size_t i = 0; i < ARRAY_LEN(x); i++) var += (x[i] - mean) * (x[i] - mean);
    var /= (float)ARRAY_LEN(x);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, sqrtf(var));
}

void test_zscore_uniform_input_no_crash(void)
{
    float x[] = {3.0f, 3.0f, 3.0f};
    tbci_normalize_zscore(x, ARRAY_LEN(x));  /* std=0, must not crash */
}

void test_zscore_null_input_no_crash(void)
{
    tbci_normalize_zscore(NULL, 4);
}

void test_zscore_single_element_no_crash(void)
{
    float x[] = {5.0f};
    tbci_normalize_zscore(x, 1);
}

/* ============================================================
 * GROUP 3 — tbci_normalize_minmax
 * ============================================================ */

void test_minmax_output_min_is_zero(void)
{
    float x[] = {1.0f, 3.0f, 2.0f, 5.0f, 4.0f};
    tbci_normalize_minmax(x, ARRAY_LEN(x));
    float min = x[0];
    for (size_t i = 1; i < ARRAY_LEN(x); i++) if (x[i] < min) min = x[i];
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, min);
}

void test_minmax_output_max_is_one(void)
{
    float x[] = {1.0f, 3.0f, 2.0f, 5.0f, 4.0f};
    tbci_normalize_minmax(x, ARRAY_LEN(x));
    float max = x[0];
    for (size_t i = 1; i < ARRAY_LEN(x); i++) if (x[i] > max) max = x[i];
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 1.0f, max);
}

void test_minmax_uniform_input_no_crash(void)
{
    float x[] = {3.0f, 3.0f, 3.0f};
    tbci_normalize_minmax(x, ARRAY_LEN(x));  /* range=0, must not crash */
}

void test_minmax_null_input_no_crash(void)
{
    tbci_normalize_minmax(NULL, 4);
}

void test_minmax_single_element_no_crash(void)
{
    float x[] = {5.0f};
    tbci_normalize_minmax(x, 1);
}

/* ============================================================
 * GROUP 4 — tbci_argmax
 * ============================================================ */

void test_argmax_returns_correct_index(void)
{
    float x[] = {1.0f, 5.0f, 3.0f, 2.0f};
    TEST_ASSERT_EQUAL_INT(1, tbci_argmax(x, ARRAY_LEN(x)));
}

void test_argmax_first_element_is_max(void)
{
    float x[] = {9.0f, 3.0f, 1.0f};
    TEST_ASSERT_EQUAL_INT(0, tbci_argmax(x, ARRAY_LEN(x)));
}

void test_argmax_last_element_is_max(void)
{
    float x[] = {1.0f, 2.0f, 9.0f};
    TEST_ASSERT_EQUAL_INT(2, tbci_argmax(x, ARRAY_LEN(x)));
}

void test_argmax_null_returns_minus_one(void)
{
    TEST_ASSERT_EQUAL_INT(-1, tbci_argmax(NULL, 4));
}

void test_argmax_zero_n_returns_minus_one(void)
{
    float x[] = {1.0f, 2.0f};
    TEST_ASSERT_EQUAL_INT(-1, tbci_argmax(x, 0));
}

void test_argmax_single_element_returns_zero(void)
{
    float x[] = {42.0f};
    TEST_ASSERT_EQUAL_INT(0, tbci_argmax(x, 1));
}

/* ============================================================
 * GROUP 5 — scorer functions
 * ============================================================ */

/* binary confusion matrix:
 *   predicted:  0   1
 * true 0:      [3,  1]   → 3 TN, 1 FP
 * true 1:      [2,  4]   → 2 FN, 4 TP
 * accuracy = (3+4)/10 = 0.7
 * f1 class0: p=3/5=0.6, r=3/4=0.75 → f1=0.667
 * f1 class1: p=4/5=0.8, r=4/6=0.667 → f1=0.727
 * macro f1 = (0.667+0.727)/2 = 0.697
 */
static size_t binary_confusion[TBCI_MAX_CLASSES][TBCI_MAX_CLASSES];

static void setup_binary_confusion(void)
{
    memset(binary_confusion, 0, sizeof(binary_confusion));
    binary_confusion[0][0] = 3;  /* TN */
    binary_confusion[0][1] = 1;  /* FP */
    binary_confusion[1][0] = 2;  /* FN */
    binary_confusion[1][1] = 4;  /* TP */
}

void test_score_accuracy_binary(void)
{
    setup_binary_confusion();
    float acc = tbci_score_accuracy(binary_confusion, 2);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.7f, acc);
}

void test_score_accuracy_perfect(void)
{
    size_t confusion[TBCI_MAX_CLASSES][TBCI_MAX_CLASSES];
    memset(confusion, 0, sizeof(confusion));
    confusion[0][0] = 5;
    confusion[1][1] = 5;
    float acc = tbci_score_accuracy(confusion, 2);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, acc);
}

void test_score_accuracy_empty_returns_zero(void)
{
    size_t confusion[TBCI_MAX_CLASSES][TBCI_MAX_CLASSES];
    memset(confusion, 0, sizeof(confusion));
    float acc = tbci_score_accuracy(confusion, 2);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, acc);
}

void test_score_f1_binary(void)
{
    setup_binary_confusion();
    float f1 = tbci_score_f1(binary_confusion, 2);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.697f, f1);
}

void test_score_f1_perfect(void)
{
    size_t confusion[TBCI_MAX_CLASSES][TBCI_MAX_CLASSES];
    memset(confusion, 0, sizeof(confusion));
    confusion[0][0] = 5;
    confusion[1][1] = 5;
    float f1 = tbci_score_f1(confusion, 2);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, f1);
}

void test_score_f1_empty_returns_zero(void)
{
    size_t confusion[TBCI_MAX_CLASSES][TBCI_MAX_CLASSES];
    memset(confusion, 0, sizeof(confusion));
    float f1 = tbci_score_f1(confusion, 2);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, f1);
}

void test_score_mcc_binary(void)
{
    setup_binary_confusion();
    float mcc = tbci_score_mcc(binary_confusion, 2);
    /* TP=4,TN=3,FP=1,FN=2 → MCC = (4*3-1*2)/sqrt((4+1)(4+2)(3+1)(3+2)) = 10/sqrt(600) ≈ 0.408 */
    TEST_ASSERT_FLOAT_WITHIN(1e-2f, 0.408f, mcc);
}

void test_score_mcc_perfect(void)
{
    size_t confusion[TBCI_MAX_CLASSES][TBCI_MAX_CLASSES];
    memset(confusion, 0, sizeof(confusion));
    confusion[0][0] = 5;
    confusion[1][1] = 5;
    float mcc = tbci_score_mcc(confusion, 2);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, mcc);
}

void test_score_mcc_empty_returns_zero(void)
{
    size_t confusion[TBCI_MAX_CLASSES][TBCI_MAX_CLASSES];
    memset(confusion, 0, sizeof(confusion));
    float mcc = tbci_score_mcc(confusion, 2);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, mcc);
}

/* ============================================================
 * GROUP 6 — scorer multiclass
 * ============================================================ */

/* 4-class confusion matrix:
 *        pred: 0  1  2  3
 * true 0:     [4, 1, 0, 0]
 * true 1:     [1, 3, 1, 0]
 * true 2:     [0, 1, 4, 1]
 * true 3:     [0, 0, 1, 3]
 *
 * accuracy = (4+3+4+3)/20 = 14/20 = 0.7
 *
 * class 0: tp=4, fp=1, fn=1 → p=4/5=0.8,  r=4/5=0.8,  f1=0.800
 * class 1: tp=3, fp=2, fn=2 → p=3/5=0.6,  r=3/5=0.6,  f1=0.600
 * class 2: tp=4, fp=2, fn=2 → p=4/6=0.667,r=4/6=0.667,f1=0.667
 * class 3: tp=3, fp=1, fn=1 → p=3/4=0.75, r=3/4=0.75, f1=0.750
 * macro f1 = (0.8+0.6+0.667+0.75)/4 = 0.704
 */
static size_t multiclass_confusion[TBCI_MAX_CLASSES][TBCI_MAX_CLASSES];

static void setup_multiclass_confusion(void)
{
    memset(multiclass_confusion, 0, sizeof(multiclass_confusion));
    multiclass_confusion[0][0] = 4; multiclass_confusion[0][1] = 1;
    multiclass_confusion[1][0] = 1; multiclass_confusion[1][1] = 3; multiclass_confusion[1][2] = 1;
    multiclass_confusion[2][1] = 1; multiclass_confusion[2][2] = 4; multiclass_confusion[2][3] = 1;
    multiclass_confusion[3][2] = 1; multiclass_confusion[3][3] = 3;
}

void test_score_accuracy_multiclass(void)
{
    setup_multiclass_confusion();
    float acc = tbci_score_accuracy(multiclass_confusion, 4);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.7f, acc);
}

void test_score_f1_multiclass(void)
{
    setup_multiclass_confusion();
    float f1 = tbci_score_f1(multiclass_confusion, 4);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.704f, f1);
}

void test_score_mcc_multiclass(void)
{
    setup_multiclass_confusion();
    float mcc = tbci_score_mcc(multiclass_confusion, 4);
    /* generalized MCC — no closed form to verify against, just sanity check range */
    TEST_ASSERT_TRUE(mcc > 0.0f && mcc < 1.0f);
}

/* --------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    // Group 1 — softmax
    RUN_TEST(test_softmax_output_sums_to_one);
    RUN_TEST(test_softmax_all_equal_input_uniform_output);
    RUN_TEST(test_softmax_higher_input_higher_output);
    RUN_TEST(test_softmax_temperature_one_standard);
    RUN_TEST(test_softmax_high_temperature_flattens_distribution);
    RUN_TEST(test_softmax_low_temperature_sharpens_distribution);
    RUN_TEST(test_softmax_null_input_no_crash);
    RUN_TEST(test_softmax_zero_n_no_crash);
    RUN_TEST(test_softmax_zero_temperature_no_crash);

    // Group 2 — zscore
    RUN_TEST(test_zscore_mean_is_zero_after_normalization);
    RUN_TEST(test_zscore_std_is_one_after_normalization);
    RUN_TEST(test_zscore_uniform_input_no_crash);
    RUN_TEST(test_zscore_null_input_no_crash);
    RUN_TEST(test_zscore_single_element_no_crash);

    // Group 3 — minmax
    RUN_TEST(test_minmax_output_min_is_zero);
    RUN_TEST(test_minmax_output_max_is_one);
    RUN_TEST(test_minmax_uniform_input_no_crash);
    RUN_TEST(test_minmax_null_input_no_crash);
    RUN_TEST(test_minmax_single_element_no_crash);

    // Group 4 — argmax
    RUN_TEST(test_argmax_returns_correct_index);
    RUN_TEST(test_argmax_first_element_is_max);
    RUN_TEST(test_argmax_last_element_is_max);
    RUN_TEST(test_argmax_null_returns_minus_one);
    RUN_TEST(test_argmax_zero_n_returns_minus_one);
    RUN_TEST(test_argmax_single_element_returns_zero);

    // Group 5 — scorers
    RUN_TEST(test_score_accuracy_binary);
    RUN_TEST(test_score_accuracy_perfect);
    RUN_TEST(test_score_accuracy_empty_returns_zero);
    RUN_TEST(test_score_f1_binary);
    RUN_TEST(test_score_f1_perfect);
    RUN_TEST(test_score_f1_empty_returns_zero);
    RUN_TEST(test_score_mcc_binary);
    RUN_TEST(test_score_mcc_perfect);
    RUN_TEST(test_score_mcc_empty_returns_zero);

    // Group 6 - scorers multiclass
    RUN_TEST(test_score_accuracy_multiclass);
    RUN_TEST(test_score_f1_multiclass);
    RUN_TEST(test_score_mcc_multiclass);

    return UNITY_END();
}