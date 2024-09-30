#include <vector>
#include <iostream>

#include "../include/parser.h"
#include "../include/ast.h"

Parser::Parser(Lexer *l) : lexer(l), current_token(TOKEN_EOF, "")
{
    advance();
}

void Parser::parse()
{
    parse_statements();

    for (auto statement : statements)
        statement->print(0);
}

bool Parser::match(TokenType type)
{
    return current_token.type == type;
}

void Parser::advance()
{
    current_token = lexer->get_next_token();
}

void Parser::error(const std::string &message)
{
    throw std::runtime_error("Parser Error: " + message + " at token: " + current_token.text);
}

void Parser::parse_statements()
{
    if (match(TOKEN_INT_TEXT) || match(TOKEN_FLOAT_TEXT) || match(TOKEN_BOOL_TEXT))
    {
        statements.emplace_back(parse_var_decl());
    }
    else if (match(TOKEN_IDENTIFIER))
    {
        statements.emplace_back(parse_var_assign());
    }
}

// <var_decl> ::= <basic_type> <id> "=" <expr> ";"
VarDecl *Parser::parse_var_decl()
{
    TokenType var_type = current_token.type;
    advance();

    // Expect identifier
    if (!match(TOKEN_IDENTIFIER))
    {
        error("Expected a variable name");
        return nullptr;
    }
    std::string var_identifier = current_token.text;

    advance();

    // Expect equals
    if (!match(TOKEN_ASSIGN))
    {
        error("Expected '='");
        return nullptr;
    }

    advance();

    ASTNode *expr = parse_expr();

    return new VarDecl(var_identifier, expr, var_type);
}

// <var_assign> ::= <id> "=" <expr> ";"
VarAssign *Parser::parse_var_assign()
{
    std::string var_identifier = current_token.text;

    advance();

    if (!match(TOKEN_ASSIGN))
    {
        error("Expected '='");
        return nullptr;
    }

    advance();

    ASTNode *expr = parse_expr();

    return new VarAssign(var_identifier, expr);
}

// <expr> ::= <expr> ("+" | "-") <term> | <term>
ASTNode *Parser::parse_expr()
{
    // Expect a proper term
    ASTNode *left_term = parse_term();

    while (!match(TOKEN_SEMICOLON))
    {
        TokenType op = current_token.type;

        advance();

        ASTNode *right_term = parse_term();

        left_term = new BinaryExpression(left_term, right_term, op);
    }

    return left_term;
}

ASTNode *Parser::parse_term()
{
    ASTNode *left_factor = parse_factor();

    advance();

    while (!match(TOKEN_SEMICOLON))
    {
        TokenType op = current_token.type;

        advance();

        ASTNode *right_factor = parse_term();

        left_factor = new BinaryExpression(left_factor, right_factor, op);
    }

    return left_factor;
}

FactorNode *Parser::parse_factor()
{
    if (match(TOKEN_MINUS) || match(TOKEN_EXCLAMATION) || match(TOKEN_TILDA))
    {
        TokenType op = current_token.type;

        advance();

        auto literal = parse_factor()->value;
        return new UnaryExpression(op, literal);
    }

    // Return AST node
    if (match(TOKEN_INT))
    {
        int value = std::stoi(current_token.text);
        return new FactorNode(new IntegerLiteral(value));
    }
    else if (match(TOKEN_FLOAT))
    {
        float value = std::stof(current_token.text);
        return new FactorNode(new FloatLiteral(value));
    }
    else if (match(TOKEN_BOOL))
    {
        bool value = current_token.text == "true";
        return new FactorNode(new BoolLiteral(value));
    }

    error("Unknown factor");
}
