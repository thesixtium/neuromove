#include "../../../../include/nodes/decoder/nn/tbci_mlp_classifier.h"

#define max(x, y) (x > y ? x : y)

void mat_mul(float* a, float** b, float* result, int n, int p) {
    // matrix a of size 1 x n (array)
    // matrix b of size n x p
    // matrix result of size 1 x p (array)
    // result = a * b
    int j, k;
    for (j = 0; j < p; j++) {
        result[j] = 0.0;
        for (k = 0; k < n; k++)
            result[j] += (a[k] * b[k][j]);
    }
}

void identity(int n, float* input, float* output) {
    output[0] = 1; // Bias term

    int i;
    for (i = 0; i < n; i++)
        output[i+1] = input[i]; // Identity function
}

void sigmoid(int n, float* input, float* output) {
    output[0] = 1; // Bias term

    int i;
    for (i = 0; i < n; i++)
        output[i+1] = 1.0 / (1.0 + exp(-input[i])); // Sigmoid function
}

void tan_h(int n, float* input, float* output) {
    output[0] = 1; // Bias term

    int i;
    for (i = 0; i < n; i++)
        output[i+1] = tanh(input[i]); // tanh function
}

void relu(int n, float* input, float* output) {
    output[0] = 1; // Bias term

    int i;
    for (i = 0; i < n; i++)
        output[i+1] = max(0.0, input[i]); // ReLU function
}

void softmax(int n, float* input, float* output) {
    output[0] = 1; // Bias term

    int i;
    float sum = 0.0;
    for (i = 0; i < n; i++)
        sum += exp(input[i]);

    for (i = 0; i < n; i++)
        output[i+1] = exp(input[i]) / sum; // Softmax function
}


void d_identity(int layer_size, float* layer_input, float* layer_output, float* layer_derivative) {
    int i;
    for (i = 0; i < layer_size; i++)
        layer_derivative[i] = 1;
}

void d_sigmoid(int layer_size, float* layer_input, float* layer_output, float* layer_derivative) {
    int i;
    for (i = 0; i < layer_size; i++)
        layer_derivative[i] = layer_output[i+1] * (1.0 - layer_output[i+1]);
}

void d_tanh(int layer_size, float* layer_input, float* layer_output, float* layer_derivative) {
    int i;
    for (i = 0; i < layer_size; i++)
        layer_derivative[i] = 1.0 - layer_output[i+1] * layer_output[i+1];
}

void d_relu(int layer_size, float* layer_input, float* layer_output, float* layer_derivative) {
    int i;
    for (i = 0; i < layer_size; i++) {
        if (layer_input[i] > 0)
            layer_derivative[i] = 1;
        else if (layer_input[i] < 0)
            layer_derivative[i] = 0;
        else // derivative does not exist
            layer_derivative[i] = 0.5; // giving arbitrary value
    }
}

void d_softmax(int layer_size, float* layer_input, float* layer_output, float* layer_derivative) {
    int i;
    for (i = 0; i < layer_size; i++)
        layer_derivative[i] = layer_output[i+1] * (1.0 - layer_output[i+1]);
}

