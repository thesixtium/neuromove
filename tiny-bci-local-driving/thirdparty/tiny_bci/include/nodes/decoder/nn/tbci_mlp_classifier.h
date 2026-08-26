#ifndef MLP_CLASSIFIER_H
#define MLP_CLASSIFIER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

typedef struct {
    int n_hidden;
    int* hidden_layers_size;
    int* hidden_activation_functions;
    float learning_rate;
    int n_iterations_max;
    int momentum;
    int output_layer_size;
    int output_activation_function;
    float** data_train;
    float** data_test;
    int feature_size;
    int train_sample_size;
    int test_sample_size;
    float*** weight;
} parameters;

void mlp_trainer(parameters*, int*, float**, float**, float*, int*,
    float**, float**, float***, float*, float*);
void mlp_classifier(parameters*, int*, float**, float**, float**, int**);

#endif