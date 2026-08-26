include_guard(GLOBAL)

option(TBCI_WITH_LSL "Build with LSL support" OFF)


if(TBCI_WITH_LSL)
    include(FetchContent)
    FetchContent_Declare(liblsl
        GIT_REPOSITORY https://github.com/sccn/liblsl.git
        GIT_TAG        v1.16.2
    )
    FetchContent_MakeAvailable(liblsl)

    if (${TBCI_BUILD_EXAMPLES})
        target_sources(tinybci_runner PRIVATE
            producer/lsl_producer.c
            src/ioutils/tbci_lsl_reader.c
            src/ioutils/tbci_lsl_writer.c
        )
        target_link_libraries(tinybci_runner PRIVATE lsl stdc++)
        target_include_directories(tinybci_runner PRIVATE
            ${liblsl_SOURCE_DIR}/include
        )
        target_compile_definitions(tinybci_runner PRIVATE TBCI_WITH_LSL)


        add_executable(tinybci_lsl_runner
            examples/run_lsl.c
            producer/lsl_producer.c
            producer/synthetic_producer.c
            producer/unicorn_producer.c
            producer/neuropawn_producer.c
            producer/tbci_producer_factory.c
            producer/tbci_trigger_generator.c
            src/ioutils/tbci_lsl_reader.c
            src/ioutils/tbci_lsl_writer.c
            producer/neuropawn_producer.c
        )

        target_link_libraries(tinybci_lsl_runner PRIVATE tiny_bci lsl stdc++)
        target_include_directories(tinybci_lsl_runner PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/producer
            ${liblsl_SOURCE_DIR}/include
        )
        target_compile_definitions(tinybci_lsl_runner PRIVATE TBCI_WITH_LSL)
    endif()
endif()