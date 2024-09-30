#include <iostream>
#include <iomanip>

#include "../include/ast.h"
#include "../include/lexer.h"

Literal::Literal(const TokenType &t) : type(t) {}

IntegerLiteral::IntegerLiteral(int v) : Literal(TOKEN_INT), value(v) {}

void IntegerLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Literal: " + std::to_string(value) << std::endl;
}

FloatLiteral::FloatLiteral(float v) : Literal(TOKEN_FLOAT), value(v) {}

void FloatLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Literal: " + std::to_string(value) << std::endl;
}

BoolLiteral::BoolLiteral(bool v) : Literal(TOKEN_BOOL), value(v) {}

void BoolLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Literal: " + std::to_string(value) << std::endl;
}

FactorNode::FactorNode(Literal *v) : value(v) {}

void FactorNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Factor: " << std::endl;
    value->print(tabs + 1);
}

UnaryExpression::UnaryExpression(const TokenType &t, Literal *v) : op(t), FactorNode(v) {}

void UnaryExpression::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "UnExpr(" + token_to_string(op) + "): " << std::endl;
    value->print(tabs + 1);
}

BinaryExpression::BinaryExpression(ASTNode *l, ASTNode *r, const TokenType &t) : left(l), op(t), right(r) {}

void BinaryExpression::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "BinExpr(" + token_to_string(op) + "): " << std::endl;
    left->print(tabs + 1);
    right->print(tabs + 1);
}

VarDecl::VarDecl(const std::string &name, ASTNode *val, const TokenType &t) : var_name(name), value(val), type(t) {}

void VarDecl::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "VarDecl: " << var_name << " = " << std::endl;
    value->print(tabs + 1);
}

VarAssign::VarAssign(const std::string &name, ASTNode *val) : var_name(name), value(val) {}

void VarAssign::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "VarAssign: " << var_name << " = " << std::endl;
    value->print(tabs + 1);
}
