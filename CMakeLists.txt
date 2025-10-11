cmake_minimum_required(VERSION 3.10)

project(ssc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES
    ../src/lexer.cpp
    ../src/type.cpp
    ../src/ast.cpp
    ../src/parser.cpp
    ../src/symbolTable.cpp
    ../src/globalSymbolTable.cpp
    ../src/semanticAnalyser.cpp
    ../src/tacGenerator.cpp
    ../src/assembler.cpp
    ../src/module.cpp
    ../src/main.cpp
)

add_executable(ssc ${SOURCES})