void calculate_local_gradient(parameters* param, int layer_no, int n_layers, int* layer_sizes, float** layer_inputs, float** layer_outputs,
    float* expected_output, float** local_gradient, float** layer_derivatives, float* error_output) {
    int i = 0;

    // If output layer
    if (layer_no == n_layers-1) {
        for (i = 0; i < param->output_layer_size; i++)
            error_output[i] = expected_output[i] - layer_outputs[layer_no][i+1];

        // Calculate the layer derivatives
        // Calculate the local gradients
        switch(param->output_activation_function) {
            case 1: // identity
                d_identity(param->output_layer_size, layer_inputs[layer_no], layer_outputs[layer_no], layer_derivatives[layer_no]);

                for (i = 0; i < param->output_layer_size; i++)
                    local_gradient[layer_no][i] = error_output[i] * layer_derivatives[layer_no][i];

                break;
            case 2: // sigmoid
                d_sigmoid(param->output_layer_size, layer_inputs[layer_no], layer_outputs[layer_no], layer_derivatives[layer_no]);

                for (i = 0; i < param->output_layer_size; i++)
                    local_gradient[layer_no][i] = error_output[i] * layer_derivatives[layer_no][i];

                break;
            case 3: // tanh
                d_tanh(param->output_layer_size, layer_inputs[layer_no], layer_outputs[layer_no], layer_derivatives[layer_no]);

                for (i = 0; i < param->output_layer_size; i++)
                    local_gradient[layer_no][i] = error_output[i] * layer_derivatives[layer_no][i];

                break;
            case 4: // relu
                d_relu(param->output_layer_size, layer_inputs[layer_no], layer_outputs[layer_no], layer_derivatives[layer_no]);

                for (i = 0; i < param->output_layer_size; i++)
                    local_gradient[layer_no][i] = error_output[i] * layer_derivatives[layer_no][i];

                break;
            case 5: // softmax
                d_softmax(param->output_layer_size, layer_inputs[layer_no], layer_outputs[layer_no], layer_derivatives[layer_no]);

                for (i = 0; i < param->output_layer_size; i++)
                    local_gradient[layer_no][i] = error_output[i] * layer_derivatives[layer_no][i];

                break;
            default:
                printf("Calculate local gradient: Invalid output activation function\n");
                exit(0);
                break;
        }
    }
    else { // If hidden layer
        // Calculate the layer derivative for all units in the layer
        // Calculate local gradient
        int j;
        switch (param->hidden_activation_functions[layer_no-1]) {
            case 1: // identity
                d_identity(layer_sizes[layer_no], layer_inputs[layer_no], layer_outputs[layer_no], layer_derivatives[layer_no]);

                for (i = 0; i < layer_sizes[layer_no]; i++) {
                    float error = 0.0;
                    for (j = 0; j < layer_sizes[layer_no+1]; j++)
                        error += local_gradient[layer_no+1][j] * param->weight[layer_no][i][j];

                    local_gradient[layer_no][i] = error * layer_derivatives[layer_no][i];
                }

                break;
            case 2: // sigmoid
                d_sigmoid(layer_sizes[layer_no], layer_inputs[layer_no], layer_outputs[layer_no], layer_derivatives[layer_no]);

                for (i = 0; i < layer_sizes[layer_no]; i++) {
                    float error = 0.0;
                    for (j = 0; j < layer_sizes[layer_no+1]; j++)
                        error += local_gradient[layer_no+1][j] * param->weight[layer_no][i][j];

                    local_gradient[layer_no][i] = error * layer_derivatives[layer_no][i];
                }

                break;
            case 3: // tanh
                d_tanh(layer_sizes[layer_no], layer_inputs[layer_no], layer_outputs[layer_no], layer_derivatives[layer_no]);

                for (i = 0; i < layer_sizes[layer_no]; i++) {
                    float error = 0.0;
                    for (j = 0; j < layer_sizes[layer_no+1]; j++)
                        error += local_gradient[layer_no+1][j] * param->weight[layer_no][i][j];

                    local_gradient[layer_no][i] = error * layer_derivatives[layer_no][i];
                }

                break;
            case 4: // relu
                d_relu(layer_sizes[layer_no], layer_inputs[layer_no], layer_outputs[layer_no], layer_derivatives[layer_no]);

                for (i = 0; i < layer_sizes[layer_no]; i++) {
                    float error = 0.0;
                    for (j = 0; j < layer_sizes[layer_no+1]; j++)
                        error += local_gradient[layer_no+1][j] * param->weight[layer_no][i][j];

                    local_gradient[layer_no][i] = error * layer_derivatives[layer_no][i];
                }

                break;
            case 5: // softmax
                d_softmax(layer_sizes[layer_no], layer_inputs[layer_no], layer_outputs[layer_no], layer_derivatives[layer_no]);

                for (i = 0; i < layer_sizes[layer_no]; i++) {
                    float error = 0.0;
                    for (j = 0; j < layer_sizes[layer_no+1]; j++)
                        error += local_gradient[layer_no+1][j] * param->weight[layer_no][i][j];

                    local_gradient[layer_no][i] = error * layer_derivatives[layer_no][i];
                }

                break;
            default:
                printf("Invalid hidden activation function\n");
                exit(0);
                break;
        }
    }
}

