# The Flutter tooling requires that developers have CMake 3.10 or later
cmake_minimum_required(VERSION 3.10)

project(projectm_ffi_library VERSION 0.0.1 LANGUAGES C CXX)

# Fetch projectM v4.1.6
include(FetchContent)

set(ENABLE_SDL_UI OFF CACHE BOOL "Disable SDL UI" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable tests" FORCE)
set(ENABLE_GLES OFF CACHE BOOL "Disable GLES" FORCE) # Keep standard OpenGL for now, we can tweak this later for Android

FetchContent_Declare(
  projectM
  GIT_REPOSITORY https://github.com/projectM-visualizer/projectm.git
  GIT_TAG        v4.1.6
)
FetchContent_MakeAvailable(projectM)

add_library(projectm_ffi SHARED
  "projectm_ffi.cpp"
)

set_target_properties(projectm_ffi PROPERTIES
  PUBLIC_HEADER projectm_ffi.h
  OUTPUT_NAME "projectm_ffi"
)

# Link projectM to our FFI library
target_link_libraries(projectm_ffi PRIVATE projectM)
target_compile_definitions(projectm_ffi PUBLIC DART_SHARED_LIB)

if (ANDROID)
  # Support Android 15 16k page size
  target_link_options(projectm_ffi PRIVATE "-Wl,-z,max-page-size=16384")
endif()
