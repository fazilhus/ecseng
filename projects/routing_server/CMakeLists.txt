#--------------------------------------------------------------------------
# routing_server project — headless P2P routing server
#--------------------------------------------------------------------------

PROJECT(p2p_routing_proj)
FILE(GLOB project_headers code/*.h)
FILE(GLOB project_sources code/*.cc)

SET(files_project ${project_headers} ${project_sources})
SOURCE_GROUP("p2p_routing_proj" FILES ${files_project})

ADD_EXECUTABLE(p2p_routing_proj ${files_project})
TARGET_LINK_LIBRARIES(p2p_routing_proj net)
ADD_DEPENDENCIES(p2p_routing_proj net)

IF(MSVC)
    set_property(TARGET p2p_routing_proj PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
ENDIF()
