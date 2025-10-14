#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <map>

#include "lexer.h"
#include "type.h"

enum class UnaryOpType
{
    NEGATE,     // Arithmetic negation (flips sign of number)
    COMPLEMENT, // Bitwise complement (flips all bits of an integer)
    NOT,        // Logical not (!x)
    DEREF,      // Accesses value stored at memory address pointed to
    ADDR_OF     // Retrieves address of variable
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
    NODE_BOOL,
    NODE_AGGREGATE_INIT,
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
    NODE_FUNC_CALL,
    NODE_CAST,
    NODE_POSTFIX,
    NODE_ARRAY_ACCESS,
    NODE_CHAR,
    NODE_STRING,
    NODE_NULL,
    NODE_STRUCT_DECL,
    NODE_LOOP_CONTROL,
    NODE_SIZE_OF,
    NODE_INCLUDE
};

inline std::string node_type_to_string(NodeType type)
{
    switch (type)
    {
    case NodeType::NODE_NUMBER:
        return "NUMBER";
    case NodeType::NODE_AGGREGATE_INIT:
        return "COMPOUND_INIT";
    case NodeType::NODE_RETURN:
        return "RETURN";
    case NodeType::NODE_FUNCTION:
        return "FUNCTION";
    case NodeType::NODE_PROGRAM:
        return "PROGRAM";
    case NodeType::NODE_UNARY:
        return "UNARY";
    case NodeType::NODE_BINARY:
        return "BINARY";
    case NodeType::NODE_VAR:
        return "VAR";
    case NodeType::NODE_VAR_ASSIGN:
        return "VAR_ASSIGN";
    case NodeType::NODE_VAR_DECL:
        return "VAR_DECL";
    case NodeType::NODE_IF:
        return "IF";
    case NodeType::NODE_WHILE:
        return "WHILE";
    case NodeType::NODE_FOR:
        return "FOR";
    case NodeType::NODE_FUNC_CALL:
        return "FUNC_CALL";
    case NodeType::NODE_CAST:
        return "CAST";
    case NodeType::NODE_POSTFIX:
        return "POSTFIX";
    case NodeType::NODE_ARRAY_ACCESS:
        return "ARRAY_ACCESS";
    case NodeType::NODE_CHAR:
        return "CHAR";
    case NodeType::NODE_STRING:
        return "STRING";
    case NodeType::NODE_STRUCT_DECL:
        return "STRUCT_DECL";
    case NodeType::NODE_LOOP_CONTROL:
        return "LOOP_CONTROL";
    case NodeType::NODE_BOOL:
        return "BOOL";
    case NodeType::NODE_SIZE_OF:
        return "SIZE_OF";
    case NodeType::NODE_NULL:
        return "NULL";
    case NodeType::NODE_INCLUDE:
        return "INCLUDE";
    default:
        return "UNKNOWN";
    }
}

enum class Specifier
{
    NONE,
    STATIC,
    EXTERN,
    CONST,
    PUBLIC,
    PRIVATE
};

UnaryOpType
get_unary_op_type(const TokenType &t);
BinOpType get_bin_op_type(const TokenType &t);
std::vector<Specifier> parse_tokens_to_specifiers(const std::vector<TokenType> &tokens);
std::string get_str_from_specifiers(const std::vector<Specifier> &specifiers);
bool contains_specifier(const std::vector<Specifier> &specifiers, Specifier s);
Type get_type_from_str(const std::string &t);

struct SourceLocation
{
    size_t line;
    size_t index; // optional: useful for character offset
    // std::string file; // useful for multiline file input (when we support that)
};

class ASTNode
{
public:
    NodeType node_type;
    SourceLocation loc;

    ASTNode(NodeType t, SourceLocation l = {}) : node_type(t), loc(l) {}
    virtual void print(int tabs) {};

    virtual ASTNode *clone() const
    {
        return nullptr; // Base implementation
    }

    virtual ~ASTNode() = default;
};

class NumericLiteral : public ASTNode
{
public:
    NumericLiteral(NodeType t, SourceLocation loc);
    Type value_type = Type(BaseType::VOID);

    virtual ~NumericLiteral() = default;
};

class IntegerLiteral : public NumericLiteral
{
public:
    int value;

    IntegerLiteral(int v, SourceLocation loc);
    void print(int tabs) override;
};

class LongLiteral : public NumericLiteral
{
public:
    long value;

    LongLiteral(long v, SourceLocation loc);
    void print(int tabs) override;
};

class UIntegerLiteral : public NumericLiteral
{
public:
    unsigned int value;

    UIntegerLiteral(unsigned int v, SourceLocation loc);
    void print(int tabs) override;
};

class ULongLiteral : public NumericLiteral
{
public:
    unsigned long value;

    ULongLiteral(unsigned long v, SourceLocation loc);
    void print(int tabs) override;
};

class DoubleLiteral : public NumericLiteral
{
public:
    double value;

    DoubleLiteral(double v, SourceLocation loc);
    void print(int tabs) override;
};

class AggregateLiteral : public ASTNode
{
public:
    std::vector<std::unique_ptr<ASTNode>> values;
    Type type = Type(BaseType::VOID);

    void add_element(std::unique_ptr<ASTNode> element);

    AggregateLiteral(Type t, SourceLocation loc);
    void print(int tabs) override;
};

class CharLiteral : public ASTNode
{
public:
    char value;
    Type value_type = Type(BaseType::CHAR);

    CharLiteral(char v, Type t, SourceLocation loc);
    void print(int tabs) override;
};

class StringLiteral : public ASTNode
{
public:
    std::string value;
    Type value_type = Type(BaseType::VOID);

    StringLiteral(const std::string &v, Type t, SourceLocation loc);
    void print(int tabs) override;
};

