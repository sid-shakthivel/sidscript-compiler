cmake_minimum_required(VERSION 3.10)

project(ssc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SOURCES
    ../src/lexer.cpp
    ../src/ast.cpp
    ../src/parser.cpp
    ../src/assembler.cpp
    ../src/symbolTable.cpp
    ../src/semanticAnalyser.cpp
    ../src/tacGenerator.cpp
    ../src/main.cpp
)

add_executable(ssc ${SOURCES})

