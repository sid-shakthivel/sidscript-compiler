#pragma once

#include "lexer.h"

class Parser
{
public:
    Parser(Lexer *l) : lexer(l), current_token(lexer->get_next_token()) {}
    void parse();

private:
    Lexer *lexer;
    Token current_token;
    bool match(TokenType type);

    void parse_statements();
    void parse_var_decl();
    void parse_var_assign();
    void parse_expr();
    void parse_term();
    void parse_unary();
    void parse_factor();
};