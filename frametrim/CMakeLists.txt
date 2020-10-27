include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/cli
    ${CMAKE_SOURCE_DIR}/lib/highlight
    ${CMAKE_SOURCE_DIR}/helpers
    ${CMAKE_SOURCE_DIR}/retrace
    ${CMAKE_BINARY_DIR}/dispatch
    ${CMAKE_SOURCE_DIR}/dispatch
    ${CMAKE_SOURCE_DIR}/lib/image
    ${CMAKE_SOURCE_DIR}/lib/ubjson
    ${CMAKE_SOURCE_DIR}/thirdparty/dxerr
    ${CMAKE_SOURCE_DIR}/thirdparty/mhook/mhook-lib
)

cmake_policy(SET CMP0079 NEW)

add_executable(glxtrim
   ft_bufferstate.cpp
   ft_framebuffer.cpp
   ft_frametrimmer.cpp
   ft_genobjectstate.cpp
   ft_matrixstate.cpp
   ft_main.cpp
   ft_objectstate.cpp
   ft_objectwithbindstate.cpp
   ft_programstate.cpp
   ft_renderbuffer.cpp
   ft_samplerstate.cpp
   ft_syncobject.cpp
   ft_texturestate.cpp
   ft_tracecall.cpp
   ft_traceoptions.cpp
   ft_vertexattribpointer.cpp
   )

set_target_properties(glxtrim PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

target_link_libraries (glxtrim
    retrace_common
    glretrace_common
    )

add_subdirectory (tests)
