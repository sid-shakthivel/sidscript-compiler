#pragma once

#include <unordered_map>
#include <tuple>

#include "./symbolTable.h"

class GlobalSymbolTable
{
public:
    std::unordered_map<std::string, std::tuple<FuncSymbol *, SymbolTable *>> functions;

    GlobalSymbolTable();

    FuncSymbol *resolve_func(const std::string &name);
    SymbolTable *get_symbol_table(const std::string &name);
};