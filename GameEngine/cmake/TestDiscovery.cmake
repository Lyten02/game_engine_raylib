# TestDiscovery.cmake - Automatic test discovery module
# This module automatically finds and registers tests from the tests/ directory

# Function to discover Python tests
function(discover_python_tests TEST_DIR OUTPUT_VAR)
    file(GLOB_RECURSE PYTHON_TEST_FILES
        "${TEST_DIR}/unit/*.py"
        "${TEST_DIR}/integration/*.py" 
        "${TEST_DIR}/system/*.py"
    )
    
    # Filter out __pycache__ and non-test files
    set(FILTERED_TESTS)
    foreach(TEST_FILE ${PYTHON_TEST_FILES})
        if(TEST_FILE MATCHES "test_.*\\.py$" AND NOT TEST_FILE MATCHES "__pycache__")
            list(APPEND FILTERED_TESTS ${TEST_FILE})
        endif()
    endforeach()
    
    set(${OUTPUT_VAR} ${FILTERED_TESTS} PARENT_SCOPE)
endfunction()

# Function to discover C++ tests
function(discover_cpp_tests TEST_DIR OUTPUT_VAR)
    file(GLOB_RECURSE CPP_TEST_FILES
        "${TEST_DIR}/unit/*.cpp"
        "${TEST_DIR}/integration/*.cpp"
        "${TEST_DIR}/system/*.cpp"
    )
    
    # Filter test files (exclude main files or helpers)
    set(FILTERED_TESTS)
    foreach(TEST_FILE ${CPP_TEST_FILES})
        if(TEST_FILE MATCHES "test_.*\\.cpp$")
            list(APPEND FILTERED_TESTS ${TEST_FILE})
        endif()
    endforeach()
    
    set(${OUTPUT_VAR} ${FILTERED_TESTS} PARENT_SCOPE)
endfunction()

# Function to discover script tests
function(discover_script_tests TEST_DIR OUTPUT_VAR)
    file(GLOB_RECURSE SCRIPT_TEST_FILES
        "${TEST_DIR}/unit/*.txt"
        "${TEST_DIR}/integration/*.txt"
        "${TEST_DIR}/system/*.txt"
    )
    
    # Filter for actual test scripts
    set(FILTERED_TESTS)
    foreach(TEST_FILE ${SCRIPT_TEST_FILES})
        if(TEST_FILE MATCHES "test.*\\.txt$" OR TEST_FILE MATCHES ".*_test\\.txt$")
            list(APPEND FILTERED_TESTS ${TEST_FILE})
        endif()
    endforeach()
    
    set(${OUTPUT_VAR} ${FILTERED_TESTS} PARENT_SCOPE)
endfunction()

# Function to get test category from path
function(get_test_category TEST_PATH OUTPUT_VAR)
    if(TEST_PATH MATCHES "/unit/")
        set(${OUTPUT_VAR} "unit" PARENT_SCOPE)
    elseif(TEST_PATH MATCHES "/integration/")
        set(${OUTPUT_VAR} "integration" PARENT_SCOPE)
    elseif(TEST_PATH MATCHES "/system/")
        set(${OUTPUT_VAR} "system" PARENT_SCOPE)
    else()
        set(${OUTPUT_VAR} "unknown" PARENT_SCOPE)
    endif()
endfunction()

# Function to create test execution command
function(create_test_command TEST_FILE TEST_TYPE OUTPUT_VAR)
    if(TEST_TYPE STREQUAL "python")
        set(${OUTPUT_VAR} "${Python3_EXECUTABLE} ${TEST_FILE}" PARENT_SCOPE)
    elseif(TEST_TYPE STREQUAL "script")
        set(${OUTPUT_VAR} "${CMAKE_BINARY_DIR}/game_engine --script ${TEST_FILE}" PARENT_SCOPE)
    endif()
endfunction()

# Main discovery function
function(discover_all_tests TEST_DIR)
    message(STATUS "Discovering tests in ${TEST_DIR}")
    
    # Discover tests by type
    discover_python_tests(${TEST_DIR} PYTHON_TESTS)
    discover_cpp_tests(${TEST_DIR} CPP_TESTS)
    discover_script_tests(${TEST_DIR} SCRIPT_TESTS)
    
    # Report findings
    list(LENGTH PYTHON_TESTS PYTHON_COUNT)
    list(LENGTH CPP_TESTS CPP_COUNT)
    list(LENGTH SCRIPT_TESTS SCRIPT_COUNT)
    
    message(STATUS "Found ${PYTHON_COUNT} Python tests")
    message(STATUS "Found ${CPP_COUNT} C++ tests")
    message(STATUS "Found ${SCRIPT_COUNT} Script tests")
    
    # Export lists to parent scope
    set(ALL_PYTHON_TESTS ${PYTHON_TESTS} PARENT_SCOPE)
    set(ALL_CPP_TESTS ${CPP_TESTS} PARENT_SCOPE)
    set(ALL_SCRIPT_TESTS ${SCRIPT_TESTS} PARENT_SCOPE)
endfunction()