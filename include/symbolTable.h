#pragma once

#include <unordered_map>
#include <stack>
#include <string>

class SymbolTable
{
public:
    SymbolTable();

    void enterScope();
    void exitScope();
    void declareVariable(const std::string &name);
    void resolveVariable(const std::string &name);

private:
    std::stack<std::unordered_map<std::string, bool>> scopes;
};
