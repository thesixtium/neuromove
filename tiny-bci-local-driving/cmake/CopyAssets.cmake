include_guard(GLOBAL)

include(cmake/RuntimeOutputPath.cmake)
set(RUNTIME_ASSET_OUTPUT_PATH_EXPRESSION "${FULL_RUNTIME_OUTPUT_PATH_EXPRESSION}/assets")

add_custom_target(copy_assets
  COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
    "${CMAKE_SOURCE_DIR}/assets/"
    "${RUNTIME_ASSET_OUTPUT_PATH_EXPRESSION}"
  COMMENT "Copying assets to output folder: ${RUNTIME_ASSET_OUTPUT_PATH_EXPRESSION}"
)