class BoolLiteral : public ASTNode
{
public:
    bool value;
    Type value_type = Type(BaseType::BOOL);

    BoolLiteral(bool v, SourceLocation loc);
    void print(int tabs) override;
};

class NullLiteral : public ASTNode
{
public:
    Type value_type = Type(BaseType::NULL_TYPE);

    NullLiteral(SourceLocation loc);
    void print(int tabs) override;
};

class CastNode : public ASTNode
{
public:
    std::unique_ptr<ASTNode> expr;
    Type target_type;
    Type src_type;

    CastNode(std::unique_ptr<ASTNode> e, Type t1, Type t2 = Type(BaseType::VOID), SourceLocation loc = {});
    void print(int tabs) override;
};

class RtnNode : public ASTNode
{
public:
    std::unique_ptr<ASTNode> value;

    RtnNode(std::unique_ptr<ASTNode> v, SourceLocation loc);
    void print(int tabs) override;
};

class FuncNode : public ASTNode
{
public:
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> params;
    std::vector<std::unique_ptr<ASTNode>> elements;
    Type return_type = Type(BaseType::VOID);
    std::vector<Specifier> specifiers;

    FuncNode(const std::string &n, std::vector<Specifier> s, SourceLocation loc = {});
    void print(int tabs) override;
    std::string get_param_name(int i);
};

class FuncCallNode : public ASTNode
{
public:
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> args;

    FuncCallNode(const std::string &n, SourceLocation loc);
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
    Type type = Type(BaseType::VOID);

    UnaryNode(UnaryOpType o, std::unique_ptr<ASTNode> v, SourceLocation loc);
    void print(int tabs) override;
};

class PostfixNode : public ASTNode
{
public:
    TokenType op;
    std::unique_ptr<ASTNode> value;
    Type type = Type(BaseType::VOID);

    std::string struct_name;
    std::string field_name;

    PostfixNode(TokenType o, std::unique_ptr<ASTNode> v, SourceLocation loc);
    void print(int tabs) override;
};

class BinaryNode : public ASTNode
{
public:
    BinOpType op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    Type type = Type(BaseType::VOID);

    BinaryNode(BinOpType o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r, SourceLocation loc);
    void print(int tabs) override;
};

class VarNode : public ASTNode
{
public:
    std::string name;
    Type type = Type(BaseType::VOID);
    std::vector<Specifier> specifiers;

    VarNode(const std::string &n, Type t, std::vector<Specifier> s, SourceLocation loc = {});
    VarNode(const std::string &n, SourceLocation loc);
    void print(int tabs) override;
};

class VarDeclNode : public ASTNode
{
public:
    std::unique_ptr<VarNode> var;
    std::unique_ptr<ASTNode> value;

    VarDeclNode(std::unique_ptr<VarNode> v, std::unique_ptr<ASTNode> val, SourceLocation loc);
    void print(int tabs) override;
};

class StructDeclNode : public ASTNode
{
public:
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> members;

    StructDeclNode(const std::string &n, std::vector<std::unique_ptr<ASTNode>> m, SourceLocation loc);
    void print(int tabs) override;
};

class VarAssignNode : public ASTNode
{
public:
    std::unique_ptr<ASTNode> var;
    std::unique_ptr<ASTNode> value;

    VarAssignNode(std::unique_ptr<ASTNode> v, std::unique_ptr<ASTNode> val, SourceLocation loc);
    void print(int tabs) override;
};

class IfNode : public ASTNode
{
public:
    std::unique_ptr<ASTNode> condition;
    std::vector<std::unique_ptr<ASTNode>> then_elements;
    std::vector<std::unique_ptr<ASTNode>> else_elements;

    IfNode(std::unique_ptr<ASTNode> c, std::vector<std::unique_ptr<ASTNode>> t, std::vector<std::unique_ptr<ASTNode>> e, SourceLocation loc);
    void print(int tabs) override;
};

class WhileNode : public ASTNode
{
public:
    std::unique_ptr<ASTNode> condition;
    std::vector<std::unique_ptr<ASTNode>> elements;

    std::string label = "";

    WhileNode(std::unique_ptr<ASTNode> c, std::vector<std::unique_ptr<ASTNode>> e, SourceLocation loc);
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

    ForNode(std::unique_ptr<ASTNode> i, std::unique_ptr<BinaryNode> c, std::unique_ptr<ASTNode> p, std::vector<std::unique_ptr<ASTNode>> e, SourceLocation loc);
    void print(int tabs) override;
};

class LoopControl : public ASTNode
{
public:
    TokenType type;
    std::string label;

    LoopControl(TokenType t, std::string l, SourceLocation loc);
    void print(int tabs) override;
};

class ArrayAccessNode : public ASTNode
{
public:
    std::unique_ptr<VarNode> array;
    std::unique_ptr<ASTNode> index;
    Type type = Type(BaseType::VOID);

    ArrayAccessNode(const ArrayAccessNode &other, SourceLocation loc);
    ArrayAccessNode(std::unique_ptr<VarNode> arr, std::unique_ptr<ASTNode> idx, SourceLocation loc);

    void print(int tabs) override;
};

class SizeOfNode : public ASTNode
{
public:
    Type type;
    std::unique_ptr<VarNode> var;

    SizeOfNode(Type t, SourceLocation loc);
    SizeOfNode(std::unique_ptr<VarNode> v, SourceLocation loc);
    void print(int tabs) override;
};

class IncludeNode : public ASTNode
{
public:
    std::string module_name;
    std::vector<std::string> args;

    IncludeNode(const std::string &module_name, std::vector<std::string> a, SourceLocation loc);
    void print(int tabs) override;
};