cmake_minimum_required(VERSION 3.10)

# For Clang to do parsing
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(System2)

set(SYSTEM2_USE_SOURCE OFF CACHE BOOL "Build source version of System2")
set(SYSTEM2_POSIX_SPAWN OFF CACHE BOOL "Use posix_spawn() instead of fork()")
set(SYSTEM2_TEST_MEMORY OFF CACHE BOOL "Test memory commitment")
set(SYSTEM2_BUILD_EXAMPLES OFF CACHE BOOL "Build System2 examples?")

if(SYSTEM2_USE_SOURCE)
    add_library(System2 "${CMAKE_CURRENT_LIST_DIR}/System2.c")
    target_include_directories(System2 PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
    if(SYSTEM2_POSIX_SPAWN)
        target_compile_definitions(System2 PUBLIC SYSTEM2_POSIX_SPAWN=1)
    endif()
else()
    add_library(System2 INTERFACE)
    target_include_directories(System2 INTERFACE "${CMAKE_CURRENT_LIST_DIR}")
    if(SYSTEM2_POSIX_SPAWN)
        target_compile_definitions(System2 INTERFACE SYSTEM2_POSIX_SPAWN=1)
    endif()
endif()


if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set_target_properties(System2 PROPERTIES C_STANDARD 99)
    
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(STANDARD_COMPILE_FLAGS "/utf-8;/W4")
    else()
        set(STANDARD_COMPILE_FLAGS  "-Wall"
                                    "-Wextra"
                                    "-pedantic")
    endif()
    
    target_compile_options(System2 INTERFACE ${STANDARD_COMPILE_FLAGS})
    set(SYSTEM2_BUILD_EXAMPLES ON CACHE BOOL "")
else()
    set(SYSTEM2_BUILD_EXAMPLES OFF CACHE BOOL "")
endif()

if(SYSTEM2_BUILD_EXAMPLES)
    add_executable(System2Example "${CMAKE_CURRENT_LIST_DIR}/main.c")
    
    if(SYSTEM2_USE_SOURCE)
        target_compile_definitions(System2Example PRIVATE SYSTEM2_USE_SOURCE=1)
    endif()
    
    if(SYSTEM2_TEST_MEMORY)
        target_compile_definitions(System2Example PRIVATE SYSTEM2_TEST_MEMORY=1)
    endif()
    
    target_link_libraries(System2Example System2)
endif()


