#pragma once

#include <vector>
#include <string>
#include <memory>

#include "globalSymbolTable.h"
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
    INCREMENT,
    DECREMENT,
    MOV,
    PUSH,
    CALL,
    ENTER_BSS,
    ENTER_DATA,
    ENTER_TEXT,
    CONVERT_TYPE
};

TACOp convert_UnaryOpType_to_TACOp(UnaryOpType op);
TACOp convert_BinOpType_to_TACOp(BinOpType op);

struct TACInstruction
{
    TACOp op;
    std::string arg1;   // First argument
    std::string arg2;   // Second argument (optional)
    std::string result; // Result variable or temporary
    Type type;

    TACOp op2; // Optional argument

    TACInstruction(TACOp op, const std::string &arg1 = "",
                   const std::string &arg2 = "", const std::string &result = "", Type type = Type::VOID)
        : op(op), arg1(arg1), arg2(arg2), result(result), op2(TACOp::NOP), type(type) {}
};

class TacGenerator
{
public:
    TacGenerator(std::shared_ptr<GlobalSymbolTable> gst);

    void generate_tac(std::shared_ptr<ProgramNode> program);
    void print_all_tac();
    static std::string gen_tac_str(TACInstruction &instruction);

    const std::vector<TACInstruction> &get_instructions() const { return instructions; }

private:
    std::shared_ptr<GlobalSymbolTable> gst;
    SymbolTable *current_st = nullptr;
    std::string current_func = "";

    std::vector<TACInstruction> instructions;
    std::vector<TACInstruction> bss_vars;
    std::vector<TACInstruction> data_vars;

    std::array<std::string, 6> registers = {"%edi", "%esi", "%edx", "%ecx", "%r8", "%r9"};

    int tempCounter;
    int labelCounter;

    std::string gen_new_temp_var();
    std::string gen_new_label(std::string label = "");

    void generate_tac_func(FuncNode *func);
    void generate_tac_element(ASTNode *element);

    std::string generate_tac_expr(ASTNode *expr, Type type = Type::VOID);
};