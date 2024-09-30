#pragma once

#include <vector>

#include "ast.h"
#include "lexer.h"

class Parser
{
public:
    Parser(Lexer *l);
    void parse();

private:
    Lexer *lexer;
    Token current_token;
    std::vector<ASTNode *> statements;

    bool match(TokenType type);
    void advance();
    void error(const std::string &message);

    void parse_statements();
    VarDecl *parse_var_decl();
    VarAssign *parse_var_assign();
    ASTNode *parse_expr();
    ASTNode *parse_term();
    FactorNode *parse_factor();
};