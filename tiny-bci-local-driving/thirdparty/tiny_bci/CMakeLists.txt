cmake_minimum_required(VERSION 3.12)

project(tiny_bci
    VERSION 0.5.0
    DESCRIPTION "A lightweight BCI signal processing library"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)  # Use -std=c99 not -std=gnu99; important for portability

if(UNIX AND NOT APPLE)
    add_compile_definitions(_XOPEN_SOURCE=500)
endif()

# Put compiled binaries in bin/ for convenience
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Optional override — set before add_library
set(TBCI_MAX_CHANNELS 64 CACHE STRING "Maximum EEG channels supported" FORCE)
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

set(TBCI_LIBRARY_TYPE STATIC)
if(BUILD_SHARED_LIBS)
    set(TBCI_LIBRARY_TYPE SHARED)
endif()

# --------------------------------------------------------------------------
# Library target
# This is the core TinyBCI library. All pipeline components go here.
# --------------------------------------------------------------------------

add_library(${PROJECT_NAME} ${TBCI_LIBRARY_TYPE}
    src/tbci_config.c
    src/tbci_context.c
    src/containers/tbci_trigger_queue.c
    src/containers/tbci_signal_buffer.c
    src/containers/tbci_epoch_queue.c
    src/ioutils/tbci_input.c
    src/mathutils/tbci_math.c
    src/nodes/tbci_node.c
    src/nodes/tbci_node_group.c
    src/nodes/preprocessing/tbci_preprocessing.c
    src/nodes/preprocessing/tbci_iir_filter_state.c
    src/nodes/preprocessing/tbci_bandpass_node.c
    src/nodes/preprocessing/tbci_notch_node.c
    src/nodes/core/tbci_seg_node.c
    src/nodes/core/tbci_sync_node.c
    src/nodes/core/tbci_core_node.c
    src/nodes/core/tbci_raw_out.c
    src/nodes/features/tbci_features_extraction.c
    src/nodes/features/tbci_cca_node.c
    src/nodes/decoder/tbci_cca_model.c
    src/nodes/decoder/tbci_decoder.c
    src/nodes/decoder/tbci_label_encoder_node.c
    src/nodes/decoder/tbci_trial_averaging_node.c
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    TBCI_MAX_CHANNELS=${TBCI_MAX_CHANNELS}
)
if(TBCI_BUILD_SHARED)
    target_compile_definitions(${PROJECT_NAME} PUBLIC -D TBCI_BUILD_SHARED)
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_precompile_headers(${PROJECT_NAME} PRIVATE include/pch.h)

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_WARNING_AS_ERROR OFF)

# --------------------------------------------------------------------------
# Tests
# --------------------------------------------------------------------------

option(TBCI_BUILD_TESTS "Build Tiny BCI examples." ${PROJECT_IS_TOP_LEVEL})
if (${TBCI_BUILD_TESTS})

enable_testing()

# Unity test framework — compiled once, linked into each test binary
add_library(unity STATIC
    tests/unity/unity.c
)

target_include_directories(unity
    PUBLIC
    tests/unity
)

# Helper macro: add_tbci_test(name source)
# Creates a test executable, links it against ${PROJECT_NAME}  and unity,
# and registers it with CTest.
macro(add_tbci_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE ${PROJECT_NAME}  unity)
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    add_test(NAME ${name} COMMAND ${name})
endmacro()

add_tbci_test(test_signal_buffer tests/test_signal_buffer.c)
add_tbci_test(test_trigger_queue tests/test_trigger_queue.c)
add_tbci_test(test_epoch_queue tests/test_epoch_queue.c)
add_tbci_test(test_context tests/test_context.c)
add_tbci_test(test_segmentation tests/test_segmentation.c)
add_tbci_test(test_pipeline tests/test_pipeline.c)
add_tbci_test(test_input tests/test_input.c)
add_tbci_test(test_node tests/test_node.c)
add_tbci_test(test_node_group tests/test_node_group.c)
add_tbci_test(test_preprocessing tests/test_preprocessing.c)
add_tbci_test(test_features_extraction tests/test_features_extraction.c)
add_tbci_test(test_cca_node tests/test_cca_node.c)
add_tbci_test(test_classifier_node tests/test_decoder.c)
add_tbci_test(test_math tests/test_math.c)
add_tbci_test(test_iir tests/test_iir_filter.c)
add_tbci_test(test_label_encoder tests/test_label_encoder.c)
add_tbci_test(test_trial_averaging tests/test_trial_averaging_node.c)
add_tbci_test(test_raw_out tests/test_raw_out.c)
#add_tbci_test(test_activations tests/test_activations.c)
endif()

# future: add_tbci_test(test_preprocessing tests/test_preprocessing.c)
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${MATH_LIBRARY})
endif()

# Link against librt for clock_gettime on older glibc (e.g. Raspberry Pi)
find_library(RT_LIBRARY rt)
if(RT_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${RT_LIBRARY})
endif()

# --------------------------------------------------------------------------
# Runner
# --------------------------------------------------------------------------
option(TBCI_BUILD_EXAMPLES "Build Tiny BCI examples." ${PROJECT_IS_TOP_LEVEL})
if (${TBCI_BUILD_EXAMPLES})

    set(RUNNER_SOURCES
        examples/run_synthetic.c
        producer/synthetic_producer.c
        producer/neuropawn_producer.c
        producer/unicorn_producer.c
        producer/tbci_producer_factory.c
        producer/tbci_trigger_generator.c
    )

    add_executable(tinybci_runner ${RUNNER_SOURCES})

    target_link_libraries(tinybci_runner PRIVATE tiny_bci)
    target_include_directories(tinybci_runner PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/producer
    )

    # --------------------------------------------------------------------------
    # NeuroPawn Knight Runner (run_neuropawn) — direct hardware, no LSL
    # --------------------------------------------------------------------------
    add_executable(tinybci_neuropawn_runner
        examples/run_neuropawn.c
        producer/neuropawn_producer.c
        producer/tbci_trigger_generator.c
    )

    target_link_libraries(tinybci_neuropawn_runner PRIVATE tiny_bci)
    target_include_directories(tinybci_neuropawn_runner PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/producer
    )

    # --------------------------------------------------------------------------
    # MLP Example (tinybci_mlp_example) — standalone, no tiny_bci/LSL dependency
    # --------------------------------------------------------------------------
    include(cmake/MLPTest.cmake)
endif()

# --------------------------------------------------------------------------
# LSL - Optional
# --------------------------------------------------------------------------
include(cmake/LSL.cmake)

# --------------------------------------------------------------------------
# ONNX Runtime - Optional
# --------------------------------------------------------------------------
include(cmake/OnnxRuntime.cmake)