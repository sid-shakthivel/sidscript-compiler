#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>

#include "lexer.h"

enum class UnaryOpType
{
    NEGATE,
    COMPLEMENT,
    DECREMENT,
    INCREMENT,
};

enum class BinOpType
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

enum class NodeType
{
    NODE_NUMBER,
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
    NODE_FUNC_CALL,
    NODE_CAST
};

enum class Type
{
    INT,
    LONG,
    VOID
};

enum class Specifier
{
    NONE,
    STATIC,
    EXTERN
};

UnaryOpType
get_unary_op_type(const TokenType &t);
BinOpType get_bin_op_type(const TokenType &t);
Type get_type(const TokenType &t);
Specifier get_specifier(const TokenType &t);
void print_type(Type &t);

class ASTNode
{
public:
    NodeType type;

    ASTNode(NodeType t) : type(t) {}
    virtual ~ASTNode() = default;
    virtual void print(int tabs) {};
};

class NumericLiteral : public ASTNode
{
public:
    NumericLiteral(NodeType t);
    Type value_type;

    virtual ~NumericLiteral() = default;
};

class IntegerLiteral : public NumericLiteral
{
public:
    int value;

    IntegerLiteral(int v);
    void print(int tabs) override;
};

class LongLiteral : public NumericLiteral
{
public:
    long value;

    LongLiteral(long v);
    void print(int tabs) override;
};

class CastNode : public ASTNode
{
public:
    std::unique_ptr<ASTNode> expr;
    Type target_type;

    CastNode(std::unique_ptr<ASTNode> e, Type t);
    void print(int tabs) override;
};

class RtnNode : public ASTNode
{
public:
    std::unique_ptr<ASTNode> value;

    RtnNode(std::unique_ptr<ASTNode> v);
    void print(int tabs) override;
};

class FuncNode : public ASTNode
{
public:
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> params;
    std::vector<std::unique_ptr<ASTNode>> elements;
    Type return_type;
    Specifier specifier;

    FuncNode(const std::string &n, Specifier s = Specifier::NONE);
    void print(int tabs) override;
    std::string get_param_name(int i);
};

class FuncCallNode : public ASTNode
{
public:
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> args;

    FuncCallNode(const std::string &n);
    void print(int tabs) override;
};

class ProgramNode : public ASTNode
{
public:
    std::vector<std::unique_ptr<ASTNode>> decls;

    ProgramNode();
    void print(int tabs = 0) override;
};

class UnaryNode : public ASTNode
{
public:
    UnaryOpType op;
    std::unique_ptr<ASTNode> value;
    Type type = Type::VOID;

    UnaryNode(UnaryOpType o, std::unique_ptr<ASTNode> v);
    void print(int tabs) override;
};

class BinaryNode : public ASTNode
{
public:
    BinOpType op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    Type type = Type::VOID;

    BinaryNode(BinOpType o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r);
    void print(int tabs) override;
};

class VarNode : public ASTNode
{
public:
    std::string name;
    Type type;
    Specifier specifier;

    VarNode(const std::string &n, Type t, Specifier s = Specifier::NONE);
    void print(int tabs) override;
};

class VarDeclNode : public ASTNode
{
public:
    std::unique_ptr<VarNode> var;
    std::unique_ptr<ASTNode> value;

    VarDeclNode(std::unique_ptr<VarNode> v, std::unique_ptr<ASTNode> val);
    void print(int tabs) override;
};

class VarAssignNode : public ASTNode
{
public:
    std::unique_ptr<VarNode> var;
    std::unique_ptr<ASTNode> value;

    VarAssignNode(std::unique_ptr<VarNode> v, std::unique_ptr<ASTNode> val);
    void print(int tabs) override;
};

class IfNode : public ASTNode
{
public:
    std::unique_ptr<BinaryNode> condition;
    std::vector<std::unique_ptr<ASTNode>> then_elements;
    std::vector<std::unique_ptr<ASTNode>> else_elements;

    IfNode(std::unique_ptr<BinaryNode> c, std::vector<std::unique_ptr<ASTNode>> t, std::vector<std::unique_ptr<ASTNode>> e);
    void print(int tabs) override;
};

class WhileNode : public ASTNode
{
public:
    std::unique_ptr<BinaryNode> condition;
    std::vector<std::unique_ptr<ASTNode>> elements;

    std::string label = "";

    WhileNode(std::unique_ptr<BinaryNode> c, std::vector<std::unique_ptr<ASTNode>> e);
    void print(int tabs) override;
};

class ForNode : public ASTNode
{
public:
    std::unique_ptr<ASTNode> init;
    std::unique_ptr<BinaryNode> condition;
    std::unique_ptr<ASTNode> post;
    std::vector<std::unique_ptr<ASTNode>> elements;

    std::string label = "";

    ForNode(std::unique_ptr<ASTNode> i, std::unique_ptr<BinaryNode> c, std::unique_ptr<ASTNode> p, std::vector<std::unique_ptr<ASTNode>> e);
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