#--------------------------------------------------------------------------
# net
#--------------------------------------------------------------------------

flat_compile(protocol.fbs)

SET(files_net
    netbuf.h
    net.h
    net.cpp
)
SOURCE_GROUP("net" FILES ${files_net})

SET(files_pch ../config.h ../config.cc)
SOURCE_GROUP("pch" FILES ${files_pch})
ADD_LIBRARY(net STATIC ${files_net} ${files_pch})
#TARGET_PCH(net ../)
ADD_DEPENDENCIES(net enet)
TARGET_LINK_LIBRARIES(net PUBLIC engine enet)
