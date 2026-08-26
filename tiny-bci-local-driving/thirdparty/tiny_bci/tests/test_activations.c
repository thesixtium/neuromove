/**
 * @file test_activations.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Unit tests for tbci_activations.h
 */

#include "unity/unity.h"
#include "mathutils/tbci_activations.h"
#include <math.h>
#include <string.h>

#define FLOAT_TOL  1e-5f
#define N          4

void setUp(void)  {}
void tearDown(void) {}

/* ============================================================
 * GROUP 1 — tbci_act_identity
 * ============================================================ */

void test_identity_unchanged(void)
{
    float x[] = {-2.0f, 0.0f, 1.0f, 3.5f};
    float expected[] = {-2.0f, 0.0f, 1.0f, 3.5f};
    tbci_act_identity(x, N);
    for (size_t i = 0; i < N; i++)
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, expected[i], x[i]);
}

void test_identity_null_no_crash(void)
{
    tbci_act_identity(NULL, N);
}

void test_identity_zero_n_no_crash(void)
{
    float x[] = {1.0f, 2.0f};
    tbci_act_identity(x, 0);
}

/* ============================================================
 * GROUP 2 — tbci_act_sigmoid
 * ============================================================ */

void test_sigmoid_zero_gives_half(void)
{
    float x[] = {0.0f};
    tbci_act_sigmoid(x, 1);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.5f, x[0]);
}

void test_sigmoid_large_positive_near_one(void)
{
    float x[] = {100.0f};
    tbci_act_sigmoid(x, 1);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 1.0f, x[0]);
}

void test_sigmoid_large_negative_near_zero(void)
{
    float x[] = {-100.0f};
    tbci_act_sigmoid(x, 1);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, x[0]);
}

void test_sigmoid_output_in_range(void)
{
    float x[] = {-3.0f, -1.0f, 0.0f, 1.0f, 3.0f};
    tbci_act_sigmoid(x, 5);
    for (size_t i = 0; i < 5; i++) {
        TEST_ASSERT_TRUE(x[i] > 0.0f);
        TEST_ASSERT_TRUE(x[i] < 1.0f);
    }
}

void test_sigmoid_null_no_crash(void)
{
    tbci_act_sigmoid(NULL, N);
}

/* ============================================================
 * GROUP 3 — tbci_act_tanh
 * ============================================================ */

void test_tanh_zero_gives_zero(void)
{
    float x[] = {0.0f};
    tbci_act_tanh(x, 1);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, x[0]);
}

void test_tanh_output_in_range(void)
{
    float x[] = {-3.0f, -1.0f, 0.0f, 1.0f, 3.0f};
    tbci_act_tanh(x, 5);
    for (size_t i = 0; i < 5; i++) {
        TEST_ASSERT_TRUE(x[i] > -1.0f);
        TEST_ASSERT_TRUE(x[i] < 1.0f);
    }
}

void test_tanh_antisymmetric(void)
{
    float pos[] = {1.0f};
    float neg[] = {-1.0f};
    tbci_act_tanh(pos, 1);
    tbci_act_tanh(neg, 1);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, -pos[0], neg[0]);
}

void test_tanh_null_no_crash(void)
{
    tbci_act_tanh(NULL, N);
}

/* ============================================================
 * GROUP 4 — tbci_act_relu
 * ============================================================ */

void test_relu_positive_unchanged(void)
{
    float x[] = {1.0f, 2.0f, 3.0f};
    float expected[] = {1.0f, 2.0f, 3.0f};
    tbci_act_relu(x, 3);
    for (size_t i = 0; i < 3; i++)
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, expected[i], x[i]);
}

void test_relu_negative_zeroed(void)
{
    float x[] = {-1.0f, -2.0f, -3.0f};
    tbci_act_relu(x, 3);
    for (size_t i = 0; i < 3; i++)
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, x[i]);
}

void test_relu_zero_unchanged(void)
{
    float x[] = {0.0f};
    tbci_act_relu(x, 1);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, x[0]);
}

