#pragma once

#include <vector>

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
    bool match(std::vector<TokenType> &end_tokens);
    void advance();
    void expect(TokenType token_type);
    void error(const std::string &message);

    std::vector<ASTNode *> parse_stmts();
    FuncNode *parse_func();
    RtnNode *parse_rtn();
    ASTNode *parse_expr(bool expect_semicolon = true);

    // IfStmt *parse_if_stmt();
    // Condition *parse_condition();
    // VarDecl *parse_var_decl();
    // VarAssign *parse_var_assign();
    // ASTNode *parse_expr(std::vector<TokenType> end_tokens);
    // ASTNode *parse_term(std::vector<TokenType> &end_tokens);
    // FactorNode *parse_factor();
};