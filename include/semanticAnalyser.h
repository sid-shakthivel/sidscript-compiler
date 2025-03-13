#pragma once

#include <memory>
#include <functional>

#include "globalSymbolTable.h"
#include "symbolTable.h"
#include "ast.h"

class SemanticAnalyser
{
public:
    SemanticAnalyser(std::shared_ptr<GlobalSymbolTable> gst);

    void analyse(std::shared_ptr<ProgramNode> program);

private:
    std::unordered_map<NodeType, std::function<void(ASTNode *)>> handlers;

    std::shared_ptr<GlobalSymbolTable> gst;
    std::string current_func_name = "";

    unsigned int loop_label_counter = 0;
    std::string gen_new_loop_label();
    std::stack<std::string> loop_scopes;

    void enter_loop_scope(std::string label);
    void exit_loop_scope();

    void analyse_node(ASTNode *node);

    void analyse_func(ASTNode *func);
    void analyse_var_decl(ASTNode *var_decl);
    void analyse_var_assign(ASTNode *node);
    void analyse_rtn(ASTNode *node);
    void analyse_if_stmt(ASTNode *node);
    void analyse_binary(ASTNode *node);
    void analyse_unary(ASTNode *node);
    void analyse_var(ASTNode *node);
    void analyse_while_stmt(ASTNode *node);
    void analyse_for_stmt(ASTNode *node);
    void analyse_loop_control(ASTNode *node);
    void analyse_func_call(ASTNode *node);
    void analyse_cast(ASTNode *node);
    void analyse_addr_of(ASTNode *node);
    void analyse_deref(ASTNode *node);

    Type infer_type(ASTNode *node);

    void error(const std::string &message);
};