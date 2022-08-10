############################################################
# CMakeLists for the tools of ratpac

if(RATPAC_RAT)
        add_subdirectory(rat)
        add_subdirectory(ratdb2json)
        add_subdirectory(ntuple)
        list(APPEND TOOLS rat ratdb2json ntuple)
endif()
if(RATPAC_ROOT)
        add_subdirectory(root)
        list(APPEND TOOLS root)
endif()

set_target_properties(${TOOLS}
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${RATPAC_BINARY_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${RATPAC_LIBRARY_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${RATPAC_LIBRARY_DIR})
