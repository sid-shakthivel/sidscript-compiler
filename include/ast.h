#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "lexer.h"

enum UnaryOpType
{
    NEGATE,
    COMPLEMENT,
    DECREMENT,
    INCREMENT,
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
    NODE_WHILE,
    NODE_FOR,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_FUNC_CALL
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
    std::vector<ASTNode *> params;
    std::vector<ASTNode *> elements;

    FuncNode(const std::string &n);
    void print(int tabs) override;
};

class FuncCallNode : public ASTNode
{
public:
    std::string name;
    std::vector<ASTNode *> args;

    FuncCallNode(const std::string &n);
    void print(int tabs) override;
};

class ProgramNode : public ASTNode
{
public:
    std::unordered_map<std::string, FuncNode *> functions;

    ProgramNode();
    void print(int tabs = 0) override;
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

class WhileNode : public ASTNode
{
public:
    BinaryNode *condition;
    std::vector<ASTNode *> elements;

    std::string label = "";

    WhileNode(BinaryNode *c, std::vector<ASTNode *> &e);
    void print(int tabs) override;
};

class ForNode : public ASTNode
{
public:
    ASTNode *init;
    BinaryNode *condition;
    ASTNode *post;
    std::vector<ASTNode *> elements;

    std::string label = "";

    ForNode(ASTNode *i, BinaryNode *c, ASTNode *p, std::vector<ASTNode *> &e);
    void print(int tabs) override;
};

class BreakNode : public ASTNode
{
public:
    std::string label;

    BreakNode(std::string l);
    void print(int tabs) override;
};

class ContinueNode : public ASTNode
{
public:
    std::string label;

    ContinueNode(std::string l);
    void print(int tabs) override;
};