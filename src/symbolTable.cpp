#include "../include/symbolTable.h"

#include <stdexcept>
#include <iostream>

Symbol::Symbol(std::string n, int o, bool t) : name(n), stack_offset(o), is_temporary(t)
{
    if (is_temporary)
        unique_id = stack_offset / -4;

    // std::cout << "Symbol: " << name << " " << stack_offset << std::endl;
}

FuncSymbol::FuncSymbol(std::string n, int ac, std::vector<Type> &at, Type rt) : Symbol(n, 0, false), arg_count(ac), arg_types(at), return_type(rt) {}

std::string Symbol::gen_unique_name()
{
    if (is_temporary)
        return name;
    else
        return name + "_" + std::to_string(unique_id);
}

SymbolTable::SymbolTable() {}

void SymbolTable::enter_scope()
{
    scopes.emplace();
}

void SymbolTable::exit_scope()
{
    if (scopes.empty())
        throw std::runtime_error("Semantic Error: No scope to exit");

    auto &current_scope = scopes.top();
    for (auto it = current_scope.begin(); it != current_scope.end();)
    {
        var_symbols[it->first] = it->second; // Use unique name
        it = current_scope.erase(it);
    }

    scopes.pop();
}

void SymbolTable::declare_variable(const std::string &name, bool is_temporary)
{
    if (scopes.empty())
        throw std::runtime_error("Semantic Error: No scope available");

    auto &current_scope = scopes.top();

    if (current_scope.count(name))
        throw std::runtime_error("Semantic Error: Variable '" + name + "' is already declared in this scope");

    Symbol *test_symbol = new Symbol(name, var_count++ * -4, is_temporary);
    current_scope[name] = test_symbol;
}

void SymbolTable::resolve_variable(const std::string &name)
{
    auto &stack_container = scopes.__get_container(); // Non-standard but widely supported in STL
    for (auto it = stack_container.rbegin(); it != stack_container.rend(); ++it)
        if (it->count(name))
            return;
    throw std::runtime_error("Semantic Error: Variable '" + name + "' is not declared");
}

int SymbolTable::get_var_count()
{
    // return all_symbols.size();
    return var_count - 1;
}

Symbol *SymbolTable::find_symbol(const std::string &name)
{
    return var_symbols[name];
}

void SymbolTable::add_temporary_variable(const std::string &name)
{
    var_symbols[name] = new Symbol(name, var_count++ * -4, true);
}

FuncSymbol *SymbolTable::resolve_func(const std::string &name)
{
    if (!func_symbols.count(name))
        throw std::runtime_error("Semantic Error: Function '" + name + "' is not declared (yet)");

    return func_symbols[name];
}