#--------------------------------------------------------------------------
# ai_proj project
#--------------------------------------------------------------------------

PROJECT(ai_proj)
FILE(GLOB project_headers code/*.h)
FILE(GLOB project_sources code/*.cc)

SET(files_project ${project_headers} ${project_sources})
SOURCE_GROUP("ai_proj" FILES ${files_project})

ADD_EXECUTABLE(ai_proj ${files_project})
TARGET_LINK_LIBRARIES(ai_proj core render)
ADD_DEPENDENCIES(ai_proj core render)

IF(MSVC)
    set_property(TARGET ai_proj PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
ENDIF()