#pragma once

#include <vector>
#include <string>
#include <memory>

#include "globalSymbolTable.h"
#include "symbolTable.h"
#include "ast.h"

class SemanticAnalyser;

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
    NOT,
    NOP,
    INCREMENT,
    DECREMENT,
    MOV,
    PUSH,
    CALL,
    ENTER_BSS,
    ENTER_DATA,
    ENTER_TEXT,
    ENTER_STR,
    ENTER_LITERAL8,
    CONVERT_TYPE,
    DEREF,
    ADDR_OF,
    PRINTF,
    STRUCT_INIT,
    MEMBER_ACCESS,
    MEMBER_ASSIGN,
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

    BinOpType cmp_op; // Optional argument
    std::string arg3; // Another optional argument

    TACInstruction(TACOp op, const std::string &arg1 = "",
                   const std::string &arg2 = "", const std::string &result = "", Type type = Type(BaseType::VOID))
        : op(op), arg1(arg1), arg2(arg2), result(result), cmp_op(BinOpType::EQUAL), type(type), arg3{""} {}
};

class TacGenerator
{
public:
    TacGenerator(std::shared_ptr<GlobalSymbolTable> gst, std::shared_ptr<SemanticAnalyser> sem_analyser);

    void generate_all_tac(std::shared_ptr<ProgramNode> program);
    void print_all_tac();
    static std::string gen_tac_str(TACInstruction &instruction);

    const std::vector<TACInstruction> &get_instructions() const { return instructions; }

private:
    std::shared_ptr<GlobalSymbolTable> gst;
    std::shared_ptr<SemanticAnalyser> sem_analyser;

    std::vector<TACInstruction> instructions;
    std::vector<TACInstruction> bss_vars;
    std::vector<TACInstruction> data_vars;
    std::vector<TACInstruction> literal8_vars;
    std::vector<TACInstruction> str_vars;

    std::unordered_map<NodeType, std::function<void(ASTNode *)>> handlers;
    std::unordered_map<NodeType, std::function<std::string(ASTNode *)>> expr_handlers;

    std::array<std::string, 6> registers = {"%edi", "%esi", "%edx", "%ecx", "%r8", "%r9"};
    std::array<std::string, 6> x64_registers = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};

    int tempCounter = 0;
    int labelCounter = 0;
    int constCounter = 0;

    std::string gen_new_temp_var();
    std::string gen_new_label(std::string label = "");
    std::string gen_new_const_label();

    void generate_tac(ASTNode *node);

    void generate_tac_func(ASTNode *element);
    void generate_tac_rtn(ASTNode *element);
    void generate_tac_var_decl(ASTNode *element);
    void generate_tac_var_assign(ASTNode *element);
    void generate_tac_if(ASTNode *element);
    void generate_tac_while(ASTNode *element);
    void generate_tac_for(ASTNode *element);
    void generate_tac_loop_ctrl(ASTNode *element);
    void generate_tac_postfix(ASTNode *element);
    void generate_tac_func_call(ASTNode *element);

    std::string generate_tac_expr(ASTNode *expr);
    std::string generate_tac_expr_var(ASTNode *expr);
    std::string generate_tac_expr_bool(ASTNode *expr);
    std::string generate_tac_expr_cast(ASTNode *expr);
    std::string generate_tac_expr_number(ASTNode *expr);
    std::string generate_tac_expr_char(ASTNode *expr);
    std::string generate_tac_expr_string(ASTNode *expr);
    std::string generate_tac_expr_unary(ASTNode *expr);
    std::string generate_tac_expr_binary(ASTNode *expr);
    std::string generate_tac_expr_postfix(ASTNode *expr);
    std::string generate_tac_expr_array_access(ASTNode *expr);
    std::string generate_tac_expr_func_call(ASTNode *expr);
    std::string generate_tac_expr_size_of(ASTNode *expr);

    void generate_tac_var_array_assign(VarNode *var_node, Symbol *var_symbol, ASTNode *value);
    void generate_tac_cmp(ASTNode *condition, const std::string &label_success,
                          const std::string &label_failure);

    void error(const std::string &message);
};