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

enum NodeType
{
    NODE_INTEGER,
    NODE_RETURN,
    NODE_FUNCTION,
    NODE_PROGRAM,
    NODE_UNARY,
};

UnaryOpType
get_unary_op_type(const TokenType &t);

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
    std::vector<ASTNode *> stmts;

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

// // <basic_type> ::= "int" | "float" | "bool"
// class Literal : public ASTNode
// {
// public:
//     TokenType type;

//     Literal(const TokenType &t);
// };

// class FloatLiteral : public Literal
// {
// public:
//     float value;

//     FloatLiteral(float v);
//     void print(int tabs) override;
// };

// class BoolLiteral : public Literal
// {
// public:
//     bool value;

//     BoolLiteral(bool v);
//     void print(int tabs) override;
// };

// // <factor> ::= <un_opr> <factor> | (int | float | bool | <func_call>)
// class FactorNode : public ASTNode
// {
// public:
//     Literal *value;

//     FactorNode(Literal *v);
//     void print(int tabs) override;
// };

// // <factor> ::= <un_opr> <factor> | (int | float | bool | <func_call>)
// class UnaryExpression : public FactorNode
// {
// public:
//     TokenType op; // "-" | "!" | "~"

//     UnaryExpression(const TokenType &t, Literal *v);
//     void print(int tabs) override;
// };

// // <expr> ::= <expr> ("+" | "-") <term> | <term>
// class BinaryExpression : public ASTNode
// {
// public:
//     ASTNode *left;
//     ASTNode *right;
//     TokenType op; // "+" | "-"

//     BinaryExpression(ASTNode *l, ASTNode *r, const TokenType &t);
//     void print(int tabs) override;
// };

// // ie int test = 5;
// class VarDecl : public ASTNode
// {
// public:
//     std::string var_name;
//     TokenType type;
//     ASTNode *value;

//     VarDecl(const std::string &name, ASTNode *val, const TokenType &t);
//     void print(int tabs) override;
// };

// // ie test = 10;
// class VarAssign : public ASTNode
// {
// public:
//     std::string var_name;
//     ASTNode *value;

//     VarAssign(const std::string &name, ASTNode *val);
//     void print(int tabs) override;
// };

// class Condition : public ASTNode
// {
// public:
//     TokenType bin_top;
//     ASTNode *left;
//     ASTNode *right;

//     Condition(const TokenType &t, ASTNode *l, ASTNode *r);
//     void print(int tabs) override;
// };

// class IfStmt : public ASTNode
// {
// public:
//     Condition *condition;
//     std::vector<ASTNode *> if_statements;
//     std::vector<ASTNode *> else_statements;

//     IfStmt(Condition *c);
//     void print(int tabs) override;
// };

// std::unique_ptr<ASTNode>