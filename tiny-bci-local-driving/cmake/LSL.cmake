include_guard(GLOBAL)

set(LSL_SOURCES
    src/lsl/inlet_helpers.c
    src/lsl/outlet_helpers.c
    src/lsl/data_source.c

    src/lsl/eeg_source.c

    src/lsl/trigger_outlet.c
    src/lsl/trigger_source.c
    src/lsl/inference_outlet.c
    src/lsl/inference_source.c
)

option(USE_LSL_TIMESTAMPS "Use EEG  timestamps from source, marking triggers with lsl_local_clock" OFF)
if (${USE_LSL_TIMESTAMPS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_LSL_TIMESTAMPS)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(liblsl
    GIT_REPOSITORY https://github.com/sccn/liblsl.git
    GIT_TAG        v1.16.2
)
FetchContent_MakeAvailable(liblsl)

set_target_properties(lslver PROPERTIES EXCLUDE_FROM_ALL ON)


include(cmake/RuntimeOutputPath.cmake)
set(CONFIG_FILE_PATH_EXPRESSION "${FULL_RUNTIME_OUTPUT_PATH_EXPRESSION}/lsl_api.cfg")

add_custom_target(copy_lsl_configuration_file ALL
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/lsl_api.cfg"
    "${CONFIG_FILE_PATH_EXPRESSION}"
  COMMENT "Copying lsl configuration file to output folder: ${CONFIG_FILE_PATH_EXPRESSION}"
)