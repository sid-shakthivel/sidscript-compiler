#include <iostream>
#include <iomanip>

#include "../include/ast.h"
#include "../include/lexer.h"

Type::Type(BaseType base) : base_type(base) {}

Type::Type(BaseType base, int ptr_level) : base_type(base), pointer_level(ptr_level) {}

Type::Type(std::string struct_name) : base_type(BaseType::STRUCT), struct_name(struct_name) {}

Type &Type::add_array_dimension(int size)
{
    array_sizes.push_back(size);
    return *this;
}

bool Type::is_pointer() const { return pointer_level > 0; }
bool Type::is_array() const { return array_sizes.size() > 0; }
bool Type::is_struct() const { return base_type == BaseType::STRUCT; }

BaseType Type::get_base_type() const { return base_type; }

size_t Type::get_size() const
{
    size_t base_size;

    if (is_pointer())
        return 8;

    switch (base_type)
    {
    case BaseType::INT:
    case BaseType::UINT:
        base_size = 4;
        break;
    case BaseType::LONG:
    case BaseType::ULONG:
    case BaseType::DOUBLE:
        base_size = 8;
        break;
    case BaseType::VOID:
        base_size = 0;
        break;
    case BaseType::STRUCT:
        base_size = 0;
        break;
    }

    if (is_array())
    {
        size_t total_size = base_size;
        for (int size : array_sizes)
            total_size *= size;
        return total_size;
    }

    return base_size;
}

size_t Type::get_array_size() const
{
    if (!is_array())
        return 0;

    size_t total_size = 1;

    for (int size : array_sizes)
        total_size *= size;

    return total_size;
}

bool Type::is_size_8() const
{
    return get_size() == 8;
}

std::string Type::to_string() const
{
    std::string result;

    // Add base type
    switch (base_type)
    {
    case BaseType::INT:
        result = "int";
        break;
    case BaseType::LONG:
        result = "long";
        break;
    case BaseType::UINT:
        result = "unsigned int";
        break;
    case BaseType::ULONG:
        result = "unsigned long";
        break;
    case BaseType::DOUBLE:
        result = "double";
        break;
    case BaseType::VOID:
        result = "void";
        break;
    case BaseType::STRUCT:
        result = "struct " + (struct_name.has_value() ? struct_name.value() : "unknown");
        break;
    }

    for (int i = 0; i < pointer_level; i++)
        result += "*";

    for (int size : array_sizes)
        result += "[" + std::to_string(size) + "]";

    return result;
}

bool Type::is_signed() const
{
    if (is_pointer() || is_array())
        return false;

    if (base_type == BaseType::INT || base_type == BaseType::LONG)
        return true;

    return false;
}

bool Type::operator==(const Type &other) const
{
    if (base_type != other.base_type)
        return false;

    if (pointer_level != other.pointer_level)
        return false;

    if (array_sizes != other.array_sizes)
        return false;

    if (base_type == BaseType::STRUCT)
    {
        if (!struct_name.has_value() || !other.struct_name.has_value())
            return false;
        return struct_name.value() == other.struct_name.value();
    }

    return true;
}

bool Type::operator!=(const Type &other) const
{
    return !(*this == other);
}

bool Type::can_assign_from(const Type &other) const
{
    if (*this == other)
        return true;

    if (is_pointer())
    {
        if (other.is_pointer())
        {
            // Allow void* to be assigned to any pointer
            if (other.base_type == BaseType::VOID)
                return true;
        }
        // Allow null (integer 0) to be assigned to pointers
        if (other.base_type == BaseType::INT && !other.is_pointer())
            return true;
        return false;
    }

    if (!is_pointer() && !other.is_pointer())
    {
        // Allow smaller integers to larger ones
        if (base_type == BaseType::LONG && other.base_type == BaseType::INT)
            return true;

        // Allow integers to double
        if (base_type == BaseType::DOUBLE &&
            (other.base_type == BaseType::INT || other.base_type == BaseType::LONG))
            return true;
    }
}

bool Type::can_convert_to(const Type &other) const
{
    if (can_assign_from(other))
        return true;

    if (!is_pointer() && !other.is_pointer() && !is_array() && !other.is_array())
    {
        if ((base_type != BaseType::VOID && base_type != BaseType::STRUCT) &&
            (other.base_type != BaseType::VOID && other.base_type != BaseType::STRUCT))
            return true;
    }

    return false;
}

bool Type::has_base_type(BaseType other) const
{
    return base_type == other;
}

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
        throw std::runtime_error("Parser Error: Invalid unary operator which is " + std::to_string(t));
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
    case TOKEN_PLUS_EQUALS:
        return BinOpType::ADD;
    case TOKEN_MINUS_EQUALS:
        return BinOpType::SUB;
    case TOKEN_STAR_EQUALS:
        return BinOpType::MUL;
    case TOKEN_SLASH_EQUALS:
        return BinOpType::DIV;
    case TOKEN_MODULUS_EQUALS:
        return BinOpType::MOD;
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

Type get_type_from_str(std::string &t)
{
    if (t == "int")
        return Type(BaseType::INT);
    else if (t == "long")
        return Type(BaseType::LONG);
    else if (t == "uint")
        return Type(BaseType::UINT);
    else if (t == "ulong")
        return Type(BaseType::ULONG);
    else if (t == "void")
        return Type(BaseType::VOID);
    else if (t == "double")
        return Type(BaseType::DOUBLE);
}

