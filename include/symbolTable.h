#pragma once

#include <unordered_map>
#include <stack>
#include <string>

struct Symbol
{
    std::string name;
    int stack_offset;
    bool is_temporary;
    int unique_id = 0;

    Symbol(std::string n, int o, bool t);
    std::string gen_unique_name();
};

class SymbolTable
{
public:
    SymbolTable();

    void enter_scope();
    void exit_scope();
    void declare_variable(const std::string &name, bool is_temporary = false);
    void resolve_variable(const std::string &name);
    int get_var_count();
    Symbol *find_symbol(const std::string &name);
    void add_temporary_variable(const std::string &name);

private:
    std::stack<std::unordered_map<std::string, Symbol *>> scopes;
    std::unordered_map<std::string, Symbol *> all_symbols;
    int var_count = 1;
};
