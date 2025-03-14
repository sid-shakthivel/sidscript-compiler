#pragma once

#include <unordered_map>
#include <tuple>
#include <memory>

#include "./symbolTable.h"

class GlobalSymbolTable
{
public:
    GlobalSymbolTable();

    void create_new_func(const std::string &func_name, std::unique_ptr<FuncSymbol>, std::shared_ptr<SymbolTable>);

    void enter_func_scope(const std::string &func_name);
    void leave_func_scope();
    bool is_global_scope() const;
    std::string get_current_func() const;

    void declare_var(VarNode *node);
    void declare_temp_var(const std::string &name, Type type);
    void declare_const_var(const std::string &name, Type type);
    void declare_str_var(const std::string &name, Type type);

    std::string check_var_defined(const std::string &name);

    FuncSymbol *get_func_symbol(const std::string &func_name);
    SymbolTable *get_func_st(const std::string &func_name);

    Symbol *get_symbol(const std::string &name);

    void enter_scope();
    void exit_scope();

    void print();

private:
    std::string current_func = "";
    std::unordered_map<std::string, std::tuple<std::unique_ptr<FuncSymbol>, std::shared_ptr<SymbolTable>>> functions;
    std::unordered_map<std::string, std::unique_ptr<Symbol>> global_variables;

    void handle_global_var_decl(VarNode *node);
    void handle_local_var_decl(VarNode *node);
};