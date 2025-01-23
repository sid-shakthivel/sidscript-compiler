#pragma once

#include <unordered_map>
#include <stack>
#include <string>

#include "ast.h"

/*
Linkage determines whether a name refers to the same object
across multiple files or within a single file
*/
enum class Linkage
{
    None,     // Unique function/variable within scope (block scope)
    Internal, // Same entity only within current file (static variables in file)
    External  // Same entity across multiple files (proper global variables/function)
};

/*
Storage duration determines how long an enity exists in memory
*/
enum class StorageDuration
{
    Automatic, // Block scope
    Static     // Persist throughout program's execution
};

struct Symbol
{
    std::string name;
    int stack_offset;
    bool is_temporary;
    int unique_id = 0;
    Linkage linkage = Linkage::None;
    StorageDuration storage_duration = StorageDuration::Automatic;

    Symbol(std::string n, int o, bool t);
    void set_linkage(Linkage l);
    void set_storage_duration(StorageDuration sd);
    // std::string gen_unique_name();
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
    SymbolTable();

    void enter_scope();
    void exit_scope();

    void declare_var(const std::string &name, bool is_static = false);
    bool check_var_defined(const std::string &name);
    void declare_temp_variable(const std::string &name);

    int get_var_count();
    Symbol *get_symbol(const std::string &name);

private:
    std::stack<std::unordered_map<std::string, Symbol *>> scopes;
    std::unordered_map<std::string, Symbol *> var_symbols;
    int var_count = 1;
};