void test_relu_mixed(void)
{
    float x[] = {-2.0f, 0.0f, 3.0f};
    tbci_act_relu(x, 3);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, x[0]);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, x[1]);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 3.0f, x[2]);
}

void test_relu_null_no_crash(void)
{
    tbci_act_relu(NULL, N);
}

/* ============================================================
 * GROUP 5 — tbci_act_softmax
 * ============================================================ */

void test_softmax_sums_to_one(void)
{
    float x[] = {1.0f, 2.0f, 3.0f, 4.0f};
    tbci_act_softmax(x, N);
    float sum = 0.0f;
    for (size_t i = 0; i < N; i++) sum += x[i];
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 1.0f, sum);
}

void test_softmax_max_input_has_max_output(void)
{
    float x[] = {1.0f, 5.0f, 2.0f, 3.0f};
    tbci_act_softmax(x, N);
    /* index 1 had max input, should have max output */
    for (size_t i = 0; i < N; i++)
        if (i != 1)
            TEST_ASSERT_TRUE(x[1] > x[i]);
}

void test_softmax_uniform_input_uniform_output(void)
{
    float x[] = {2.0f, 2.0f, 2.0f, 2.0f};
    tbci_act_softmax(x, N);
    for (size_t i = 0; i < N; i++)
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.25f, x[i]);
}

void test_softmax_numerically_stable_large_input(void)
{
    float x[] = {1000.0f, 1001.0f, 1002.0f};
    tbci_act_softmax(x, 3);
    float sum = 0.0f;
    for (size_t i = 0; i < 3; i++) sum += x[i];
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, sum);
    /* no NaN */
    for (size_t i = 0; i < 3; i++)
        TEST_ASSERT_FALSE(x[i] != x[i]);
}

void test_softmax_null_no_crash(void)
{
    tbci_act_softmax(NULL, N);
}

/* ============================================================
 * GROUP 6 — tbci_act_apply dispatch
 * ============================================================ */

void test_act_apply_identity(void)
{
    float x[] = {1.0f, 2.0f};
    float expected[] = {1.0f, 2.0f};
    tbci_act_apply(x, 2, TBCI_ACT_IDENTITY);
    for (size_t i = 0; i < 2; i++)
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, expected[i], x[i]);
}

void test_act_apply_sigmoid(void)
{
    float x[] = {0.0f};
    tbci_act_apply(x, 1, TBCI_ACT_SIGMOID);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.5f, x[0]);
}

void test_act_apply_invalid_returns_error(void)
{
    float x[] = {1.0f};
    int s = tbci_act_apply(x, 1, (TBCI_ActivationFn)99);
    TEST_ASSERT_NOT_EQUAL(0, s);  /* TBCI_ERR_INVALID_ARG */
}

/* ============================================================
 * GROUP 7 — derivatives
 * ============================================================ */

void test_dact_identity_all_ones(void)
{
    float x[]  = {-1.0f, 0.0f, 1.0f, 2.0f};
    float y[]  = {-1.0f, 0.0f, 1.0f, 2.0f};
    float dx[N] = {0};
    tbci_dact_identity(x, y, dx, N);
    for (size_t i = 0; i < N; i++)
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 1.0f, dx[i]);
}

void test_dact_sigmoid_formula(void)
{
    /* d/dx sigmoid = y * (1 - y) */
    float x[] = {0.0f};
    float y[] = {0.5f};  /* sigmoid(0) = 0.5 */
    float dx[1] = {0};
    tbci_dact_sigmoid(x, y, dx, 1);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.25f, dx[0]);  /* 0.5 * 0.5 */
}

void test_dact_tanh_formula(void)
{
    /* d/dx tanh = 1 - y^2 */
    float x[] = {0.0f};
    float y[] = {0.0f};  /* tanh(0) = 0 */
    float dx[1] = {0};
    tbci_dact_tanh(x, y, dx, 1);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 1.0f, dx[0]);  /* 1 - 0^2 = 1 */
}

