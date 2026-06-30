# The Flutter tooling requires that developers have CMake 3.10 or later
cmake_minimum_required(VERSION 3.10)

project(projectm_ffi_library VERSION 0.0.1 LANGUAGES C CXX)

# Fetch projectM v4.1.6
include(FetchContent)

set(ENABLE_SDL_UI OFF CACHE BOOL "Disable SDL UI" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable tests" FORCE)
if(WIN32)
  set(ENABLE_GLES OFF CACHE BOOL "Use Desktop GL on Windows" FORCE)
else()
  set(ENABLE_GLES ON CACHE BOOL "Use GLES for Flutter EGL contexts" FORCE)
endif()

# Workaround for projectM bug: it uses CMAKE_PROJECT_VERSION instead of PROJECT_VERSION
set(CMAKE_PROJECT_VERSION_MAJOR "4")
set(CMAKE_PROJECT_VERSION_MINOR "1")
set(CMAKE_PROJECT_VERSION_PATCH "6")
set(CMAKE_PROJECT_VERSION "4.1.6")

# Add cmake module path so it finds our dummy FindGLEW.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(WIN32)
  FetchContent_Declare(
    glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
    GIT_TAG        glew-2.2.0
  )
  FetchContent_MakeAvailable(glew)
  include_directories("${glew_SOURCE_DIR}/include")
endif()

FetchContent_Declare(
  projectM
  GIT_REPOSITORY https://github.com/projectM-visualizer/projectm.git
  GIT_TAG        v4.1.6
)
FetchContent_MakeAvailable(projectM)

add_library(projectm_ffi SHARED
  "projectm_ffi.cpp"
  "audio_capture.cpp"
)

set_target_properties(projectm_ffi PROPERTIES
  PUBLIC_HEADER projectm_ffi.h
  OUTPUT_NAME "projectm_ffi"
  DEBUG_POSTFIX ""
)

# Link projectM to our FFI library
target_link_libraries(projectm_ffi PRIVATE projectM)

if(UNIX AND NOT APPLE AND NOT ANDROID)
  target_link_libraries(projectm_ffi PRIVATE dl pthread m)
endif()
target_compile_definitions(projectm_ffi PUBLIC DART_SHARED_LIB)

if (ANDROID)
  # Support Android 15 16k page size
  target_link_options(projectm_ffi PRIVATE "-Wl,-z,max-page-size=16384")
endif()
