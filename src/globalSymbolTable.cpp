#include "../include/globalSymbolTable.h"

GlobalSymbolTable::GlobalSymbolTable() {}

FuncSymbol *GlobalSymbolTable::resolve_func(const std::string &name)
{
    auto it = functions.find(name);
    if (it == functions.end())
    {
        return nullptr;
    }
    return std::get<0>(it->second);
}

SymbolTable *GlobalSymbolTable::get_symbol_table(const std::string &name)
{
    auto it = functions.find(name);
    if (it == functions.end())
    {
        return nullptr;
    }
    return std::get<1>(it->second);
}