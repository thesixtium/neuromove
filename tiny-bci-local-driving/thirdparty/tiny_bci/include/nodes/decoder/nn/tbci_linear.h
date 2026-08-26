/**
 * @file tbci_linear.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Single fully-connected linear layer for TinyBCI neural network nodes.
 *
 * Implements a linear transformation: output = activation(input @ weights + bias)
 *
 * Owns weights, biases, gradient buffers and cached activations for backprop.
 * No dynamic allocation — all storage is fixed-size arrays sized by build-time
 * defines. Used as a building block by TBCI_SequentialClassifier.
 *
 * ## Memory
 *
 * With default defines (128 neurons):
 *   weights + bias:       128 * 128 * 4 + 128 * 4 = 65.5 KB per layer
 *   grad_w + grad_b:      65.5 KB per layer
 *   pre/post activation:  128 * 4 * 2 = 1 KB per layer
 *
 * Override at build time:
 *   -DTBCI_MAX_SEQ_NEURONS=64
 */

#ifndef TBCI_LINEAR_H
#define TBCI_LINEAR_H

#include "tbci_common.h"
#include "mathutils/tbci_activations.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Build-time defines
 * -------------------------------------------------------------------------- */

/** Maximum number of neurons per layer. Override at build time. */
#ifndef TBCI_MAX_SEQ_NEURONS
#define TBCI_MAX_SEQ_NEURONS 128
#endif

/* --------------------------------------------------------------------------
 * Struct
 * -------------------------------------------------------------------------- */

/**
 * @brief Single fully-connected linear layer.
 *
 * weights[i][j] connects input neuron i to output neuron j.
 * pre_act caches the pre-activation values from the forward pass,
 * needed by backprop to compute activation derivatives.
 * post_act caches the post-activation values (layer output).
 */
typedef struct {
    float weights[TBCI_MAX_SEQ_NEURONS][TBCI_MAX_SEQ_NEURONS]; /**< Weight matrix [input][output].          */
    float bias[TBCI_MAX_SEQ_NEURONS];                           /**< Bias vector [output].                   */
    float grad_w[TBCI_MAX_SEQ_NEURONS][TBCI_MAX_SEQ_NEURONS];  /**< Weight gradients, accumulated per batch.*/
    float grad_b[TBCI_MAX_SEQ_NEURONS];                         /**< Bias gradients, accumulated per batch.  */
    float pre_act[TBCI_MAX_SEQ_NEURONS];                        /**< Pre-activation cache for backprop.      */
    float post_act[TBCI_MAX_SEQ_NEURONS];                       /**< Post-activation cache (layer output).   */
    size_t          input_size;                                  /**< Number of input neurons.                */
    size_t          output_size;                                 /**< Number of output neurons.               */
    TBCI_ActivationFn activation;                                  /**< Activation function for this layer.     */
} TBCI_LinearLayer;

typedef enum {
    TBCI_INIT_DEFAULT = 0,  /**< Auto-select based on activation. Logs a warning. */
    TBCI_INIT_ZEROS,        /**< All weights set to zero. Useful for bias init or debugging. */
    TBCI_INIT_XAVIER,       /**< Xavier uniform: U[-sqrt(6/(in+out)), sqrt(6/(in+out))].
                             Recommended for sigmoid and tanh activations.               */
    TBCI_INIT_HE,           /**< He uniform: U[-sqrt(6/in), sqrt(6/in)].
                             Recommended for ReLU activations.                           */
} TBCI_WeightInit;

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialise a linear layer.
 *
 * Zeros weights, biases and gradients. Sets input/output sizes and activation.
 * Weights should be initialized separately via tbci_linear_init_weights.
 *
 * @param[out] layer        Must not be NULL.
 * @param[in]  input_size   Number of input neurons. Must be <= TBCI_MAX_SEQ_NEURONS.
 * @param[in]  output_size  Number of output neurons. Must be <= TBCI_MAX_SEQ_NEURONS.
 * @param[in]  activation   Activation function for this layer.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if layer is NULL or sizes exceed build-time limits.
 */
TBCI_API TBCI_Status tbci_linear_init(TBCI_LinearLayer *layer,
                                       size_t input_size,
                                       size_t output_size,
                                       TBCI_ActivationFn activation);