void back_propagation(parameters* param, int training_example, int n_layers, int* layer_sizes, float** layer_inputs, float** layer_outputs, float** layer_derivatives,
    float* expected_output, float*** weight_correction, float** local_gradient, float* error_output) {
    /* ------------------ Expected output ----------------------------------------*/
    int i;
    int j;

    // Make the respective element in expected_output to 1 and rest all 0
    // Ex: If y = 3 and output_layer_size = 4 then expected_output = [0, 0, 1, 0]
    if (param->output_layer_size == 1)
        expected_output[0] = param->data_train[training_example][param->feature_size-1];
    else
        expected_output[(int)(param->data_train[training_example][param->feature_size-1] - 1)] = 1;

    /*----------- Calculate weight corrections for all layers' weights -------------------*/
    // Weight correction for the output layer
    calculate_local_gradient(param, n_layers-1, n_layers, layer_sizes, layer_inputs, layer_outputs, expected_output, local_gradient, layer_derivatives, error_output);
    for (i = 0; i < param->output_layer_size; i++)
        for (j = 0; j < layer_sizes[n_layers-2]+1; j++)
            weight_correction[n_layers-2][j][i] = (param->learning_rate) * local_gradient[n_layers-1][i] * layer_outputs[n_layers-2][j];

    // Weight correction for the hidden layers
    int k;
    for (i = n_layers-2; i >= 1; i--) {
        calculate_local_gradient(param, i, n_layers, layer_sizes, layer_inputs, layer_outputs, expected_output, local_gradient, layer_derivatives, error_output);

        for (j = 0; j < layer_sizes[i]; j++)
            for (k = 0; k < layer_sizes[i-1]+1; k++)
                weight_correction[i-1][k][j] = (param->learning_rate) * local_gradient[i][j] * layer_outputs[i-1][k];
    }

    /*----------------- Update the weights -------------------------------------*/
    for (i = 0; i < n_layers-1; i++) {
        for (j = 0; j < layer_sizes[i]+1; j++) {
            for (k = 0; k < layer_sizes[i+1]; k++) {
                param->weight[i][j][k] += weight_correction[i][j][k];
            }
        }
    }
}

