/**
 * @file tbci_mlp_example.c
 *
 * Demonstrates how to wire up tbci_mlp_trainer / tbci_mlp_classifier on
 * the MNIST handwritten-digit dataset:
 *
 *   1. Build a `parameters` struct describing the network topology.
 *   2. Allocate the ragged arrays the library expects (weight[], layer
 *      inputs/outputs, backprop scratch buffers) — these are NOT
 *      fixed-size, so this uses malloc/free throughout.
 *   3. Load mnist_train.csv / mnist_test.csv, train a 1-hidden-layer MLP,
 *      and classify the held-out set with a 10x10 confusion matrix.
 *
 * Expected CSV format (header row skipped, one row per image):
 *   label,pix-1-1,pix-1-2,...,pix-28-28
 * where label is the digit 0-9 and pix-i-j is the pixel intensity
 * (0-255) at row i, column j of the 28x28 image (784 pixel columns).
 *
 * Usage:
 *   ./mlp_example [train.csv] [test.csv] [n_epochs]
 * All three arguments are optional (default: mnist_train.csv,
 * mnist_test.csv, 10 epochs).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/nodes/decoder/nn/tbci_mlp_classifier.h"

/* Set by CMake (target_compile_definitions) to the absolute path of the
 * examples/ directory, so the default CSV paths below work regardless of
 * the working directory the exe is launched from (e.g. bin/ vs project
 * root). Falls back to a relative path if compiled outside that build. */
#ifndef TBCI_EXAMPLES_DIR
#define TBCI_EXAMPLES_DIR "examples"
#endif

/* --------------------------------------------------------------------------
 * Problem size.
 * -------------------------------------------------------------------------- */

#define N_PIXELS        784    /* 28 x 28 MNIST image                      */
#define N_CLASSES       10     /* digits 0-9                               */
#define N_HIDDEN_LAYERS 1
#define HIDDEN1_SIZE    128

#define OUTPUT_SIZE     N_CLASSES

/* Activation codes used throughout this library:
 * 1 = identity, 2 = sigmoid, 3 = tanh, 4 = relu, 5 = softmax */
#define ACT_SIGMOID     2
#define ACT_SOFTMAX     5

/* Longest line we expect in the CSV: 784 pixel values (up to 3 digits
 * each) + label + commas + newline is well under this. */
#define CSV_LINE_BUF    8192

/* --------------------------------------------------------------------------
 * MNIST CSV loading
 * -------------------------------------------------------------------------- */

static float** alloc_matrix( int rows, int cols )
{
    float** m = (float**)malloc( rows * sizeof( float* ) );
    for ( int r = 0; r < rows; r++ )
        m[r] = (float*)malloc( cols * sizeof( float ) );
    return m;
}

static void free_matrix( float** m, int rows )
{
    for ( int r = 0; r < rows; r++ )
        free( m[r] );
    free( m );
}

/* Counts data rows in a CSV file, skipping the header line. */
static int count_data_rows( const char* path )
{
    FILE* fp = fopen( path, "r" );
    if ( !fp ) {
        fprintf( stderr, "Error: cannot open %s\n", path );
        exit( 1 );
    }

    char line[CSV_LINE_BUF];
    int count = 0;

    if ( !fgets( line, sizeof( line ), fp ) ) { /* skip header */
        fclose( fp );
        return 0;
    }
    while ( fgets( line, sizeof( line ), fp ) ) {
        if ( line[0] == '\n' || line[0] == '\0' )
            continue; /* skip blank trailing line */
        count++;
    }

    fclose( fp );
    return count;
}

/* Loads an MNIST CSV into an (n_rows) x (n_pixels+1) matrix: columns
 * 0..n_pixels-1 are pixel intensities scaled to [0, 1], and the last
 * column is the class label.
 *
 * The library's back_propagation/mlp_classifier code indexes multi-class
 * targets as 1..output_layer_size (see expected_output[label-1] = 1),
 * so MNIST digit d is stored here as d+1, not d. */
static float** load_mnist_csv( const char* path, int n_rows, int n_pixels )
{
    FILE* fp = fopen( path, "r" );
    if ( !fp ) {
        fprintf( stderr, "Error: cannot open %s\n", path );
        exit( 1 );
    }

    char line[CSV_LINE_BUF];
    float** data = alloc_matrix( n_rows, n_pixels + 1 );

    if ( !fgets( line, sizeof( line ), fp ) ) { /* skip header */
        fclose( fp );
        return data;
    }

    int row = 0;
    while ( row < n_rows && fgets( line, sizeof( line ), fp ) ) {
        if ( line[0] == '\n' || line[0] == '\0' )
            continue;

        char* tok = strtok( line, "," );
        int label = atoi( tok );

        for ( int p = 0; p < n_pixels; p++ ) {
            tok = strtok( NULL, "," );
            data[row][p] = tok ? ( atof( tok ) / 255.0 ) : 0.0;
        }

        data[row][n_pixels] = (float)( label + 1 );
        row++;
    }

    fclose( fp );
    return data;
}

