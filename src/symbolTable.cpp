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

bool Symbol::has_static_sd() { return storage_duration == StorageDuration::Static; }

FuncSymbol::FuncSymbol(std::string n, int ac, std::vector<Type> &at, Type rt) : Symbol(n, 0, false), arg_count(ac), arg_types(at), return_type(rt) {}

SymbolTable::SymbolTable()
{
    stack_size = 0;
}

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
        Symbol *existing_symbol = current_scope[name].get();

        if (existing_symbol->storage_duration == StorageDuration::Static && !is_static)
            throw std::runtime_error("Semantic Error: Variable '" + name + "' with static storage duration conflicts with an automatic variable");

        throw std::runtime_error("Semantic Error: Variable '" + name + "' is already declared in this scope");
    }

    adjust_stack(type);

    std::shared_ptr<Symbol> symbol = std::make_shared<Symbol>(name, stack_size * -1, false);
    symbol->set_storage_duration(is_static ? StorageDuration::Static : StorageDuration::Automatic);

    bool has_name_changed = false;

    auto it = var_symbols.find(name);
    if (it != var_symbols.end())
    {
        symbol->unique_name = name + std::to_string(var_count);
        has_name_changed = true;
    }
    else
        symbol->unique_name = name;

    current_scope[name] = symbol;
    var_symbols[symbol->unique_name] = symbol;
    var_count += 1;

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

int SymbolTable::get_stack_size()
{
    return align_to(stack_size, 16);
}

Symbol *SymbolTable::get_symbol(const std::string &name)
{
    if (var_symbols.find(name) != var_symbols.end())
        return var_symbols[name].get();

    return nullptr;
}

void SymbolTable::declare_temp_var(const std::string &name, Type type)
{
    adjust_stack(type);
    var_symbols[name] = std::make_shared<Symbol>(name, stack_size * -1, true);
    var_count += 1;
}

void SymbolTable::declare_const_var(const std::string &name, Type type)
{
    std::shared_ptr<Symbol> new_const_var = std::make_shared<Symbol>(name, 0, false);
    new_const_var->is_literal8 = true;
    var_symbols[name] = new_const_var;
}

void SymbolTable::print()
{
    std::cout << "\n=== Symbol Table Debug ===\n";
    std::cout << "Symbol table size: " << var_symbols.size() << std::endl;
    std::cout << "Symbols:" << std::endl;
    try
    {
        for (const auto &pair : var_symbols)
        {
            if (!pair.second)
            {
                std::cout << "  WARNING: Null pointer found for key: " << pair.first << std::endl;
                continue;
            }
            std::cout << "  " << pair.first
                      << " (name: " << pair.second->name
                      << ", offset: " << pair.second->stack_offset
                      << ", temp: " << pair.second->is_temporary
                      << ", addr: " << pair.second.get() << ")" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception while printing symbols: " << e.what() << std::endl;
    }
    std::cout << "========================\n";
}

int SymbolTable::align_to(int size, int alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

void SymbolTable::adjust_stack(Type &type)
{
    stack_size = align_to(stack_size, type.get_size());
    stack_size += type.get_size();
}
