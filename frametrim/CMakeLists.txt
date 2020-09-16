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
   ft_framebufferstate.cpp
   ft_genobjectstate.cpp
   ft_matrixstate.cpp
   ft_main.cpp
   ft_objectstate.cpp
   ft_objectwithbindstate.cpp
   ft_programstate.cpp
   ft_state.cpp
   ft_samplerstate.cpp
   ft_texturestate.cpp
   )

add_executable(glxtrimreverse
   ftr_main.cpp
   ftr_state.cpp
   ftr_attachableobject.cpp
   ftr_bufobject.cpp
   ftr_genobject.cpp
   ftr_globalstateobject.cpp
   ftr_framebufferobject.cpp
   ftr_matrixobject.cpp
   ftr_programobj.cpp
   ftr_texobject.cpp
   ftr_tracecall.cpp
   ftr_tracecallext.cpp
   ftr_traceobject.cpp
   ftr_vertexattribarray.cpp
   )

target_link_libraries (glxtrim
    retrace_common
    glretrace_common
    )


target_link_libraries (glxtrimreverse
    retrace_common
    glretrace_common
    )

add_subdirectory (tests)
