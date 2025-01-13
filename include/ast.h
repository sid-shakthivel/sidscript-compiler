#pragma once

#include <string>
#include <vector>

#include "lexer.h"

enum UnaryOpType
{
    NEGATE,
    COMPLEMENT,
    DECREMENT,
};

enum BinOpType
{
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    AND,
    OR,
    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    GREATER_THAN,
    LESS_OR_EQUAL,
    GREATER_OR_EQUAL
};

enum NodeType
{
    NODE_INTEGER,
    NODE_RETURN,
    NODE_FUNCTION,
    NODE_PROGRAM,
    NODE_UNARY,
    NODE_BINARY,
    NODE_VAR,
    NODE_VAR_ASSIGN,
    NODE_VAR_DECL,
    NODE_IF,
};

UnaryOpType get_unary_op_type(const TokenType &t);
BinOpType get_bin_op_type(const TokenType &t);

class ASTNode
{
public:
    NodeType type;

    ASTNode(NodeType t) : type(t) {}
    virtual ~ASTNode() = default;
    virtual void print(int tabs)
    {
    }
};

class IntegerLiteral : public ASTNode
{
public:
    int value;

    IntegerLiteral(int v);
    void print(int tabs) override;
};

class RtnNode : public ASTNode
{
public:
    ASTNode *value;

    RtnNode(ASTNode *v);
    void print(int tabs) override;
};

class FuncNode : public ASTNode
{
public:
    std::string name;
    std::vector<ASTNode *> args;
    std::vector<ASTNode *> elements;

    FuncNode(const std::string &n, std::vector<ASTNode *> &s);
    void print(int tabs) override;
};

class ProgramNode : public ASTNode
{
public:
    FuncNode *func;

    ProgramNode(FuncNode *f);
    void print(int tabs) override;
};

class UnaryNode : public ASTNode
{
public:
    UnaryOpType op;
    ASTNode *value;

    UnaryNode(UnaryOpType o, ASTNode *v);
    void print(int tabs) override;
};

class BinaryNode : public ASTNode
{
public:
    BinOpType op;
    ASTNode *left;
    ASTNode *right;

    BinaryNode(BinOpType o, ASTNode *l, ASTNode *r);
    void print(int tabs) override;
};

class VarNode : public ASTNode
{
public:
    std::string name;

    VarNode(const std::string &n);
    void print(int tabs) override;
};

class VarDeclNode : public ASTNode
{
public:
    VarNode *var;
    ASTNode *value;

    VarDeclNode(VarNode *v, ASTNode *val);
    void print(int tabs) override;
};

class VarAssignNode : public ASTNode
{
public:
    VarNode *var;
    ASTNode *value;

    VarAssignNode(VarNode *v, ASTNode *val);
    void print(int tabs) override;
};

class IfNode : public ASTNode
{
public:
    BinaryNode *condition;
    std::vector<ASTNode *> then_elements;
    std::vector<ASTNode *> else_elements;

    IfNode(BinaryNode *c, std::vector<ASTNode *> &t, std::vector<ASTNode *> &e);
    void print(int tabs) override;
};