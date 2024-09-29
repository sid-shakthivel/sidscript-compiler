#pragma once

#include <string>

#include "lexer.h"

class ASTNode
{
public:
    virtual ~ASTNode() = default;
};

// <basic_type> ::= "int" | "float" | "bool"
class Literal : public ASTNode
{
public:
    TokenType type;

    Literal(const TokenType &t)
        : type(t) {}
};

class IntegerLiteral : public Literal
{
public:
    int value;

    IntegerLiteral(int v)
        : Literal(TOKEN_INT), value(v) {}
};

class FloatLiteral : public Literal
{
public:
    float value;

    FloatLiteral(float v)
        : Literal(TOKEN_FLOAT), value(v) {}
};

class BoolLiteral : public Literal
{
public:
    bool value;

    BoolLiteral(bool v)
        : Literal(TOKEN_BOOL), value(v) {}
};

// <factor> ::= <un_opr> <factor> | (int | float | bool | <func_call>)
class FactorNode : public ASTNode
{
public:
    Literal *value;

    FactorNode(Literal *v)
        : value(v) {}
};

// <factor> ::= <un_opr> <factor> | (int | float | bool | <func_call>)
class UnaryExpression : public FactorNode
{
public:
    TokenType op; // "-" | "!" | "~"

    UnaryExpression(const TokenType &t, Literal *v)
        : op(t), FactorNode(v) {}
};

// <term> ::= <term> ("*" | "/") <factor> | <factor>
class TermNode : public ASTNode
{
public:
    FactorNode *left;
    FactorNode *right;
    TokenType op; // "*" | "/"

    TermNode(FactorNode *l, FactorNode *r, const TokenType &t)
        : left(l), op(t), right(r) {}
};

// <expr> ::= <expr> ("+" | "-") <term> | <term>
class BinaryExpression : public ASTNode
{
public:
    TermNode *left;
    TermNode *right;
    TokenType op; // "+" | "-"

    BinaryExpression(TermNode *l, TermNode *r, const TokenType &t)
        : left(l), op(t), right(r) {}
};

// std::unique_ptr<ASTNode>