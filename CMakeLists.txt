cmake_minimum_required(VERSION 3.20)
project(Symbolic VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)

option(SYMBOLIC_BUILD_EXAMPLE "Build example subproject." ON)

set(SYMBOLIC_HEADERS_BASE ${CMAKE_CURRENT_SOURCE_DIR}/include/symbolic)
set(SYMBOLIC_SOURCES_BASE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SYMBOLIC_HEADER_FILES
    ${SYMBOLIC_HEADERS_BASE}/alloc.hpp
    ${SYMBOLIC_HEADERS_BASE}/asserts.hpp
    ${SYMBOLIC_HEADERS_BASE}/names.hpp
)

set(SYMBOLIC_SOURCE_FILES
    ${SYMBOLIC_SOURCES_BASE}/names.cpp
)

add_library(symbolic                    STATIC ${SYMBOLIC_HEADER_FILES} ${SYMBOLIC_SOURCE_FILES})
target_include_directories(symbolic     PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (SYMBOLIC_BUILD_EXAMPLE)
    add_subdirectory("example/")
endif()
