cmake_minimum_required(VERSION 3.10)

# For Clang to do parsing
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(System2)

set(SYSTEM2_USE_SOURCE OFF CACHE BOOL "")

if(SYSTEM2_USE_SOURCE)
    add_library(System2 "${CMAKE_CURRENT_LIST_DIR}/System2.c")
    target_include_directories(System2 PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
else()
    add_library(System2 INTERFACE)
    target_include_directories(System2 INTERFACE "${CMAKE_CURRENT_LIST_DIR}")
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set_target_properties(System2 PROPERTIES C_STANDARD 99)
    
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(STANDARD_COMPILE_FLAGS "/utf-8;/W1")
    else()
        set(STANDARD_COMPILE_FLAGS  "-Wall"
                                    "-Wno-return-local-addr"
                                    "-Wno-sign-compare"
                                    "-Wno-unused-variable"
                                    "-Wno-unused-parameter"
                                    "-Wno-switch"
                                    "-Wno-gnu-zero-variadic-macro-arguments"
                                    "-Wextra"
                                    "-pedantic"
                                    "-Werror")
    endif()
    
    target_compile_options(System2 INTERFACE ${STANDARD_COMPILE_FLAGS})
    set(SYSTEM2_BUILD_EXAMPLES ON CACHE BOOL "")
else()
    set(SYSTEM2_BUILD_EXAMPLES OFF CACHE BOOL "")
endif()

if(SYSTEM2_BUILD_EXAMPLES)
    add_executable(System2Example "${CMAKE_CURRENT_LIST_DIR}/main.c")
    
    if(SYSTEM2_USE_SOURCE)
        target_compile_definitions(System2Example PRIVATE USE_SYSTEM2_SOURCE=1)
    endif()
    
    target_link_libraries(System2Example System2)
endif()


