#pragma once

#include <unordered_map>
#include <tuple>
#include <memory>

#include "./symbolTable.h"

class GlobalSymbolTable
{
public:
    std::unordered_map<std::string, std::tuple<std::unique_ptr<FuncSymbol>, std::shared_ptr<SymbolTable>>> functions;
    // std::unordered_map<std::string, std::tuple<FuncSymbol *, SymbolTable *>> functions;
    std::unordered_map<std::string, std::unique_ptr<Symbol>> global_variables;

    GlobalSymbolTable();

    FuncSymbol *get_func_symbol(const std::string &func_name);
    SymbolTable *get_func_st(const std::string &func_name);

    Symbol *get_symbol(const std::string &func_name, const std::string &name);

    void enter_scope(const std::string &func_name);
    void exit_scope(const std::string &func_name);

    void declare_var(const std::string &func_name, VarNode *node);
    std::string check_var_defined(const std::string &func_name, const std::string &name);

    void print();
};