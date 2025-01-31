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
    Linkage linkage = Linkage::None;
    StorageDuration storage_duration = StorageDuration::Automatic;
    std::string unique_name;
    Type type = Type::VOID;

    Symbol(std::string n, int o, bool it);
    void set_linkage(Linkage l);
    void set_storage_duration(StorageDuration sd);
    void set_type(Type t);
    bool has_static_sd();
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

    std::tuple<bool, std::string> declare_var(const std::string &name, bool is_static = false, Type type = Type::INT);
    std::tuple<bool, std::string> check_var_defined(const std::string &name);
    void declare_temp_variable(const std::string &name, Type type);
    int get_stack_size();
    Symbol *get_symbol(const std::string &name);

    void print();

private:
    std::stack<std::unordered_map<std::string, std::shared_ptr<Symbol>>> scopes;
    std::unordered_map<std::string, std::shared_ptr<Symbol>> var_symbols;

    int align_to(int size, int alignment);
    void adjust_stack(Type &type);

    int var_count = 0;
    int stack_size = 0;
};