NumericLiteral::NumericLiteral(NodeType t) : ASTNode(t) {}

IntegerLiteral::IntegerLiteral(int v) : NumericLiteral(NodeType::NODE_NUMBER), value(v)
{
    value_type = Type(BaseType::INT);
}

void IntegerLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "(Int) Literal: " + std::to_string(value) << std::endl;
}

LongLiteral::LongLiteral(long v) : NumericLiteral(NodeType::NODE_NUMBER), value(v)
{
    value_type = Type(BaseType::LONG);
}

void LongLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "(Long) Literal: " + std::to_string(value) << std::endl;
}

UIntegerLiteral::UIntegerLiteral(unsigned int v) : NumericLiteral(NodeType::NODE_NUMBER), value(v)
{
    value_type = Type(BaseType::UINT);
}

void UIntegerLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "(UInt) Literal: " + std::to_string(value) << std::endl;
}

ULongLiteral::ULongLiteral(unsigned long v) : NumericLiteral(NodeType::NODE_NUMBER), value(v)
{
    value_type = Type(BaseType::UINT);
}

void ULongLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "(ULong) Literal: " + std::to_string(value) << std::endl;
}

DoubleLiteral::DoubleLiteral(double v) : NumericLiteral(NodeType::NODE_NUMBER), value(v)
{
    value_type = Type(BaseType::DOUBLE);
}

void DoubleLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "(Double) Literal: " + std::to_string(value) << std::endl;
}

ArrayLiteral::ArrayLiteral(Type t) : ASTNode(NodeType::NODE_ARRAY_INIT), arr_type(t) {}

void ArrayLiteral::add_element(std::unique_ptr<ASTNode> e)
{
    values.push_back(std::move(e));
}

void ArrayLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "ArrayInit:" << std::endl;
    for (const auto &elem : values)
        elem->print(tabs + 1);
}

CharLiteral::CharLiteral(char v) : ASTNode(NodeType::NODE_CHAR), value(v) {}

void CharLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Char: " << value << std::endl;
}

StringLiteral::StringLiteral(const std::string &v) : ASTNode(NodeType::NODE_STRING), value(v) {}

void StringLiteral::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "String: " << value << std::endl;
}

CastNode::CastNode(std::unique_ptr<ASTNode> e, Type t1, Type t2) : ASTNode(NodeType::NODE_CAST), expr(std::move(e)), target_type(t1), src_type(t2) {}

void CastNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Cast: " << std::endl;
    std::cout << std::string(tabs + 1, ' ') << "Target Type: " << target_type.to_string() << std::endl;
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

PostfixNode::PostfixNode(TokenType o, std::unique_ptr<ASTNode> v) : ASTNode(NodeType::NODE_POSTFIX), op(o), value(std::move(v)) {}

void PostfixNode::print(int tabs)
{
    auto get_postfix_op_string = [](TokenType o) -> std::string
    {
        switch (o)
        {
        case TOKEN_INCREMENT:
            return "INCREMENT";
        case TOKEN_DECREMENT:
            return "DECREMENT";
        }
        return "Unknown";
    };

    std::cout << std::string(tabs, ' ') << "Postfix: " << std::endl;
    std::cout << std::string(tabs + 1, ' ') << "Type: " << get_postfix_op_string(op) << std::endl;
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

VarNode::VarNode(const std::string &n) : ASTNode(NodeType::NODE_VAR), name(n)
{
}

VarNode::VarNode(const std::string &n, Type t, Specifier s) : ASTNode(NodeType::NODE_VAR), name(n), type(t), specifier(s)
{
}

void VarNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "Var: " << name << std::endl;

    if (specifier == Specifier::STATIC)
        std::cout << std::string(tabs + 1, ' ') << "Specifier: STATIC" << std::endl;
    else if (specifier == Specifier::EXTERN)
        std::cout << std::string(tabs + 1, ' ') << "Specifier: EXTERN" << std::endl;
}

VarAssignNode::VarAssignNode(std::unique_ptr<ASTNode> v, std::unique_ptr<ASTNode> val) : ASTNode(NodeType::NODE_VAR_ASSIGN), var(std::move(v)), value(std::move(val)) {}

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

DerefNode::DerefNode(std::unique_ptr<ASTNode> v) : ASTNode(NodeType::NODE_DEREF), expr(std::move(v)) {}

void DerefNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "PointerDeref: " << std::endl;
    expr->print(tabs + 1);
}

AddrOfNode::AddrOfNode(std::unique_ptr<ASTNode> v) : ASTNode(NodeType::NODE_ADDR_OF), expr(std::move(v)) {}

void AddrOfNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "AddrOf: " << std::endl;
    expr->print(tabs + 1);
}

ArrayAccessNode::ArrayAccessNode(std::unique_ptr<VarNode> arr, std::unique_ptr<ASTNode> idx) : ASTNode(NodeType::NODE_ARRAY_ACCESS), array(std::move(arr)), index(std::move(idx)) {}

void ArrayAccessNode::print(int tabs)
{
    std::cout << std::string(tabs, ' ') << "ArrayAccess: " << std::endl;
    std::cout << std::string(tabs + 1, ' ') << "Type: " << type.to_string() << std::endl;
    array->print(tabs + 1);
    index->print(tabs + 1);
}