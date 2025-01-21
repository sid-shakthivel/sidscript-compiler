#pragma once

#include <unordered_map>
#include <stack>
#include <string>

#include "ast.h"

struct Symbol
{
    std::string name;
    int stack_offset;
    bool is_temporary;
    int unique_id = 0;

    Symbol(std::string n, int o, bool t);
    std::string gen_unique_name();
};

struct FuncSymbol : public Symbol
{
    int arg_count = 0;
    std::vector<Type> arg_types;
    Type return_type;

    FuncSymbol(std::string n, int ac, std::vector<Type> &at, Type rt);
};

class SymbolTable
{
public:
    // std::unordered_map<std::string, FuncSymbol *> func_symbols;

    SymbolTable();

    void enter_scope();
    void exit_scope();
    void declare_variable(const std::string &name);
    void resolve_variable(const std::string &name);
    int get_var_count();
    Symbol *find_symbol(const std::string &name);
    void declare_temp_variable(const std::string &name);
    // FuncSymbol *resolve_func(const std::string &name);

private:
    std::stack<std::unordered_map<std::string, Symbol *>> scopes;
    std::unordered_map<std::string, Symbol *> var_symbols;
    int var_count = 1;
};
