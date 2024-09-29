cmake_minimum_required(VERSION 3.10)

project(ssc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(SOURCES
    ../src/lexer.cpp
    ../src/main.cpp
)

add_executable(ssc ${SOURCES})

llvm_map_components_to_libnames(LLVM_LIBS core orcjit native)

target_link_libraries(ssc ${LLVM_LIBS})