void mlp_classifier(parameters* param, int* layer_sizes, float** final_output, float** layer_inputs, float** layer_outputs, int** confusion_matrix) {
    // Guard against null pointers before any dereference below
    if (!param || !layer_sizes || !final_output || !layer_inputs || !layer_outputs || !confusion_matrix) {
        fprintf(stderr, "mlp_classifier: null pointer argument passed, aborting\n");
        return;
    }

    int n_layers = param->n_hidden + 2;
    int i = 0;

    // Classify the test dataset on the test samples
    int test_example;
    for (test_example = 0; test_example < param->test_sample_size; test_example++) {
        printf("Classifying test example %d of %d\r", test_example+1, param->test_sample_size);
        // Fill the input layer's input and output (both are equal) from data_test matrix for the given test example
        layer_outputs[0][0] = 1; // Bias term of input layer
        for (i = 0; i < param->feature_size-1; i++)
            layer_outputs[0][i+1] = layer_inputs[0][i] = param->data_test[test_example][i];

        // Perform forward propagation for each hidden layer
        // Calculate input and output of each hidden layer
        for (i = 1; i < n_layers-1; i++) {
            // Compute layer_inputs[i]
            mat_mul(layer_outputs[i-1], param->weight[i-1], layer_inputs[i], layer_sizes[i-1]+1, layer_sizes[i]);

            // Compute layer_outputs[i]
            // Activation functions (identity - 1, sigmoid - 2, tanh - 3, relu - 4, softmax - 5)
            switch (param->hidden_activation_functions[i-1]) {
                case 1: // identity
                    identity(layer_sizes[i], layer_inputs[i], layer_outputs[i]);
                    break;
                case 2: // sigmoid
                    sigmoid(layer_sizes[i], layer_inputs[i], layer_outputs[i]);
                    break;
                case 3: // tanh
                    tan_h(layer_sizes[i], layer_inputs[i], layer_outputs[i]);
                    break;
                case 4: // relu
                    relu(layer_sizes[i], layer_inputs[i], layer_outputs[i]);
                    break;
                case 5: // softmax
                    softmax(layer_sizes[i], layer_inputs[i], layer_outputs[i]);
                    break;
                default:
                    printf("Forward propagation: Invalid hidden activation function\n");
                    exit(0);
                    break;
            }
        }

        // Fill the output layers's input and output
        mat_mul(layer_outputs[n_layers-2], param->weight[n_layers-2], layer_inputs[n_layers-1], layer_sizes[n_layers-2]+1, layer_sizes[n_layers-1]);

        // Activation functions (identity - 1, sigmoid - 2, tanh - 3, relu - 4, softmax - 5)
        switch (param->output_activation_function) {
            case 1: // identity
                identity(layer_sizes[n_layers-1], layer_inputs[n_layers-1], layer_outputs[n_layers-1]);
                break;
            case 2: // sigmoid
                sigmoid(layer_sizes[n_layers-1], layer_inputs[n_layers-1], layer_outputs[n_layers-1]);
                break;
            case 3: // tanh
                tan_h(layer_sizes[n_layers-1], layer_inputs[n_layers-1], layer_outputs[n_layers-1]);
                break;
            case 4: // relu
                relu(layer_sizes[n_layers-1], layer_inputs[n_layers-1], layer_outputs[n_layers-1]);
                break;
            case 5: // softmax
                softmax(layer_sizes[n_layers-1], layer_inputs[n_layers-1], layer_outputs[n_layers-1]);
                break;
            default:
                printf("Forward propagation: Invalid hidden activation function\n");
                exit(0);
                break;
        }

        // Save the computed output into a output matrix
        // Final computed output is present in layer_outputs[n_layers-1] from index 1
        for (i = 0; i < param->output_layer_size; i++)
            final_output[test_example][i] = layer_outputs[n_layers-1][i+1];
    }

    // Find the output class for each test example
    if (param->output_layer_size == 1) { // Binary classification
        for (test_example = 0; test_example < param->test_sample_size; test_example++) {
            if (final_output[test_example][0] < 0.5)
                final_output[test_example][0] = 0;
            else
                final_output[test_example][0] = 1;
        }
    }
    else { // Multi-class classification
        for (test_example = 0; test_example < param->test_sample_size; test_example++) {
            float max = -1;
            int max_class;
            for (i = 0; i < param->output_layer_size; i++) {
                if (final_output[test_example][i] > max) {
                    max = final_output[test_example][i];
                    max_class = i+1;
                }
            }
            final_output[test_example][0] = max_class;
        }
    }

    // Calculate the confusion matrix
    if (param->output_layer_size == 1) { // Binary classification
        int true_positive = 0, true_negative = 0, false_positive = 0, false_negative = 0;
        for (test_example = 0; test_example < param->test_sample_size; test_example++) {
            if (final_output[test_example][0] == 0) {
                if (param->data_test[test_example][param->feature_size-1] == 0)
                    ++true_negative;
                else
                    ++false_positive;
            }
            else {
                if (param->data_test[test_example][param->feature_size-1] == 1)
                    ++true_positive;
                else
                    ++false_negative;
            }
        }

        // Find the accuracy
        float accuracy = (float)(true_positive + true_negative) / param->test_sample_size;

        // Print confusion matrix
        printf("\n\nConfusion matrix\n");
        printf("-----------------\n\n");

        printf("\t    |predicted 0\t predicted 1\n");
        printf("--------------------------------------------\n");
        printf("Actual 0    |%d\t\t%d\n\n", true_negative, false_positive);
        printf("Actual 1    |%d\t\t%d\n\n", false_negative, true_positive);

        // Print the accuracy
        printf("\nAccuracy: %.2lf\n\n", accuracy * 100);
    }
    else { // Multi-class classification
        // Fill the confusion matrix
        int actual_class, predicted_class;
        for (test_example = 0; test_example < param->test_sample_size; test_example++) {
            actual_class = param->data_test[test_example][param->feature_size-1] - 1;
            predicted_class = final_output[test_example][0] - 1;

            ++confusion_matrix[actual_class][predicted_class];
        }

        // Print the confusion matrix
        printf("\t");
        for (predicted_class = 1; predicted_class <= param->output_layer_size; predicted_class++)
            printf("Predicted %d  ", predicted_class);
        printf("\n---------------------------------------------------------------------------\n");

        for (actual_class = 0; actual_class < param->output_layer_size; actual_class++) {
            printf("Actual %d | ", actual_class+1);
            for (predicted_class = 0; predicted_class < param->output_layer_size; predicted_class++)
                printf("%d\t", confusion_matrix[actual_class][predicted_class]);
            printf("\n");
        }

        // Find the accuracy
        float accuracy = 0.0;
        for (i = 0; i < param->output_layer_size; i++)
            accuracy += confusion_matrix[i][i];
        accuracy /= param->test_sample_size;

        // Print the accuracy
        printf("\nAccuracy: %.2lf\n\n", accuracy * 100);
    }
}

