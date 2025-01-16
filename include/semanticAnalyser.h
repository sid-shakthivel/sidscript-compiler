#pragma once

#include "symbolTable.h"
#include "ast.h"

class SemanticAnalyser
{
public:
    SemanticAnalyser(SymbolTable *symbolTable);

    void analyse(ProgramNode *program);

private:
    SymbolTable *symbolTable;

    unsigned int loop_label_counter = 0;
    std::string gen_new_loop_label();
    std::stack<std::string> loop_scopes;

    void enter_loop_scope(std::string label);
    void exit_loop_scope();

    void analyse_func(FuncNode *func);
    void analyse_node(ASTNode *node);
    void analyse_var_decl(VarDeclNode *node);
    void analyser_var_assign(VarAssignNode *node);
    void analyse_rtn(RtnNode *node);
    void analyse_if_stmt(IfNode *node);
    void analyse_binary(BinaryNode *node);
    void analyse_unary(UnaryNode *node);
    void analyse_var(VarNode *node);
    void analyse_while_stmt(WhileNode *node);
    void analyse_for_stmt(ForNode *node);
    void analyser_loop_control(ASTNode *node);
};