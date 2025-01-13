#include "../include/symbolTable.h"

#include <stdexcept>

SymbolTable::SymbolTable() {}

void SymbolTable::enterScope() { scopes.emplace(); }

void SymbolTable::exitScope()
{
    if (scopes.empty())
        throw std::runtime_error("Semantic Error: No scope to exit");
    scopes.pop();
}

void SymbolTable::declareVariable(const std::string &name, bool isTemporary)
{
    if (scopes.empty())
        throw std::runtime_error("Semantic Error: No scope available");
    auto &current_scope = scopes.top();
    if (current_scope.count(name))
        throw std::runtime_error("Semantic Error: Variable '" + name + "' is already declared in this scope");
    Symbol *test = new Symbol(name, isTemporary);
    current_scope[name] = test;
}

void SymbolTable::resolveVariable(const std::string &name)
{
    auto &stack_container = scopes.__get_container(); // Non-standard but widely supported in STL
    for (auto it = stack_container.rbegin(); it != stack_container.rend(); ++it)
        if (it->count(name))
            return;
    throw std::runtime_error("Semantic Error: Variable '" + name + "' is not declared");
}