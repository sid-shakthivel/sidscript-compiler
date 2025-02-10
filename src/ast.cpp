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
    case TOKEN_INCREMENT:
        return UnaryOpType::INCREMENT;
    case TOKEN_DECREMENT:
        return UnaryOpType::DECREMENT;
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

Specifier get_specifier(const TokenType &t)
{
    switch (t)
    {
    case TOKEN_STATIC:
        return Specifier::STATIC;
    case TOKEN_EXTERN:
        return Specifier::EXTERN;
    default:
        return Specifier::NONE;
    }
}

std::string get_type_str(Type &t)
{
    switch (t)
    {
    case Type::INT:
        return "int";
    case Type::LONG:
        return "long";
    case Type::UINT:
        return "uint";
    case Type::ULONG:
        return "ulong";
    case Type::VOID:
        return "void";
    case Type::DOUBLE:
        return "double";
    default:
        return "unknown";
    }
}

Type get_type_from_str(std::string &t)
{
    if (t == "int")
        return Type::INT;
    else if (t == "long")
        return Type::LONG;
    else if (t == "uint")
        return Type::UINT;
    else if (t == "ulong")
        return Type::ULONG;
    else if (t == "void")
        return Type::VOID;
    else if (t == "double")
        return Type::DOUBLE;
}

NumericLiteral::NumericLiteral(NodeType t) : ASTNode(t) {}

IntegerLiteral::IntegerLiteral(int v) : NumericLiteral(NodeType::NODE_NUMBER), value(v)
{
    value_type = Type::INT;
}

void IntegerLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "(Int) Literal: " + std::to_string(value) << std::endl;
}

LongLiteral::LongLiteral(long v) : NumericLiteral(NodeType::NODE_NUMBER), value(v)
{
    value_type = Type::LONG;
}

void LongLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "(Long) Literal: " + std::to_string(value) << std::endl;
}

UIntegerLiteral::UIntegerLiteral(unsigned int v) : NumericLiteral(NodeType::NODE_NUMBER), value(v)
{
    value_type = Type::UINT;
}

void UIntegerLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "(UInt) Literal: " + std::to_string(value) << std::endl;
}

ULongLiteral::ULongLiteral(unsigned long v) : NumericLiteral(NodeType::NODE_NUMBER), value(v)
{
    value_type = Type::ULONG;
}

void ULongLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "(ULong) Literal: " + std::to_string(value) << std::endl;
}

DoubleLiteral::DoubleLiteral(double v) : NumericLiteral(NodeType::NODE_NUMBER), value(v)
{
    value_type = Type::DOUBLE;
}

void DoubleLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "(Double) Literal: " + std::to_string(value) << std::endl;
}

CastNode::CastNode(std::unique_ptr<ASTNode> e, Type t1, Type t2) : ASTNode(NodeType::NODE_CAST), expr(std::move(e)), target_type(t1), src_type(t2) {}

void CastNode::print(int tabs)
{
    auto target_type_str = get_type_str(target_type);
    std::cout << std::string(tabs, ' ') << "Cast: " << std::endl;
    std::cout << std::string(tabs + 1, ' ') << "Target Type: " << target_type_str << std::endl;
    expr->print(tabs + 1);
}

RtnNode::RtnNode(std::unique_ptr<ASTNode> v) : ASTNode(NodeType::NODE_RETURN), value(std::move(v)) {}

void RtnNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Rtn: " << std::endl;
    value->print(tabs + 1);
}

FuncNode::FuncNode(const std::string &n, Specifier s) : ASTNode(NodeType::NODE_FUNCTION), name(n), specifier(s) {}

void FuncNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Func: " << std::endl;
    std::cout << std::string(tabs + 1, ' ') << "Name: " << name << std::endl;

    if (specifier == Specifier::STATIC)
        std::cout << std::string(tabs + 1, ' ') << "Specifier: STATIC" << std::endl;
    else if (specifier == Specifier::EXTERN)
        std::cout << std::string(tabs + 1, ' ') << "Specifier: EXTERN" << std::endl;

    std::cout << std::string(tabs + 1, ' ') << "Params: " << std::endl;

    for (auto &param : params)
        param->print(tabs + 2);

    std::cout << std::string(tabs + 1, ' ') << "Body: " << std::endl;

    for (auto &stmt : elements)
        stmt->print(tabs + 2);
}

std::string FuncNode::get_param_name(int i)
{
    return dynamic_cast<VarDeclNode *>(params[i].get())->var->name;
}

FuncCallNode::FuncCallNode(const std::string &n) : ASTNode(NodeType::NODE_FUNC_CALL), name(n) {}

void FuncCallNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "FuncCall: " << name << std::endl;
    for (auto &arg : args)
        arg->print(tabs + 1);
}

ProgramNode::ProgramNode() : ASTNode(NodeType::NODE_PROGRAM) {}

void ProgramNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Program: " << std::endl;
    for (auto &decl : decls)
        decl->print(tabs + 1);
}

UnaryNode::UnaryNode(UnaryOpType o, std::unique_ptr<ASTNode> v) : ASTNode(NodeType::NODE_UNARY), op(o), value(std::move(v)) {}

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
        case UnaryOpType::INCREMENT:
            return "INCREMENT";
        }
        return "";
    };

    std::cout << std::string(tabs, ' ') << "Unary: " << std::endl;
    std::cout << std::string(tabs + 1, ' ') << "Type: " << get_unary_op_string(op) << std::endl;
    value->print(tabs + 1);
}

BinaryNode::BinaryNode(BinOpType o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r) : ASTNode(NodeType::NODE_BINARY), op(o), left(std::move(l)), right(std::move(r)) {}

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

VarNode::VarNode(const std::string &n, Type t, Specifier s) : ASTNode(NodeType::NODE_VAR), name(n), type(t), specifier(s) {}

void VarNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Var: " << name << std::endl;

    if (specifier == Specifier::STATIC)
        std::cout << std::string(tabs + 1, ' ') << "Specifier: STATIC" << std::endl;
    else if (specifier == Specifier::EXTERN)
        std::cout << std::string(tabs + 1, ' ') << "Specifier: EXTERN" << std::endl;
}

VarAssignNode::VarAssignNode(std::unique_ptr<VarNode> v, std::unique_ptr<ASTNode> val) : ASTNode(NodeType::NODE_VAR_ASSIGN), var(std::move(v)), value(std::move(val)) {}

void VarAssignNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "VarAssign: " << std::endl;
    var->print(tabs + 1);
    value->print(tabs + 1);
}

VarDeclNode::VarDeclNode(std::unique_ptr<VarNode> v, std::unique_ptr<ASTNode> val) : ASTNode(NodeType::NODE_VAR_DECL), var(std::move(v)), value(std::move(val)) {}

void VarDeclNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "VarDecl: " << std::endl;
    var->print(tabs + 1);
    if (value != nullptr)
        value->print(tabs + 1);
}

IfNode::IfNode(std::unique_ptr<BinaryNode> c, std::vector<std::unique_ptr<ASTNode>> t, std::vector<std::unique_ptr<ASTNode>> e) : ASTNode(NodeType::NODE_IF), condition(std::move(c)), then_elements(std::move(t)), else_elements(std::move(e)) {}

void IfNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "If: " << std::endl;
    condition->print(tabs + 1);

    std::cout << std::string(tabs + 1, ' ') << "If Stms:" << std::endl;
    for (auto &statement : then_elements)
        statement->print(tabs + 2);

    std::cout << std::string(tabs + 1, ' ') << "Else Stms:" << std::endl;
    for (auto &statement : else_elements)
        statement->print(tabs + 2);
}

WhileNode::WhileNode(std::unique_ptr<BinaryNode> c, std::vector<std::unique_ptr<ASTNode>> e) : ASTNode(NodeType::NODE_WHILE), condition(std::move(c)), elements(std::move(e)) {}

void WhileNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "While: " << std::endl;
    condition->print(tabs + 1);
    std::cout << std::string(tabs + 1, ' ') << "While Elements:" << std::endl;
    for (auto &element : elements)
        element->print(tabs + 2);
}

ForNode::ForNode(std::unique_ptr<ASTNode> i, std::unique_ptr<BinaryNode> c, std::unique_ptr<ASTNode> p, std::vector<std::unique_ptr<ASTNode>> e) : ASTNode(NodeType::NODE_FOR), init(std::move(i)), condition(std::move(c)), post(std::move(p)), elements(std::move(e)) {}

void ForNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "For: " << std::endl;
    init->print(tabs + 1);
    condition->print(tabs + 1);
    post->print(tabs + 1);
    std::cout << std::string(tabs + 1, ' ') << "For Elements:" << std::endl;
    for (auto &element : elements)
        element->print(tabs + 2);
}

ContinueNode::ContinueNode(std::string l) : ASTNode(NodeType::NODE_CONTINUE), label(l) {}

void ContinueNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Continue " << label << std::endl;
}

BreakNode::BreakNode(std::string l) : ASTNode(NodeType::NODE_BREAK), label(l) {}

void BreakNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Break " << label << std::endl;
}
