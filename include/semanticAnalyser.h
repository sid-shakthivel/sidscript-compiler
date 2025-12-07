#pragma once

#include <memory>
#include <functional>
#include <map>

#include "globalSymbolTable.h"
#include "symbolTable.h"
#include "ast.h"
#include "semanticAnalyser.h"

class SemanticAnalyser
{
public:
    SemanticAnalyser(std::shared_ptr<GlobalSymbolTable> gst, std::string module_name);

    void analyse(std::shared_ptr<ProgramNode> &program);

    Type infer_type(ASTNode *node, std::optional<std::string> struct_name = std::nullopt);

private:
    std::unordered_map<NodeType, std::function<void(ASTNode *)>> handlers;
    std::shared_ptr<GlobalSymbolTable> gst;
    std::string module_name;

    unsigned int loop_label_counter = 0;
    std::string gen_new_loop_label();
    std::stack<std::string> loop_scopes;

    void enter_loop_scope(const std::string &label);
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
    void analyse_struct_decl(ASTNode *node);
    void analyse_aggregate_literal(ASTNode *node, const Type &type = Type(BaseType::VOID));
    void analyse_postfix(ASTNode *node);
    void analyse_include(ASTNode *node);

    void analyse_specifiers(const std::vector<Specifier> &specifiers, ASTNode *node);

    bool try_promote_literal(std::unique_ptr<ASTNode> &expr, const Type &target);

    void validate_type_assignment(Type &target_type, std::unique_ptr<ASTNode> &source_expr, const std::string &context);

    void error(const std::string &message, const SourceLocation &loc);
};