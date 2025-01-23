#pragma once

#include <unordered_map>
#include <tuple>

#include "./symbolTable.h"

class GlobalSymbolTable
{
public:
    std::unordered_map<std::string, std::tuple<FuncSymbol *, SymbolTable *>> functions;
    std::unordered_map<std::string, Symbol *> global_variables;

    GlobalSymbolTable();

    FuncSymbol *get_func_symbol(const std::string &func_name);
    SymbolTable *get_func_st(const std::string &func_name);

    Symbol *get_symbol(const std::string &func_name, const std::string &name);

    void enter_scope(const std::string &func_name);
    void exit_scope(const std::string &func_name);

    void declare_var(const std::string &func_name, VarNode*node);
    void check_var_defined(const std::string &func_name, const std::string &name);
};