/* --------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------- */

int main( int argc, char** argv )
{
    const char* train_path = ( argc > 1 ) ? argv[1] : TBCI_EXAMPLES_DIR "/mnist_train.csv";
    const char* test_path  = ( argc > 2 ) ? argv[2] : TBCI_EXAMPLES_DIR "/mnist_test.csv";
    int n_epochs            = ( argc > 3 ) ? atoi( argv[3] ) : 10;

    /* ------------------------------------------------------------------
     * 1. Load train/test sets. Sample counts come from the files
     *    themselves rather than being hardcoded.
     * ------------------------------------------------------------------ */
    printf( "Counting rows in %s...\n", train_path );
    int n_train = count_data_rows( train_path );
    printf( "Counting rows in %s...\n", test_path );
    int n_test  = count_data_rows( test_path );
    printf( "Train samples: %d, Test samples: %d\n", n_train, n_test );

    printf( "Loading training data...\n" );
    float** data_train = load_mnist_csv( train_path, n_train, N_PIXELS );
    printf( "Loading test data...\n" );
    float** data_test  = load_mnist_csv( test_path, n_test, N_PIXELS );

    /* ------------------------------------------------------------------
     * 2. Network topology.
     *    n_layers = n_hidden + 2 (input layer + hidden layers + output).
     *    layer_sizes[0]           = number of input features (pixels)
     *    layer_sizes[1..n_hidden] = hidden_layers_size[i-1]
     *    layer_sizes[n_layers-1]  = output_layer_size (10 digit classes)
     * ------------------------------------------------------------------ */
    int hidden_layers_size[N_HIDDEN_LAYERS]          = { HIDDEN1_SIZE };
    int hidden_activation_functions[N_HIDDEN_LAYERS] = { ACT_SIGMOID };

    int n_layers = N_HIDDEN_LAYERS + 2;
    int layer_sizes[/* n_layers */ 3]; /* = {input, hidden1, output} for N_HIDDEN_LAYERS=1 */
    layer_sizes[0]            = N_PIXELS;
    layer_sizes[1]            = HIDDEN1_SIZE;
    layer_sizes[n_layers - 1] = OUTPUT_SIZE;

    /* ------------------------------------------------------------------
     * 3. Fill the `parameters` struct.
     * ------------------------------------------------------------------ */
    parameters param;
    param.n_hidden                     = N_HIDDEN_LAYERS;
    param.hidden_layers_size           = hidden_layers_size;
    param.hidden_activation_functions  = hidden_activation_functions;
    param.learning_rate                = 0.05;
    param.n_iterations_max             = n_epochs;
    param.momentum                     = 0; /* unused by this trainer impl */
    param.output_layer_size            = OUTPUT_SIZE;
    param.output_activation_function   = ACT_SOFTMAX;
    param.data_train                   = data_train;
    param.data_test                    = data_test;
    param.feature_size                 = N_PIXELS + 1; /* + label column */
    param.train_sample_size            = n_train;
    param.test_sample_size             = n_test;

    /* ------------------------------------------------------------------
     * 4. Allocate weight[layer][from_unit][to_unit].
     *    weight[i] has shape (layer_sizes[i]+1) x layer_sizes[i+1];
     *    the "+1" row is the bias weight row.
     * ------------------------------------------------------------------ */
    param.weight = (float***)malloc( ( n_layers - 1 ) * sizeof( float** ) );
    for ( int i = 0; i < n_layers - 1; i++ ) {
        param.weight[i] = (float**)malloc( ( layer_sizes[i] + 1 ) * sizeof( float* ) );
        for ( int j = 0; j < layer_sizes[i] + 1; j++ )
            param.weight[i][j] = (float*)malloc( layer_sizes[i + 1] * sizeof( float ) );
    }

    /* ------------------------------------------------------------------
     * 5. Allocate per-layer scratch used by forward/back propagation.
     *    layer_inputs[i]  : layer_sizes[i] units  (pre-activation, no bias)
     *    layer_outputs[i] : layer_sizes[i]+1 units (post-activation, w/ bias)
     * ------------------------------------------------------------------ */
    float** layer_inputs  = (float**)malloc( n_layers * sizeof( float* ) );
    float** layer_outputs = (float**)malloc( n_layers * sizeof( float* ) );
    for ( int i = 0; i < n_layers; i++ ) {
        layer_inputs[i]  = (float*)malloc( layer_sizes[i] * sizeof( float ) );
        layer_outputs[i] = (float*)malloc( ( layer_sizes[i] + 1 ) * sizeof( float ) );
    }

    /* epsilon[i]: init range for weight[i], one per weight matrix */
    float* epsilon = (float*)malloc( ( n_layers - 1 ) * sizeof( float ) );

    /* indices[]: shuffled each epoch to randomize training example order */
    int* indices = (int*)malloc( n_train * sizeof( int ) );

    /* ------------------------------------------------------------------
     * 5b. Back-propagation scratch buffers.
     *     mlp_trainer no longer allocates these itself, so the caller
     *     owns them now:
     *       layer_derivatives[l] / local_gradient[l] : layer_sizes[l]
     *         values each, for l = 1..n_layers-1 (index 0, the input
     *         layer, is unused but allocated anyway for simplicity).
     *       weight_correction : mirrors param.weight's shape exactly
     *         (same ragged (layer_sizes[i]+1) x layer_sizes[i+1] layout).
     *       expected_output / error_output : output_layer_size values.
     * ------------------------------------------------------------------ */
    float** layer_derivatives = (float**)malloc( n_layers * sizeof( float* ) );
    float** local_gradient    = (float**)malloc( n_layers * sizeof( float* ) );
    for ( int i = 0; i < n_layers; i++ ) {
        layer_derivatives[i] = (float*)malloc( layer_sizes[i] * sizeof( float ) );
        local_gradient[i]    = (float*)malloc( layer_sizes[i] * sizeof( float ) );
    }

    float*** weight_correction = (float***)malloc( ( n_layers - 1 ) * sizeof( float** ) );
    for ( int i = 0; i < n_layers - 1; i++ ) {
        weight_correction[i] = (float**)malloc( ( layer_sizes[i] + 1 ) * sizeof( float* ) );
        for ( int j = 0; j < layer_sizes[i] + 1; j++ )
            weight_correction[i][j] = (float*)malloc( layer_sizes[i + 1] * sizeof( float ) );
    }

    float* expected_output = (float*)malloc( OUTPUT_SIZE * sizeof( float ) );
    float* error_output    = (float*)malloc( OUTPUT_SIZE * sizeof( float ) );

    /* ------------------------------------------------------------------
     * 6. Train.
     * ------------------------------------------------------------------ */
    printf( "Training for %d epoch(s)...\n", n_epochs );
    mlp_trainer( &param, layer_sizes, layer_inputs, layer_outputs, epsilon, indices,
        layer_derivatives, local_gradient, weight_correction, expected_output, error_output );
    puts( "\nTraining complete." );

    /* ------------------------------------------------------------------
     * 7. Classify the held-out set.
     *    Note: the library's confusion-matrix printout labels rows/columns
     *    "Actual 1".."Actual 10" / "Predicted 1".."Predicted 10", which are
     *    1-indexed internally — "1" corresponds to digit 0, "10" to digit 9.
     * ------------------------------------------------------------------ */
    float** final_output = alloc_matrix( n_test, OUTPUT_SIZE );

    int** confusion_matrix = (int**)malloc( OUTPUT_SIZE * sizeof( int* ) );
    for ( int i = 0; i < OUTPUT_SIZE; i++ )
        confusion_matrix[i] = (int*)calloc( OUTPUT_SIZE, sizeof( int ) );

    mlp_classifier( &param, layer_sizes, final_output, layer_inputs, layer_outputs, confusion_matrix );

    /* ------------------------------------------------------------------
     * 8. Cleanup.
     * ------------------------------------------------------------------ */
    for ( int i = 0; i < OUTPUT_SIZE; i++ )
        free( confusion_matrix[i] );
    free( confusion_matrix );
    free_matrix( final_output, n_test );

    free( indices );
    free( epsilon );
    for ( int i = 0; i < n_layers; i++ ) {
        free( layer_inputs[i] );
        free( layer_outputs[i] );
        free( layer_derivatives[i] );
        free( local_gradient[i] );
    }
    free( layer_inputs );
    free( layer_outputs );
    free( layer_derivatives );
    free( local_gradient );

    free( expected_output );
    free( error_output );

    for ( int i = 0; i < n_layers - 1; i++ ) {
        for ( int j = 0; j < layer_sizes[i] + 1; j++ ) {
            free( param.weight[i][j] );
            free( weight_correction[i][j] );
        }
        free( param.weight[i] );
        free( weight_correction[i] );
    }
    free( param.weight );
    free( weight_correction );

    free_matrix( data_train, n_train );
    free_matrix( data_test, n_test );

    return 0;
}