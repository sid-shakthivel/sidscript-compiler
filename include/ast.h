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
    NODE_ARRAY_INIT,
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
    NODE_CAST,
    NODE_POSTFIX,
    NODE_DEREF,
    NODE_ADDR_OF,
    NODE_ARRAY_ACCESS,
    NODE_CHAR,
    NODE_STRING
};

enum class BaseType
{
    INT,
    LONG,
    UINT,
    ULONG,
    DOUBLE,
    VOID,
    STRUCT,
    CHAR
};

class Type
{
private:
    BaseType base_type;
    int pointer_level = 0;
    std::vector<int> array_sizes;
    std::optional<std::string> struct_name;

public:
    Type(BaseType base);
    Type(BaseType base, int ptr_level);
    Type(std::string struct_name);

    Type &add_array_dimension(int size);

    bool is_pointer() const;
    bool is_array() const;
    bool is_struct() const;
    BaseType get_base_type() const;
    bool has_base_type(BaseType other) const;

    bool is_signed() const;

    std::string to_string() const;
    void print();

    size_t get_size() const;
    size_t get_array_size() const;
    bool is_size_8() const;

    // Type compatibility checks
    bool can_assign_from(const Type &other) const;
    bool can_convert_to(const Type &other) const;

    // Equality operators
    bool operator==(const Type &other) const;
    bool operator!=(const Type &other) const;

    bool is_integral() const;
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
Specifier get_specifier(const TokenType &t);
Type get_type_from_str(std::string &t);

class ASTNode
{
public:
    NodeType type;

    ASTNode(NodeType t) : type(t) {}
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
    NumericLiteral(NodeType t);
    Type value_type = Type(BaseType::VOID);

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

class UIntegerLiteral : public NumericLiteral
{
public:
    unsigned int value;

    UIntegerLiteral(unsigned int v);
    void print(int tabs) override;
};

class ULongLiteral : public NumericLiteral
{
public:
    unsigned long value;

    ULongLiteral(unsigned long v);
    void print(int tabs) override;
};

class DoubleLiteral : public NumericLiteral
{
public:
    double value;

    DoubleLiteral(double v);
    void print(int tabs) override;
};

class ArrayLiteral : public ASTNode
{
public:
    std::vector<std::unique_ptr<ASTNode>> values;
    Type arr_type = Type(BaseType::VOID);

    void add_element(std::unique_ptr<ASTNode> element);

    ArrayLiteral(Type t);
    void print(int tabs) override;
};

class CharLiteral : public ASTNode
{
public:
    char value;
    Type value_type = Type(BaseType::CHAR);

    CharLiteral(char v, Type t);
    void print(int tabs) override;
};

class StringLiteral : public ASTNode
{
public:
    std::string value;
    Type value_type = Type(BaseType::VOID);

    StringLiteral(const std::string &v, Type t);
    void print(int tabs) override;
};

class CastNode : public ASTNode
{
public:
    std::unique_ptr<ASTNode> expr;
    Type target_type;
    Type src_type;

    CastNode(std::unique_ptr<ASTNode> e, Type t1, Type t2 = Type(BaseType::VOID));
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
    Type return_type = Type(BaseType::VOID);
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
    Type type = Type(BaseType::VOID);

    UnaryNode(UnaryOpType o, std::unique_ptr<ASTNode> v);
    void print(int tabs) override;
};

class PostfixNode : public ASTNode
{
public:
    TokenType op;
    std::unique_ptr<ASTNode> value;
    Type type = Type(BaseType::VOID);

    PostfixNode(TokenType o, std::unique_ptr<ASTNode> v);
    void print(int tabs) override;
};

class BinaryNode : public ASTNode
{
public:
    BinOpType op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    Type type = Type(BaseType::VOID);

    BinaryNode(BinOpType o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r);
    void print(int tabs) override;
};

class VarNode : public ASTNode
{
public:
    std::string name;
    Type type = Type(BaseType::VOID);
    Specifier specifier = Specifier::NONE;

    VarNode(const std::string &n, Type t, Specifier s = Specifier::NONE);
    VarNode(const std::string &n);
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
    std::unique_ptr<ASTNode> var;
    std::unique_ptr<ASTNode> value;

    VarAssignNode(std::unique_ptr<ASTNode> v, std::unique_ptr<ASTNode> val);
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

class DerefNode : public ASTNode
{
public:
    std::unique_ptr<ASTNode> expr;
    Type type = Type(BaseType::VOID);

    DerefNode(std::unique_ptr<ASTNode> expr);
    void print(int tabs) override;
};

class AddrOfNode : public ASTNode
{
public:
    std::unique_ptr<ASTNode> expr;
    Type type = Type(BaseType::VOID);

    AddrOfNode(std::unique_ptr<ASTNode> expr);
    void print(int tabs) override;
};

class ArrayAccessNode : public ASTNode
{
public:
    std::unique_ptr<VarNode> array;
    std::unique_ptr<ASTNode> index;
    Type type = Type(BaseType::VOID);

    ArrayAccessNode(const ArrayAccessNode &other)
        : ASTNode(NodeType::NODE_ARRAY_ACCESS),
          array(std::make_unique<VarNode>(*other.array)),
          index(other.index ? other.index->clone() : nullptr) {}

    ArrayAccessNode(std::unique_ptr<VarNode> arr, std::unique_ptr<ASTNode> idx);

    void print(int tabs) override;
};