#pragma once

#include <unordered_map>
#include <stack>
#include <string>

struct Symbol
{
    std::string name;
    bool isTemporary;

    Symbol(std::string n, bool t) : name(n), isTemporary(t) {}
};

class SymbolTable
{
public:
    SymbolTable();

    void enterScope();
    void exitScope();
    void declareVariable(const std::string &name, bool isTemporary = false);
    void resolveVariable(const std::string &name);

private:
    std::stack<std::unordered_map<std::string, Symbol *>> scopes;
};
