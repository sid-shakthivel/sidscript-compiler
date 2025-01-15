#include "../include/semanticAnalyser.h"

SemanticAnalyser::SemanticAnalyser(SymbolTable *symbolTable) : symbolTable(symbolTable) {}

void SemanticAnalyser::analyse(ProgramNode *program)
{
    analyse_func(program->func);
}

void SemanticAnalyser::analyse_func(FuncNode *func)
{
    symbolTable->enter_scope();
    for (auto element : func->elements)
        analyse_node(element);
    symbolTable->exit_scope();
}

void SemanticAnalyser::analyse_node(ASTNode *node)
{
    if (node->type == NodeType::NODE_VAR_DECL)
        analyse_var_decl((VarDeclNode *)node);
    else if (node->type == NodeType::NODE_VAR_ASSIGN)
        analyser_var_assign((VarAssignNode *)node);
    else if (node->type == NodeType::NODE_IF)
        analyse_if_stmt((IfNode *)node);
    else if (node->type == NodeType::NODE_RETURN)
        analyse_rtn((RtnNode *)node);
    else if (node->type == NodeType::NODE_BINARY)
        analyse_binary((BinaryNode *)node);
    else if (node->type == NodeType::NODE_UNARY)
        analyse_unary((UnaryNode *)node);
    else if (node->type == NodeType::NODE_VAR)
        analyse_var((VarNode *)node);
}

void SemanticAnalyser::analyse_var_decl(VarDeclNode *node)
{
    symbolTable->declare_variable(node->var->name);
    if (node->value)
        analyse_node(node->value);
}

void SemanticAnalyser::analyser_var_assign(VarAssignNode *node)
{
    symbolTable->resolve_variable(node->var->name);
    analyse_node(node->value);
}

void SemanticAnalyser::analyse_rtn(RtnNode *node)
{
    if (node->value)
        analyse_node(node->value);
}

void SemanticAnalyser::analyse_if_stmt(IfNode *node)
{
    analyse_node(node->condition);

    symbolTable->enter_scope();
    for (auto stmt : node->then_elements)
        analyse_node(stmt);
    symbolTable->exit_scope();

    if (!node->else_elements.empty())
    {
        symbolTable->enter_scope();
        for (auto stmt : node->else_elements)
            analyse_node(stmt);
        symbolTable->exit_scope();
    }
}

void SemanticAnalyser::analyse_while_stmt(WhileNode *node)
{
    analyse_node(node->condition);

    symbolTable->enter_scope();
    for (auto stmt : node->elements)
        analyse_node(stmt);
    symbolTable->exit_scope();
}

void SemanticAnalyser::analyse_for_stmt(ForNode *node)
{
    symbolTable->enter_scope();
    analyse_node(node->init);
    analyse_node(node->condition);
    analyse_node(node->post);
    for (auto stmt : node->elements)
        analyse_node(stmt);
    symbolTable->exit_scope();
}

void SemanticAnalyser::analyse_binary(BinaryNode *node)
{
    analyse_node(node->left);
    analyse_node(node->right);
}

void SemanticAnalyser::analyse_unary(UnaryNode *node)
{
    analyse_node(node->value);
}

void SemanticAnalyser::analyse_var(VarNode *node)
{
    symbolTable->resolve_variable(node->name);
}