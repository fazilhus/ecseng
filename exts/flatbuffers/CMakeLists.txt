
#get_filename_component(flatcFolderName "flatc.exe" DIRECTORY)
get_filename_component(FLATC flatc.exe REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
if ("${FLATC}" STREQUAL "")
	message(FATAL_ERROR "Could not find flatc.exe!")
else()
	message(STATUS "Using flatc located at ${FLATC}")
endif()
set_property(GLOBAL PROPERTY FLAT_COMPILER "${FLATC}")

function(flat_compile)
    get_property(FLATC GLOBAL PROPERTY FLAT_COMPILER)

    set(files "")
    set(fbs "")
    set(include_dirs "") # Accumulate include directories here

    foreach(fb ${ARGN})
        get_filename_component(filename ${fb} NAME)
        get_filename_component(foldername ${fb} DIRECTORY)
        string(REPLACE ".fbs" "_gen.h" out_header ${filename})

        set(abs_output_folder "${CMAKE_BINARY_DIR}/generated/flat${foldername}")
        list(APPEND include_dirs "${abs_output_folder}") # Add to our list

        list(APPEND fbs ${fb})
        set(output ${abs_output_folder}/${out_header})

        add_custom_command(
                OUTPUT ${output}
                COMMAND ${FLATC} -c --gen-object-api --gen-mutable --include-prefix "" --keep-prefix --cpp-str-flex-ctor -I ${CMAKE_CURRENT_SOURCE_DIR} --filename-suffix "" -o ${abs_output_folder} "${CMAKE_CURRENT_SOURCE_DIR}/${fb}"
                MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${fb}"
                DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${fb}"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Compiling ${fb}"
                VERBATIM
        )
        list(APPEND files ${output})
    endforeach()

    # Remove duplicate directories so we don't clutter the compile command
    list(REMOVE_DUPLICATES include_dirs)

    set(files_proto ${files} PARENT_SCOPE)
    source_group("generated" FILES ${files})
    source_group("protocol" FILES ${fbs})

    # Fixed: Removed "FILES"
    add_custom_target(gen_files
                      DEPENDS ${files}
    )

    add_library(protocol INTERFACE)

    # Fixed: Include all unique generated directories
    target_include_directories(protocol INTERFACE ${include_dirs})

    add_dependencies(protocol gen_files)

endfunction()