void initialize_weights(parameters* param, int n_layers, int* layer_sizes, float* epsilon) {
    srand(time(0));

    // epsilon = sqrt(6/(layer_size[i] + layer_size[i+1])) used for random initialization
    int i;
    for (i = 0; i < n_layers-1; i++)
        epsilon[i] = sqrt(6.0 / (layer_sizes[i] + layer_sizes[i+1]));

    // Random initialization between [-epsilon[i], epsilon[i]] for weight[i]
    int j, k;
    for (i = 0; i < n_layers-1; i++)
        for (j = 0; j < layer_sizes[i]+1; j++)
            for (k = 0; k < layer_sizes[i+1]; k++)
                param->weight[i][j][k] = -epsilon[i] + ((float)rand() / ((float)RAND_MAX / (2.0 * epsilon[i])));
}

void randomly_shuffle(int* a, int n) {
    int i, j;
    srand(time(NULL));
    for (i = n-1; i > 0; i--) {
        j = rand() % (i+1);
        int temp = a[i];
        a[i] = a[j];
        a[j] = temp;
    }
}

void forward_propagation(parameters* param, int training_example, int n_layers, int* layer_sizes, float** layer_inputs, float** layer_outputs) {
    // Fill the input layer's input and output (both are equal) from data matrix with the given training example
    int i;
    layer_outputs[0][0] = 1; // Bias term of input layer
    for (i = 0; i < param->feature_size-1; i++)
        layer_outputs[0][i+1] = layer_inputs[0][i] = param->data_train[training_example][i];

    // Perform forward propagation for each hidden layer
    // Calculate input and output of each hidden layer
    for (i = 1; i < n_layers-1; i++) {
        // Compute layer_inputs[i]
        mat_mul(layer_outputs[i-1], param->weight[i-1], layer_inputs[i], layer_sizes[i-1]+1, layer_sizes[i]);

        // Compute layer_outputs[i]
        // Activation functions (identity - 1, sigmoid - 2, tanh - 3, relu - 4, softmax - 5)
        switch (param->hidden_activation_functions[i-1]) {
            case 1: // identity
                identity(layer_sizes[i], layer_inputs[i], layer_outputs[i]);
                break;
            case 2: // sigmoid
                sigmoid(layer_sizes[i], layer_inputs[i], layer_outputs[i]);
                break;
            case 3: // tanh
                tan_h(layer_sizes[i], layer_inputs[i], layer_outputs[i]);
                break;
            case 4: // relu
                relu(layer_sizes[i], layer_inputs[i], layer_outputs[i]);
                break;
            case 5: // softmax
                softmax(layer_sizes[i], layer_inputs[i], layer_outputs[i]);
                break;
            default:
                printf("Forward propagation: Invalid hidden activation function\n");
                exit(0);
                break;
        }
    }

    // Fill the output layers's input and output
    mat_mul(layer_outputs[n_layers-2], param->weight[n_layers-2], layer_inputs[n_layers-1], layer_sizes[n_layers-2]+1, layer_sizes[n_layers-1]);

    // Activation functions (identity - 1, sigmoid - 2, tanh - 3, relu - 4, softmax - 5)
    switch (param->output_activation_function) {
        case 1: // identity
            identity(layer_sizes[n_layers-1], layer_inputs[n_layers-1], layer_outputs[n_layers-1]);
            break;
        case 2: // sigmoid
            sigmoid(layer_sizes[n_layers-1], layer_inputs[n_layers-1], layer_outputs[n_layers-1]);
            break;
        case 3: // tanh
            tan_h(layer_sizes[n_layers-1], layer_inputs[n_layers-1], layer_outputs[n_layers-1]);
            break;
        case 4: // relu
            relu(layer_sizes[n_layers-1], layer_inputs[n_layers-1], layer_outputs[n_layers-1]);
            break;
        case 5: // softmax
            softmax(layer_sizes[n_layers-1], layer_inputs[n_layers-1], layer_outputs[n_layers-1]);
            break;
        default:
            printf("Forward propagation: Invalid hidden activation function\n");
            exit(0);
            break;
    }
}

