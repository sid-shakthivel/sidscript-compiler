#pragma once

#include <unordered_map>
#include <tuple>
#include <memory>

#include "./symbolTable.h"

class GlobalSymbolTable
{
public:
    GlobalSymbolTable();

    void create_new_func(const std::string &func_name, std::unique_ptr<FuncSymbol>, std::shared_ptr<SymbolTable>);

    void enter_func_scope(const std::string &func_name);
    void leave_func_scope();
    bool is_global_scope() const;
    std::string get_current_func() const;

    void declare_var(VarNode *node);
    void declare_temp_var(const std::string &name, const Type &type);
    void declare_const_var(const std::string &name, const Type &type);
    void declare_str_var(const std::string &name, const Type &type);

    std::string check_var_defined(const std::string &name);
    bool check_struct_defined(const std::string &name);

    FuncSymbol *get_func_symbol(const std::string &func_name);
    SymbolTable *get_func_st(const std::string &func_name);

    Symbol *get_symbol(const std::string &name);

    void enter_scope();
    void exit_scope();

    void add_import(const std::string &imported_module_name, const std::vector<std::string> &imported_names);
    void check_imports();

    void print();

    std::string current_module = "";

    /*
        This is a map of struct names to a map of field names to their respective types
        (All struct members are public for now)
    */
    std::unordered_map<std::string, std::pair<std::map<std::string, Type>, std::string>> struct_table;

private:
    std::string current_func = "";
    std::unordered_map<std::string, std::tuple<std::unique_ptr<FuncSymbol>, std::shared_ptr<SymbolTable>, std::string>> functions;
    std::unordered_map<std::string, std::tuple<std::unique_ptr<Symbol>, std::string>> global_variables;

    /*
        This is a map of maps of vectors of strings.
        The first map is the module name to a map of modules (imported) to a vector of strings (names of variables/functions imported)
    */
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>> import_table;

    void handle_global_var_decl(VarNode *node);
    void handle_local_var_decl(VarNode *node);
};