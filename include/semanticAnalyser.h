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

    void analyse_func(FuncNode *func);
    void analyse_node(ASTNode *node);
    void analyse_var_decl(VarDeclNode *node);
    void analyser_var_assign(VarAssignNode *node);
    void analyse_rtn(RtnNode *node);
    void analyse_if_stmt(IfNode *node);
    void analyse_binary(BinaryNode *node);
    void analyse_unary(UnaryNode *node);
    void analyse_var(VarNode *node);
};