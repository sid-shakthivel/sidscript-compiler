#include <iostream>
#include <iomanip>

#include "../include/ast.h"
#include "../include/lexer.h"

UnaryOpType get_unary_op_type(const TokenType &t)
{
    switch (t)
    {
    case TOKEN_MINUS:
        return UnaryOpType::NEGATE;
    case TOKEN_TILDA:
        return UnaryOpType::COMPLEMENT;
    default:
        return UnaryOpType::DECREMENT;
    }
}

BinOpType get_bin_op_type(const TokenType &t)
{
    switch (t)
    {
    case TOKEN_PLUS:
        return BinOpType::ADD;
    case TOKEN_MINUS:
        return BinOpType::SUB;
    case TOKEN_STAR:
        return BinOpType::MUL;
    case TOKEN_SLASH:
        return BinOpType::DIV;
    case TOKEN_PERCENT:
        return BinOpType::MOD;
    case TOKEN_AND:
        return BinOpType::AND;
    case TOKEN_OR:
        return BinOpType::OR;
    case TOKEN_EQUALS:
        return BinOpType::EQUAL;
    case TOKEN_NOT_EQUALS:
        return BinOpType::NOT_EQUAL;
    case TOKEN_LT:
        return BinOpType::LESS_THAN;
    case TOKEN_GT:
        return BinOpType::GREATER_THAN;
    case TOKEN_LE:
        return BinOpType::LESS_OR_EQUAL;
    case TOKEN_GE:
        return BinOpType::GREATER_OR_EQUAL;
    default:
        return BinOpType::ADD;
    }
}

IntegerLiteral::IntegerLiteral(int v) : ASTNode(NODE_INTEGER), value(v) {}

void IntegerLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Literal: " + std::to_string(value) << std::endl;
}

RtnNode::RtnNode(ASTNode *v) : ASTNode(NODE_RETURN), value(v) {}

void RtnNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Rtn: " << std::endl;
    value->print(tabs + 1);
}

FuncNode::FuncNode(const std::string &n, std::vector<ASTNode *> &s) : ASTNode(NODE_FUNCTION), name(n), stmts(s) {}

void FuncNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Func: " << std::endl;
    std::cout << std::string(tabs + 1, ' ') << "Name: " << name << std::endl;
    std::cout << std::string(tabs + 1, ' ') << "Body: " << std::endl;

    for (auto stmt : stmts)
    {
        stmt->print(tabs + 2);
    }
}

ProgramNode::ProgramNode(FuncNode *f) : ASTNode(NODE_PROGRAM), func(f) {}

void ProgramNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Program: " << std::endl;
    func->print(tabs + 1);
}

UnaryNode::UnaryNode(UnaryOpType o, ASTNode *v) : ASTNode(NODE_UNARY), op(o), value(v) {}

void UnaryNode::print(int tabs)
{
    auto get_unary_op_string = [](UnaryOpType o) -> std::string
    {
        switch (o)
        {
        case UnaryOpType::NEGATE:
            return "NEGATE";
        case UnaryOpType::COMPLEMENT:
            return "COMPLEMENT";
        case UnaryOpType::DECREMENT:
            return "DECREMENT";
        }
        return "";
    };

    std::cout << std::string(tabs, ' ') << "Unary: " << std::endl;
    std::cout << std::string(tabs + 1, ' ') << "Type: " << get_unary_op_string(op) << std::endl;
    value->print(tabs + 1);
}

BinaryNode::BinaryNode(BinOpType o, ASTNode *l, ASTNode *r) : ASTNode(NODE_BINARY), op(o), left(l), right(r) {}

