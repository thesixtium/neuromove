include_guard(GLOBAL)

option(TBCI_MLP_TEST "Build an example to test MLP implementation" OFF)


if(TBCI_MLP_TEST)

    add_executable(tinybci_mlp_example
        examples/tbci_mlp_example.c
        src/nodes/decoder/nn/tbci_mlp_classifier.c
    )

    target_include_directories(tinybci_mlp_example PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_compile_definitions(tinybci_mlp_example PRIVATE
        TBCI_EXAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/examples"
    )

    if(MATH_LIBRARY)
        target_link_libraries(tinybci_mlp_example PRIVATE ${MATH_LIBRARY})
    endif()
endif()