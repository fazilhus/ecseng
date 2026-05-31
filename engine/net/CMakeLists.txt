#--------------------------------------------------------------------------
# net library — standalone networking (no render/glew/glfw dependencies)
# Only depends on enet and flatbuffers-generated protocol.
#--------------------------------------------------------------------------

flat_compile(protocol.fbs)

SET(files_net
    net.h
    net.cpp
)
SOURCE_GROUP("net" FILES ${files_net})

ADD_LIBRARY(net STATIC ${files_net})
ADD_DEPENDENCIES(net enet protocol)
TARGET_LINK_LIBRARIES(net PUBLIC enet protocol)
TARGET_INCLUDE_DIRECTORIES(net PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/exts/enet/include
    ${CMAKE_SOURCE_DIR}/exts/flatbuffers/include
)
TARGET_COMPILE_DEFINITIONS(net PUBLIC NOMINMAX)
SET_TARGET_PROPERTIES(net PROPERTIES FOLDER "engine")
