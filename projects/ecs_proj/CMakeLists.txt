#--------------------------------------------------------------------------
# ecs_proj project
#--------------------------------------------------------------------------

PROJECT(ecs_proj)
FILE(GLOB project_headers code/*.h)
FILE(GLOB project_sources code/*.cc)

SET(files_project ${project_headers} ${project_sources})
SOURCE_GROUP("ecs_proj" FILES ${files_project})

ADD_EXECUTABLE(ecs_proj ${files_project})
TARGET_LINK_LIBRARIES(ecs_proj core render)
ADD_DEPENDENCIES(ecs_proj core render)

IF(MSVC)
    set_property(TARGET ecs_proj PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
ENDIF()