/**
 * @brief Initialize weights using Xavier uniform initialization.
 *
 * Draws weights from U[-epsilon, epsilon] where
 * epsilon = sqrt(6 / (input_size + output_size)).
 * Biases initialized to zero.
 *
 * @param[in,out] layer  Must not be NULL and must be initialized via tbci_linear_init.
 * @param[in]     init   The initialization method to use.
 * @param[in]     seed   Random seed. Pass 0 to use time-based seed.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if layer is NULL.
 */
TBCI_API TBCI_Status tbci_linear_init_weights(TBCI_LinearLayer *layer, TBCI_WeightInit init, uint32_t seed);

/**
 * @brief Forward pass through the linear layer.
 *
 * Computes pre_act = input @ weights + bias, then applies activation in-place.
 * Caches pre_act and post_act for use by tbci_linear_backward.
 *
 * @param[in,out] layer   Must not be NULL.
 * @param[in]     input   Input array of length input_size. Must not be NULL.
 * @param[out]    output  Output array of length output_size. Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status tbci_linear_forward(TBCI_LinearLayer *layer, const float *input, float *output);

/**
 * @brief Backward pass through the linear layer.
 *
 * Computes gradients of loss w.r.t. weights, biases and input.
 * Accumulates grad_w and grad_b — call tbci_linear_zero_grad before
 * each new sample or batch.
 *
 * @param[in,out] layer       Must not be NULL.
 * @param[in]     input       Input to this layer from forward pass. Must not be NULL.
 * @param[in]     grad_output Gradient from the next layer. Length output_size.
 * @param[out]    grad_input  Gradient to propagate to previous layer. Length input_size.
 * @param[in]     grad_clip   Max gradient norm. 0.0f disables clipping.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL.
 */
TBCI_API TBCI_Status tbci_linear_backward(TBCI_LinearLayer *layer,
                                           const float *input,
                                           const float *grad_output,
                                           float *grad_input,
                                           float grad_clip);

/**
 * @brief Update weights using SGD.
 *
 * weights -= learning_rate * grad_w
 * bias    -= learning_rate * grad_b
 *
 * @param[in,out] layer          Must not be NULL.
 * @param[in]     learning_rate  Step size. Must be > 0.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if layer is NULL or learning_rate <= 0.
 */
TBCI_API TBCI_Status tbci_linear_update_sgd(TBCI_LinearLayer *layer, float learning_rate);

/**
 * @brief Zero gradient buffers.
 *
 * Must be called before each new sample or batch to avoid gradient accumulation
 * across samples.
 *
 * @param[in,out] layer  Must not be NULL.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if layer is NULL.
 */
TBCI_API TBCI_Status tbci_linear_zero_grad(TBCI_LinearLayer *layer);

/**
 * @brief Save layer weights and biases to a caller-provided buffer.
 *
 * Serializes weights then biases as raw floats.
 * Buffer must be at least tbci_linear_weight_size(layer) bytes.
 *
 * @param[in]  layer   Must not be NULL.
 * @param[out] buf     Caller-provided buffer. Must not be NULL.
 * @param[in]  len     Buffer length in bytes.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL or buf too small.
 */
TBCI_API TBCI_Status tbci_linear_save(const TBCI_LinearLayer *layer, float *buf, size_t len);

/**
 * @brief Load layer weights and biases from a caller-provided buffer.
 *
 * @param[in,out] layer  Must not be NULL.
 * @param[in]     buf    Caller-provided buffer. Must not be NULL.
 * @param[in]     len    Buffer length in bytes.
 * @return TBCI_OK on success.
 * @return TBCI_ERR_INVALID_ARG if any pointer is NULL or buf too small.
 */
TBCI_API TBCI_Status tbci_linear_load(TBCI_LinearLayer *layer, const float *buf, size_t len);

/**
 * @brief Returns the number of bytes needed to serialize this layer's weights.
 *
 * @param[in] layer  Must not be NULL.
 * @return Size in bytes, or 0 if layer is NULL.
 */
size_t tbci_linear_weight_size(const TBCI_LinearLayer *layer);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_LINEAR_H */