void mlp_trainer(parameters* param, int* layer_sizes, float** layer_inputs, float** layer_outputs, float* epsilon, int* indices,
    float** layer_derivatives, float** local_gradient, float*** weight_correction, float* expected_output, float* error_output) {
    // Guard against null pointers before any dereference below
    if (!param || !layer_sizes || !layer_inputs || !layer_outputs || !epsilon || !indices ||
        !layer_derivatives || !local_gradient || !weight_correction || !expected_output || !error_output) {
        fprintf(stderr, "mlp_trainer: null pointer argument passed, aborting\n");
        return;
    }

    // Total number of layers
    int n_layers = param->n_hidden + 2;
    int i = 0;

    // Initialize the weights
    initialize_weights(param, n_layers, layer_sizes, epsilon);

    for (i = 0; i < param->train_sample_size; i++)
        indices[i] = i;

    /* ------------------------------------------------------------------
     * Scratch buffers required by back_propagation() / calculate_local_gradient().
     * layer_derivatives[l] and local_gradient[l] hold layer_sizes[l] values
     * for l = 1..n_layers-1 (index 0, the input layer, is unused).
     * weight_correction mirrors param->weight's shape exactly.
     * ------------------------------------------------------------------ */
    // Train the MLP
    int training_example, j;
    for (i = 0; i < param->n_iterations_max; i++) {
        printf("Iteration %d of %d(max)\r", i+1, param->n_iterations_max);
        // Randomly shuffle the data
        randomly_shuffle(indices, param->train_sample_size);

        for (j = 0; j < param->train_sample_size; j++) {
            training_example = indices[j];

            // Reset the one-hot target buffer (back_propagation only sets
            // the active class index for multi-class problems).
            memset(expected_output, 0, param->output_layer_size * sizeof(float));

            // Perform forward propagation on the jth training example
            forward_propagation(param, training_example, n_layers, layer_sizes, layer_inputs, layer_outputs);

            // Perform back propagation and update weights
            back_propagation(param, training_example, n_layers, layer_sizes, layer_inputs, layer_outputs,
                layer_derivatives, expected_output, weight_correction, local_gradient, error_output);
        }
    }
}