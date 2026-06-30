set(GLEW_FOUND TRUE)
if(NOT TARGET GLEW::glew)
    add_library(GLEW::glew ALIAS glew)
endif()
