#include "../include/symbolTable.h"

#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <ios>

Symbol::Symbol(std::string n, int o, Type t) : name(n), stack_offset(o), type(t) {}

void Symbol::set_linkage(Linkage l) { linkage = l; }
void Symbol::set_storage_duration(StorageDuration sd) { storage_duration = sd; }
void Symbol::set_is_temp(bool it) { is_temporary = it; }

bool Symbol::has_static_sd() { return storage_duration == StorageDuration::Static; }

FuncSymbol::FuncSymbol(std::string n, int ac, std::vector<Type> &at, Type rt) : Symbol(n, 0, rt), arg_count(ac), arg_types(at), return_type(rt) {}

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

std::tuple<bool, std::string> SymbolTable::declare_var(const std::string &name, Type type, bool is_static)
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

    std::shared_ptr<Symbol> symbol = std::make_shared<Symbol>(name, stack_size * -1, type);
    symbol->set_storage_duration(is_static ? StorageDuration::Static : StorageDuration::Automatic);

    auto it = var_symbols.find(name);
    if (it != var_symbols.end())
        symbol->unique_name = name + std::to_string(var_count);
    else
        symbol->unique_name = name;

    current_scope[name] = symbol;
    var_symbols[symbol->unique_name] = symbol;
    var_count += 1;

    return std::make_tuple(symbol->unique_name == symbol->name, symbol->unique_name);
}

std::tuple<bool, std::string> SymbolTable::check_var_defined(const std::string &name)
{
    auto &stack_container = scopes.__get_container();
    for (auto it = stack_container.rbegin(); it != stack_container.rend(); ++it)
        if (it->count(name))
        {
            auto it2 = it->find(name);
            if (it2 != it->end())
                return {true, it2->second->unique_name};
        }

    return {false, ""};
}

Symbol *SymbolTable::get_symbol(const std::string &name)
{
    auto it = var_symbols.find(name);
    return it != var_symbols.end() ? it->second.get() : nullptr;
}

void SymbolTable::declare_temp_var(const std::string &name, Type type)
{
    adjust_stack(type);
    std::shared_ptr<Symbol> new_temp_var = std::make_shared<Symbol>(name, stack_size * -1, type);
    new_temp_var->set_is_temp(true);
    var_symbols[name] = new_temp_var;
    var_count += 1;
}

void SymbolTable::declare_const_var(const std::string &name, Type type)
{
    std::shared_ptr<Symbol> new_const_var = std::make_shared<Symbol>(name, 0, type);
    new_const_var->is_literal8 = true;
    var_symbols[name] = new_const_var;
}

void SymbolTable::declare_str_var(const std::string &name, Type type)
{
    var_symbols[name] = std::make_shared<Symbol>(name, 0, type);
}

void SymbolTable::print()
{
    std::cout << "\n=== Symbol Table Debug ===\n"
              << "Symbol table size: " << var_symbols.size() << "\nSymbols:\n";

    try
    {
        for (const auto &[name, symbol] : var_symbols)
        {
            if (!symbol)
            {
                std::cout << "  WARNING: Null pointer found for key: " << name << '\n';
                continue;
            }

            std::cout << "  " << std::left << std::setw(20) << name
                      << " | name: " << std::setw(15) << symbol->name
                      << " | offset: " << std::setw(5) << symbol->stack_offset
                      << " | temp: " << std::setw(5) << (symbol->is_temporary ? "yes" : "no")
                      << " | size: " << symbol->type.get_size() << '\n';
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception while printing symbols: " << e.what() << '\n';
    }
    std::cout << "========================\n";
}

int SymbolTable::get_stack_size()
{
    return align_to(stack_size, DEFAULT_ALIGNMENT);
}

void SymbolTable::adjust_stack(Type &type)
{
    stack_size = align_to(stack_size, type.get_size());
    stack_size += type.get_size();
}

int SymbolTable::align_to(int size, int alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}
