cmake_minimum_required(VERSION 3.14)
set(PROJECT_NAME "projectm_texture")
project(${PROJECT_NAME} LANGUAGES CXX)

set(PLUGIN_NAME projectm_texture_plugin)

add_library(${PLUGIN_NAME} SHARED
  "projectm_texture_plugin.cpp"
  "gl_d3d_interop.cpp"
)

apply_standard_settings(${PLUGIN_NAME})

set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
)

target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

target_include_directories(${PLUGIN_NAME} INTERFACE
  "$<C_COMPILER_ID:MSVC>:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<CXX_COMPILER_ID:MSVC>:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_include_directories(${PLUGIN_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(${PLUGIN_NAME} PRIVATE flutter flutter_wrapper_plugin)

# Link Windows graphics APIs required for WGL/D3D11 Interop
target_link_libraries(${PLUGIN_NAME} PRIVATE opengl32 d3d11 dxgi)

# List of absolute paths to libraries that should be bundled with the plugin
set(projectm_texture_bundled_libraries
  ""
  PARENT_SCOPE
)
