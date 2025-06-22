# CMake script to apply patches in a platform-independent way

# Function to apply a patch file
function(apply_patch PATCH_FILE SOURCE_DIR)
    message(STATUS "Applying patch: ${PATCH_FILE}")
    
    # Copy patch file to source directory
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy ${PATCH_FILE} ${SOURCE_DIR}/
        RESULT_VARIABLE COPY_RESULT
    )
    
    if(NOT COPY_RESULT EQUAL 0)
        message(WARNING "Failed to copy patch file: ${PATCH_FILE}")
        return()
    endif()
    
    # Get patch filename
    get_filename_component(PATCH_NAME ${PATCH_FILE} NAME)
    
    # Try to apply patch
    if(WIN32)
        # On Windows, try git apply first
        find_program(GIT_EXECUTABLE git)
        if(GIT_EXECUTABLE)
            execute_process(
                COMMAND ${GIT_EXECUTABLE} apply ${PATCH_NAME}
                WORKING_DIRECTORY ${SOURCE_DIR}
                RESULT_VARIABLE PATCH_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
            )
        else()
            message(WARNING "Git not found on Windows, patch may not be applied")
            set(PATCH_RESULT 1)
        endif()
    else()
        # On Unix-like systems, use patch command
        execute_process(
            COMMAND patch -p1 -N
            INPUT_FILE ${SOURCE_DIR}/${PATCH_NAME}
            WORKING_DIRECTORY ${SOURCE_DIR}
            RESULT_VARIABLE PATCH_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )
    endif()
    
    if(PATCH_RESULT EQUAL 0)
        message(STATUS "Successfully applied patch: ${PATCH_FILE}")
    else()
        message(STATUS "Patch already applied or not needed: ${PATCH_FILE}")
    endif()
endfunction()

# Apply raylib CMake fix patch
apply_patch(${PATCH_DIR}/raylib-cmake-fix.patch ${SOURCE_DIR})