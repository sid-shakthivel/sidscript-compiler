#pragma once

#include <string>

#include "lexer.h"

class ASTNode
{
public:
    virtual ~ASTNode() = default;
    virtual void print(int tabs)
    {
    }
};

// <basic_type> ::= "int" | "float" | "bool"
class Literal : public ASTNode
{
public:
    TokenType type;

    Literal(const TokenType &t);
};

class IntegerLiteral : public Literal
{
public:
    int value;

    IntegerLiteral(int v);
    void print(int tabs) override;
};

class FloatLiteral : public Literal
{
public:
    float value;

    FloatLiteral(float v);
    void print(int tabs) override;
};

class BoolLiteral : public Literal
{
public:
    bool value;

    BoolLiteral(bool v);
    void print(int tabs) override;
};

// <factor> ::= <un_opr> <factor> | (int | float | bool | <func_call>)
class FactorNode : public ASTNode
{
public:
    Literal *value;

    FactorNode(Literal *v);
    void print(int tabs) override;
};

// <factor> ::= <un_opr> <factor> | (int | float | bool | <func_call>)
class UnaryExpression : public FactorNode
{
public:
    TokenType op; // "-" | "!" | "~"

    UnaryExpression(const TokenType &t, Literal *v);
    void print(int tabs) override;
};

// <expr> ::= <expr> ("+" | "-") <term> | <term>
class BinaryExpression : public ASTNode
{
public:
    ASTNode *left;
    ASTNode *right;
    TokenType op; // "+" | "-"

    BinaryExpression(ASTNode *l, ASTNode *r, const TokenType &t);
    void print(int tabs) override;
};

// ie int test = 5;
class VarDecl : public ASTNode
{
public:
    std::string var_name;
    TokenType type;
    ASTNode *value;

    VarDecl(const std::string &name, ASTNode *val, const TokenType &t);
    void print(int tabs) override;
};

// ie test = 10;
class VarAssign : public ASTNode
{
public:
    std::string var_name;
    ASTNode *value;

    VarAssign(const std::string &name, ASTNode *val);
    void print(int tabs) override;
};

// std::unique_ptr<ASTNode>