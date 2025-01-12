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
    void expect(TokenType token_type);
    void expect_and_advance(TokenType token_type);
    void expect(std::vector<TokenType> &tokens);
    void error(const std::string &message);

    std::vector<ASTNode *> parse_elements();
    FuncNode *parse_func();
    RtnNode *parse_rtn();
    VarDeclNode *parse_var_decl();
    ASTNode *parse_factor();
    ASTNode *parse_expr(int min_precedence);
    IfNode *parse_if_stmt();

    int get_precedence(TokenType op);
};