void test_dact_relu_positive_gives_one(void)
{
    float x[] = {1.0f, 2.0f};
    float y[] = {1.0f, 2.0f};
    float dx[2] = {0};
    tbci_dact_relu(x, y, dx, 2);
    for (size_t i = 0; i < 2; i++)
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 1.0f, dx[i]);
}

void test_dact_relu_negative_gives_zero(void)
{
    float x[] = {-1.0f, -2.0f};
    float y[] = {0.0f,   0.0f};
    float dx[2] = {0};
    tbci_dact_relu(x, y, dx, 2);
    for (size_t i = 0; i < 2; i++)
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOL, 0.0f, dx[i]);
}

void test_dact_apply_invalid_returns_error(void)
{
    float x[] = {1.0f};
    float y[] = {1.0f};
    float dx[1] = {0};
    int s = tbci_dact_apply(x, y, dx, 1, (TBCI_ActivationFn)99);
    TEST_ASSERT_NOT_EQUAL(0, s);
}

/* ============================================================
 * GROUP 8 — tbci_activation_name
 * ============================================================ */

void test_activation_name_known(void)
{
    TEST_ASSERT_NOT_NULL(tbci_activation_name(TBCI_ACT_RELU));
    TEST_ASSERT_NOT_NULL(tbci_activation_name(TBCI_ACT_SIGMOID));
    TEST_ASSERT_NOT_NULL(tbci_activation_name(TBCI_ACT_SOFTMAX));
    TEST_ASSERT_NOT_NULL(tbci_activation_name(TBCI_ACT_TANH));
    TEST_ASSERT_NOT_NULL(tbci_activation_name(TBCI_ACT_IDENTITY));
}

void test_activation_name_unknown_not_null(void)
{
    /* should return a fallback string, not NULL */
    TEST_ASSERT_NOT_NULL(tbci_activation_name((TBCI_ActivationFn)99));
}

/* --------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    // Group 1 — identity
    RUN_TEST(test_identity_unchanged);
    RUN_TEST(test_identity_null_no_crash);
    RUN_TEST(test_identity_zero_n_no_crash);

    // Group 2 — sigmoid
    RUN_TEST(test_sigmoid_zero_gives_half);
    RUN_TEST(test_sigmoid_large_positive_near_one);
    RUN_TEST(test_sigmoid_large_negative_near_zero);
    RUN_TEST(test_sigmoid_output_in_range);
    RUN_TEST(test_sigmoid_null_no_crash);

    // Group 3 — tanh
    RUN_TEST(test_tanh_zero_gives_zero);
    RUN_TEST(test_tanh_output_in_range);
    RUN_TEST(test_tanh_antisymmetric);
    RUN_TEST(test_tanh_null_no_crash);

    // Group 4 — relu
    RUN_TEST(test_relu_positive_unchanged);
    RUN_TEST(test_relu_negative_zeroed);
    RUN_TEST(test_relu_zero_unchanged);
    RUN_TEST(test_relu_mixed);
    RUN_TEST(test_relu_null_no_crash);

    // Group 5 — softmax
    RUN_TEST(test_softmax_sums_to_one);
    RUN_TEST(test_softmax_max_input_has_max_output);
    RUN_TEST(test_softmax_uniform_input_uniform_output);
    RUN_TEST(test_softmax_numerically_stable_large_input);
    RUN_TEST(test_softmax_null_no_crash);

    // Group 6 — apply dispatch
    RUN_TEST(test_act_apply_identity);
    RUN_TEST(test_act_apply_sigmoid);
    RUN_TEST(test_act_apply_invalid_returns_error);

    // Group 7 — derivatives
    RUN_TEST(test_dact_identity_all_ones);
    RUN_TEST(test_dact_sigmoid_formula);
    RUN_TEST(test_dact_tanh_formula);
    RUN_TEST(test_dact_relu_positive_gives_one);
    RUN_TEST(test_dact_relu_negative_gives_zero);
    RUN_TEST(test_dact_apply_invalid_returns_error);

    // Group 8 — names
    RUN_TEST(test_activation_name_known);
    RUN_TEST(test_activation_name_unknown_not_null);

    return UNITY_END();
}