#pragma once

#include <vector>
#include <string>

#include "symbolTable.h"
#include "ast.h"

enum class TACOp
{
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    GT,
    LT,
    GTE,
    LTE,
    EQUAL,
    NOT_EQUAL,
    AND,
    OR,
    FUNC_BEGIN, // Function prologue
    FUNC_END,   // Function epilogue
    RETURN,
    ASSIGN,
    GOTO, // Unconditional jump
    IF,   // Conditional jump
    LABEL,
    ALLOC_STACK,
    DEALLOC_STACK,
    NEGATE,
    COMPLEMENT,
    NOP,
};

TACOp convert_UnaryOpType_to_TACOp(UnaryOpType op);
TACOp convert_BinOpType_to_TACOp(BinOpType op);

struct TACInstruction
{
    TACOp op;
    std::string arg1;   // First argument
    std::string arg2;   // Second argument (optional)
    std::string result; // Result variable or temporary

    TACOp op2;

    TACInstruction(TACOp op, const std::string &arg1 = "",
                   const std::string &arg2 = "", const std::string &result = "")
        : op(op), arg1(arg1), arg2(arg2), result(result), op2(TACOp::NOP) {}
};

class TacGenerator
{
public:
    TacGenerator(SymbolTable *symbolTable);

    std::vector<TACInstruction> generate_tac(ProgramNode *program);
    void print_tac();

private:
    SymbolTable *symbolTable;
    std::vector<TACInstruction> instructions;

    int tempCounter;
    int labelCounter;

    std::string gen_new_temp_var();
    std::string gen_new_label(std::string label = "");

    void generate_tac_func(FuncNode *func);
    void generate_tac_element(ASTNode *element);

    std::string generate_tac_expr(ASTNode *expr);
};