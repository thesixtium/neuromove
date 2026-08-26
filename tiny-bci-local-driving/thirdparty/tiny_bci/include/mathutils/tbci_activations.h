/**
 * @file tbci_activations.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Activation functions and their derivatives for TinyBCI neural network nodes.
 *
 * All functions operate in-place on float arrays. No dynamic allocation.
 * Shared across TBCI_LinearLayer, TBCI_SequentialClassifier, and any future
 * neural network components.
 */

#ifndef TBCI_ACTIVATIONS_H
#define TBCI_ACTIVATIONS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Activation function enum
 * -------------------------------------------------------------------------- */

typedef enum {
    TBCI_ACT_IDENTITY = 0,  /**< f(x) = x                          */
    TBCI_ACT_SIGMOID,       /**< f(x) = 1 / (1 + exp(-x))          */
    TBCI_ACT_TANH,          /**< f(x) = tanh(x)                    */
    TBCI_ACT_RELU,          /**< f(x) = max(0, x)                  */
    TBCI_ACT_SOFTMAX,       /**< f(x_i) = exp(x_i) / sum(exp(x))  */
} TBCI_ActivationFn;


/**
 * @brief Returns human-readable name for an activation function.
 * Used in warning messages.
 */
const char *tbci_activation_name(TBCI_ActivationFn act);

/* --------------------------------------------------------------------------
 * Forward activations
 * -------------------------------------------------------------------------- */

/**
 * @brief Identity activation. f(x) = x.
 * @param[in,out] x  Float array of length n. Modified in-place.
 * @param[in]     n  Number of elements.
 */
void tbci_act_identity(float *x, size_t n);

/**
 * @brief Sigmoid activation. f(x) = 1 / (1 + exp(-x)).
 * @param[in,out] x  Float array of length n. Modified in-place.
 * @param[in]     n  Number of elements.
 */
void tbci_act_sigmoid(float *x, size_t n);

/**
 * @brief Tanh activation. f(x) = tanh(x).
 * @param[in,out] x  Float array of length n. Modified in-place.
 * @param[in]     n  Number of elements.
 */
void tbci_act_tanh(float *x, size_t n);

/**
 * @brief ReLU activation. f(x) = max(0, x).
 * @param[in,out] x  Float array of length n. Modified in-place.
 * @param[in]     n  Number of elements.
 */
void tbci_act_relu(float *x, size_t n);

/**
 * @brief Softmax activation. f(x_i) = exp(x_i) / sum(exp(x)).
 *
 * Numerically stable — subtracts max before exponentiation.
 *
 * @param[in,out] x  Float array of length n. Modified in-place.
 * @param[in]     n  Number of elements.
 */
void tbci_act_softmax(float *x, size_t n);

/**
 * @brief Apply activation function by enum value.
 *
 * Dispatches to the correct activation function. Returns TBCI_ERR_INVALID_ARG
 * on unknown activation type.
 *
 * @param[in,out] x    Float array of length n. Modified in-place.
 * @param[in]     n    Number of elements.
 * @param[in]     act  Activation function to apply.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if activation type is unknown.
 */
int tbci_act_apply(float *x, size_t n, TBCI_ActivationFn act);

/* --------------------------------------------------------------------------
 * Derivatives (for backprop)
 * -------------------------------------------------------------------------- */

/**
 * @brief Derivative of identity. d/dx = 1.
 * @param[in]  x    Pre-activation values. Length n.
 * @param[in]  y    Post-activation values. Length n.
 * @param[out] dx   Output derivatives. Length n.
 * @param[in]  n    Number of elements.
 */
void tbci_dact_identity(const float *x, const float *y, float *dx, size_t n);

/**
 * @brief Derivative of sigmoid. d/dx = y * (1 - y).
 * @param[in]  x    Pre-activation values. Length n.
 * @param[in]  y    Post-activation values. Length n.
 * @param[out] dx   Output derivatives. Length n.
 * @param[in]  n    Number of elements.
 */
void tbci_dact_sigmoid(const float *x, const float *y, float *dx, size_t n);

/**
 * @brief Derivative of tanh. d/dx = 1 - y^2.
 * @param[in]  x    Pre-activation values. Length n.
 * @param[in]  y    Post-activation values. Length n.
 * @param[out] dx   Output derivatives. Length n.
 * @param[in]  n    Number of elements.
 */
void tbci_dact_tanh(const float *x, const float *y, float *dx, size_t n);

/**
 * @brief Derivative of ReLU. d/dx = 1 if x > 0, 0 otherwise.
 *
 * At x == 0, derivative is set to 0 (subgradient choice).
 *
 * @param[in]  x    Pre-activation values. Length n.
 * @param[in]  y    Post-activation values. Length n.
 * @param[out] dx   Output derivatives. Length n.
 * @param[in]  n    Number of elements.
 */
void tbci_dact_relu(const float *x, const float *y, float *dx, size_t n);

/**
 * @brief Derivative of softmax (diagonal of Jacobian). d/dx_i = y_i * (1 - y_i).
 *
 * Note: this is the diagonal approximation, sufficient for cross-entropy loss
 * where the full Jacobian is not needed.
 *
 * @param[in]  x    Pre-activation values. Length n.
 * @param[in]  y    Post-activation values. Length n.
 * @param[out] dx   Output derivatives. Length n.
 * @param[in]  n    Number of elements.
 */
void tbci_dact_softmax(const float *x, const float *y, float *dx, size_t n);

/**
 * @brief Apply activation derivative by enum value.
 *
 * @param[in]  x    Pre-activation values. Length n.
 * @param[in]  y    Post-activation values. Length n.
 * @param[out] dx   Output derivatives. Length n.
 * @param[in]  n    Number of elements.
 * @param[in]  act  Activation function whose derivative to apply.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if activation type is unknown.
 */
int tbci_dact_apply(const float *x, const float *y, float *dx, size_t n, TBCI_ActivationFn act);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_ACTIVATIONS_H */