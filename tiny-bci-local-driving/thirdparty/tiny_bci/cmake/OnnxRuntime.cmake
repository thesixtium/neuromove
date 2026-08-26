include_guard(GLOBAL)

option(TBCI_WITH_ONNX "Build with ONNX Runtime support" OFF)

if(TBCI_WITH_ONNX)
    if(DEFINED ONNXRUNTIME_ROOT)
        # ---- Local prebuilt path ----
        message(STATUS "Using local ONNX Runtime from ONNXRUNTIME_ROOT: ${ONNXRUNTIME_ROOT}")

        find_path(ORT_INCLUDE_DIR onnxruntime_c_api.h
                HINTS ${ONNXRUNTIME_ROOT}/include
                NO_DEFAULT_PATH
                REQUIRED)
        find_library(ORT_LIB onnxruntime
                HINTS ${ONNXRUNTIME_ROOT}/lib
                NO_DEFAULT_PATH
                REQUIRED)
        if(WIN32)
            find_file(ORT_DLL onnxruntime.dll
                    HINTS ${ONNXRUNTIME_ROOT}/lib ${ONNXRUNTIME_ROOT}/bin
                    NO_DEFAULT_PATH
                    REQUIRED)
        endif()

        message(STATUS "ONNX Runtime found at ONNXRUNTIME_ROOT: ${ORT_LIB}")
    else()
        # ---- Download via FetchContent ----
        if(WIN32)
            set(ORT_PLATFORM "win-x64")
            set(ORT_EXT "zip")
        elseif(APPLE)
            set(ORT_PLATFORM "osx-universal2")
            set(ORT_EXT "tgz")
        else()
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
                set(ORT_PLATFORM "linux-aarch64")
            else()
                set(ORT_PLATFORM "linux-x64")
            endif()
            set(ORT_EXT "tgz")
        endif()

        set(ORT_VERSION "1.20.1")
        set(ORT_URL
                "https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION}/onnxruntime-${ORT_PLATFORM}-${ORT_VERSION}.${ORT_EXT}"
        )

        include(FetchContent)
        message(STATUS "Downloading ONNX Runtime ${ORT_VERSION} for ${ORT_PLATFORM} from ${ORT_URL}")

        FetchContent_Declare(onnxruntime
                URL ${ORT_URL}
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(onnxruntime)

        set(ORT_INCLUDE_DIR ${onnxruntime_SOURCE_DIR}/include)
        find_library(ORT_LIB onnxruntime
                HINTS ${onnxruntime_SOURCE_DIR}/lib
                NO_DEFAULT_PATH
                REQUIRED)
        if(WIN32)
            find_file(ORT_DLL onnxruntime.dll
                    HINTS ${onnxruntime_SOURCE_DIR}/lib ${onnxruntime_SOURCE_DIR}/bin
                    NO_DEFAULT_PATH
                    REQUIRED)
        endif()

        message(STATUS "ONNX Runtime downloaded: ${ORT_LIB}")
    endif()

    if(WIN32)
        message(STATUS "ONNX Runtime DLL: ${ORT_DLL}")
    endif()

    target_sources(${PROJECT_NAME} PRIVATE
            src/nodes/decoder/tbci_onnx_model.c
    )
    target_include_directories(${PROJECT_NAME} PRIVATE ${ORT_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${ORT_LIB})
    target_compile_definitions(${PROJECT_NAME} PUBLIC TBCI_WITH_ONNX)

    # MinGW/GCC fix: ONNX Runtime Windows prebuilt headers use _stdcall in
    # function pointer declarations. Strict C99 (-std=c99, not gnu99) does not
    # auto-define _stdcall. Map it to GCC's __stdcall (a no-op on x86_64).
    if(MINGW)
        target_compile_definitions(${PROJECT_NAME} PUBLIC "_stdcall=__stdcall")
    endif()

    if (${TBCI_BUILD_TESTS})
        add_tbci_test(test_onnx_model tests/test_onnx_model.c)
        target_compile_definitions(test_onnx_model PRIVATE TBCI_WITH_ONNX)
        target_include_directories(test_onnx_model PRIVATE ${ORT_INCLUDE_DIR})
        target_link_libraries(test_onnx_model PRIVATE ${ORT_LIB})
        configure_file(tests/models/model.onnx
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/model.onnx COPYONLY)
        configure_file(tests/models/model.onnx
                ${CMAKE_BINARY_DIR}/model.onnx COPYONLY)
        target_compile_definitions(test_onnx_model PRIVATE
                TBCI_TEST_MODEL_PATH="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/model.onnx"
        )
        if(WIN32)
                add_custom_command(TARGET test_onnx_model POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${ORT_DLL}"
                        "$<TARGET_FILE_DIR:test_onnx_model>"
                        COMMENT "Copying onnxruntime.dll next to test_onnx_model"
                )
        endif()
    endif()

    if (${TBCI_BUILD_EXAMPLES})
        add_executable(tinybci_onnx_runner
                examples/run_onnx.c
                producer/synthetic_producer.c
                producer/unicorn_producer.c
                producer/tbci_producer_factory.c
                producer/tbci_trigger_generator.c
        )
        target_link_libraries(tinybci_onnx_runner PRIVATE tiny_bci ${ORT_LIB})
        target_include_directories(tinybci_onnx_runner PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/include
                ${CMAKE_CURRENT_SOURCE_DIR}/producer
                ${ORT_INCLUDE_DIR}
        )
        target_compile_definitions(tinybci_onnx_runner PRIVATE TBCI_WITH_ONNX)
        if(WIN32)
                add_custom_command(TARGET tinybci_onnx_runner POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${ORT_DLL}"
                        "$<TARGET_FILE_DIR:tinybci_onnx_runner>"
                        COMMENT "Copying onnxruntime.dll next to tinybci_onnx_runner"
                )
        endif()

        # --------------------------------------------------------------------------
        # Motor Imagery NeuroPawn Runner — EEGNet ONNX + NeuroPawn Knight headset
        # --------------------------------------------------------------------------
        add_executable(tinybci_mi_neuropawn_runner
                examples/motor_imagery_neuropawn.c
                producer/neuropawn_producer.c
                producer/tbci_trigger_generator.c
        )
        target_link_libraries(tinybci_mi_neuropawn_runner PRIVATE tiny_bci ${ORT_LIB})
        target_include_directories(tinybci_mi_neuropawn_runner PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/include
                ${CMAKE_CURRENT_SOURCE_DIR}/producer
                ${ORT_INCLUDE_DIR}
        )
        target_compile_definitions(tinybci_mi_neuropawn_runner PRIVATE TBCI_WITH_ONNX)
        if(WIN32)
                add_custom_command(TARGET tinybci_mi_neuropawn_runner POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${ORT_DLL}"
                        "$<TARGET_FILE_DIR:tinybci_mi_neuropawn_runner>"
                        COMMENT "Copying onnxruntime.dll next to tinybci_mi_neuropawn_runner"
                )
        endif()
    endif()

endif()