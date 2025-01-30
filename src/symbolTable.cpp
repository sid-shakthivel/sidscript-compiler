#include "../include/symbolTable.h"

#include <stdexcept>
#include <iostream>

Symbol::Symbol(std::string n, int o, bool it) : name(n), stack_offset(o), is_temporary(it)
{
    // std::cout << "Symbol: " << name << " " << stack_offset << std::endl;
}

void Symbol::set_linkage(Linkage l) { linkage = l; }
void Symbol::set_storage_duration(StorageDuration sd) { storage_duration = sd; }
void Symbol::set_type(Type t) { type = t; }

bool Symbol::has_static_sd()
{
    return storage_duration == StorageDuration::Static;
}

FuncSymbol::FuncSymbol(std::string n, int ac, std::vector<Type> &at, Type rt) : Symbol(n, 0, false), arg_count(ac), arg_types(at), return_type(rt) {}

SymbolTable::SymbolTable() {}

void SymbolTable::enter_scope()
{
    scopes.emplace();
}

void SymbolTable::exit_scope()
{
    if (scopes.empty())
        throw std::runtime_error("Semantic Error: No scope to exit");

    scopes.pop();
}

std::tuple<bool, std::string> SymbolTable::declare_var(const std::string &name, bool is_static, Type type)
{
    if (scopes.empty())
        throw std::runtime_error("Semantic Error: No scope available");

    auto &current_scope = scopes.top();

    if (current_scope.count(name))
    {
        Symbol *existing_symbol = current_scope[name];

        if (existing_symbol->storage_duration == StorageDuration::Static && !is_static)
            throw std::runtime_error("Semantic Error: Variable '" + name + "' with static storage duration conflicts with an automatic variable");

        throw std::runtime_error("Semantic Error: Variable '" + name + "' is already declared in this scope");
    }

    var_count += type == Type::INT ? 1 : 3;

    Symbol *symbol = new Symbol(name, var_count * -4, false);
    symbol->set_storage_duration(is_static ? StorageDuration::Static : StorageDuration::Automatic);
    current_scope[name] = symbol;

    bool has_name_changed = false;

    auto it = var_symbols.find(name);
    if (it != var_symbols.end())
    {
        symbol->unique_name = name + std::to_string(var_count);
        has_name_changed = true;
    }
    else
        symbol->unique_name = name;

    var_symbols[symbol->unique_name] = symbol;

    return std::make_tuple(has_name_changed, symbol->unique_name);
}

std::tuple<bool, std::string> SymbolTable::check_var_defined(const std::string &name)
{
    auto &stack_container = scopes.__get_container(); // Non-standard but widely supported in STL
    for (auto it = stack_container.rbegin(); it != stack_container.rend(); ++it)
        if (it->count(name))
        {
            auto it2 = it->find(name);
            if (it2 != it->end())
                return std::make_tuple(true, it2->second->unique_name);
        }

    return std::make_tuple(false, "");
}

int SymbolTable::get_var_count()
{
    return var_count;
}

Symbol *SymbolTable::get_symbol(const std::string &name)
{
    return var_symbols[name];
}

void SymbolTable::declare_temp_variable(const std::string &name)
{
    var_symbols[name] = new Symbol(name, var_count++ * -4, true);
}

void SymbolTable::print()
{
    for (auto it = var_symbols.begin(); it != var_symbols.end(); ++it)
        std::cout << " - " << it->first << std::endl;
}