void BinaryNode::print(int tabs)
{
    auto get_binary_op_string = [](BinOpType o) -> std::string
    {
        switch (o)
        {
        case BinOpType::ADD:
            return "ADD";
        case BinOpType::SUB:
            return "SUB";
        case BinOpType::MUL:
            return "MUL";
        case BinOpType::DIV:
            return "DIV";
        case BinOpType::MOD:
            return "MOD";
        case BinOpType::AND:
            return "AND";
        case BinOpType::OR:
            return "OR";
        case BinOpType::EQUAL:
            return "EQUAL";
        case BinOpType::NOT_EQUAL:
            return "NOT_EQUAL";
        case BinOpType::LESS_THAN:
            return "LESS_THAN";
        case BinOpType::GREATER_THAN:
            return "GREATER_THAN";
        case BinOpType::LESS_OR_EQUAL:
            return "LESS_OR_EQUAL";
        case BinOpType::GREATER_OR_EQUAL:
            return "GREATER_OR_EQUAL";
        }
        return "";
    };

    std::cout << std::string(tabs, ' ') << "Binary: " << std::endl;
    std::cout << std::string(tabs + 1, ' ') << "Type: " << get_binary_op_string(op) << std::endl;
    left->print(tabs + 1);
    right->print(tabs + 1);
}

// FloatLiteral::FloatLiteral(float v) : Literal(TOKEN_FLOAT), value(v) {}

// void FloatLiteral::print(int tabs)
// {
//     std::cout << std::string(tabs, ' ') << "Literal: " + std::to_string(value) << std::endl;
// }

// BoolLiteral::BoolLiteral(bool v) : Literal(TOKEN_BOOL), value(v) {}

// void BoolLiteral::print(int tabs)
// {
//     std::cout << std::string(tabs, ' ') << "Literal: " + std::to_string(value) << std::endl;
// }

// FactorNode::FactorNode(Literal *v) : value(v) {}

// void FactorNode::print(int tabs)
// {
//     std::cout << std::string(tabs, ' ') << "Factor: " << std::endl;
//     value->print(tabs + 1);
// }

// UnaryExpression::UnaryExpression(const TokenType &t, Literal *v) : op(t), FactorNode(v) {}

// void UnaryExpression::print(int tabs)
// {
//     std::cout << std::string(tabs, ' ') << "UnExpr(" + token_to_string(op) + "): " << std::endl;
//     value->print(tabs + 1);
// }

// BinaryExpression::BinaryExpression(ASTNode *l, ASTNode *r, const TokenType &t) : left(l), op(t), right(r) {}

// void BinaryExpression::print(int tabs)
// {
//     std::cout << std::string(tabs, ' ') << "BinExpr(" + token_to_string(op) + "): " << std::endl;
//     left->print(tabs + 1);
//     right->print(tabs + 1);
// }

// VarDecl::VarDecl(const std::string &name, ASTNode *val, const TokenType &t) : var_name(name), value(val), type(t) {}

// void VarDecl::print(int tabs)
// {
//     std::cout << std::string(tabs, ' ') << "VarDecl: " << var_name << " = " << std::endl;
//     value->print(tabs + 1);
// }

// VarAssign::VarAssign(const std::string &name, ASTNode *val) : var_name(name), value(val) {}

// void VarAssign::print(int tabs)
// {
//     std::cout << std::string(tabs, ' ') << "VarAssign: " << var_name << " = " << std::endl;
//     value->print(tabs + 1);
// }

// Condition::Condition(const TokenType &t, ASTNode *l, ASTNode *r) : bin_top(t), left(l), right(r) {}

// void Condition::print(int tabs)
// {
//     std::cout << std::string(tabs, ' ') << "Condition: " << token_to_string(bin_top) << std::endl;
//     left->print(tabs + 1);
//     right->print(tabs + 1);
// }

// IfStmt::IfStmt(Condition *c) : condition(c) {}

// void IfStmt::print(int tabs)
// {
//     std::cout << std::string(tabs, ' ') << "IfStmt: " << std::endl;
//     condition->print(tabs + 1);

//     std::cout << std::string(tabs + 1, ' ') << "If Stms:" << std::endl;
//     for (auto statement : if_statements)
//         statement->print(tabs + 1);

//     std::cout << std::string(tabs + 1, ' ') << "Else Stms:" << std::endl;
//     for (auto statement : else_statements)
//         statement->print(tabs + 1);
// }