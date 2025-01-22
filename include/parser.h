#pragma once

#include <vector>
#include <unordered_map>

#include "ast.h"
#include "lexer.h"

class Parser
{
public:
    Parser(Lexer *l);
    ProgramNode *parse();

private:
    Lexer *lexer;
    Token current_token;

    bool match(TokenType type);
    bool match(std::vector<TokenType> &tokens);
    void advance();
    void retreat();
    void expect(TokenType token_type);
    void expect_and_advance(TokenType token_type);
    void expect(std::vector<TokenType> &tokens);
    void error(const std::string &message);

    std::vector<ASTNode *> parse_block();
    FuncNode *parse_func_decl();
    void parse_param_list(FuncNode *func);
    void parse_args_list(FuncCallNode *func_call);
    RtnNode *parse_rtn();
    VarDeclNode *parse_var_decl(TokenType specifier = TOKEN_EOF);
    VarAssignNode *parse_var_assign();
    ASTNode *parse_factor();
    ASTNode *parse_expr(int min_precedence = 0);
    IfNode *parse_if_stmt();
    WhileNode *parse_while_stmt();
    ForNode *parse_for_stmt();
    ASTNode *parse_for_init();
    ASTNode *parse_loop_control();

    int get_precedence(TokenType op);
};