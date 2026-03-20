#--------------------------------------------------------------------------
# net_proj project
#--------------------------------------------------------------------------

PROJECT(net_proj)
FILE(GLOB project_headers code/*.h)
FILE(GLOB project_sources code/*.cc)

SET(files_project ${project_headers} ${project_sources})
SOURCE_GROUP("net_proj" FILES ${files_project})

ADD_EXECUTABLE(net_proj ${files_project})
TARGET_LINK_LIBRARIES(net_proj core render)
ADD_DEPENDENCIES(net_proj core render)

IF(MSVC)
    set_property(TARGET net_proj PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
ENDIF()