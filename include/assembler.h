#pragma once

#include <string>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <functional>

#include "symbolTable.h"
#include "globalSymbolTable.h"
#include "tacGenerator.h"

enum class VarType
{
    BSS,
    DATA,
    TEXT,
    LITERAL8,
    STR,
};

class Assembler
{
public:
    Assembler(std::shared_ptr<GlobalSymbolTable> &gst, const std::string &filename);
    void assemble(const std::vector<TACInstruction> &instructions);

private:
    std::shared_ptr<GlobalSymbolTable> gst;
    VarType current_var_type = VarType::TEXT;
    FILE *file;

    std::unordered_map<TACOp, std::function<void(const TACInstruction &)>> handlers;

    void compare_and_store_result(const std::string &operand_a, const std::string &operand_b, const std::string &result, const char *reg, const std::string &op, const Type &type);

    void emit_func_begin(const TACInstruction &instruction);
    void emit_func_end(const TACInstruction &instruction);
    void emit_return(const TACInstruction &instruction);
    void emit_bin_op(const TACInstruction &instruction, const std::string &op);
    void emit_cmp_op(const TACInstruction &instruction, const std::string &op);
    void emit_if(const TACInstruction &instruction);
    void emit_goto(const TACInstruction &instruction);
    void emit_label(const TACInstruction &instruction);
    void emit_call(const TACInstruction &instruction);
    void emit_mov_between_reg(const TACInstruction &instruction);
    void emit_nop(const TACInstruction &instruction);
    void emit_section(const TACInstruction &instruction);
    void emit_unary_op(const TACInstruction &instruction, const std::string &op);
    void emit_convert_type(const TACInstruction &instruction);
    void emit_deref(const TACInstruction &instruction);
    void emit_addr_of(const TACInstruction &instruction);
    void emit_struct_init(const TACInstruction &instruction);
    void emit_not(const TACInstruction &instruction);

    void emit_logical_op(const TACInstruction &instruction, const std::string &op);
    void emit_logical_and(const TACInstruction &instruction);
    void emit_logical_or(const TACInstruction &instruction);

    void emit_div_mod(const TACInstruction &instruction, const bool &is_mod);
    void emit_mod(const TACInstruction &instruction);
    void emit_div(const TACInstruction &instruction);

    std::string select_mov_instr(const Type &type);
    std::string select_cmp_instr(const Type &type);
    std::string select_reg_name(const char *base_reg, const Type &type);
    std::string select_conditional_jmp(const BinOpType &op, const Type &type);

    std::string format_mem_operand(const std::string &sym_name);
    std::string format_typed_instr(const std::string &instr, const Type &type);
    std::string normalise_signed_instr(const std::string &instr);

    void emit_load(const std::string &operand, const char *reg, Type type, const std::string &arg2 = "");
    void emit_store(const std::string &operand, const char *reg, Type type, const std::string &arg2 = "");

    void emit_assign(const TACInstruction &instruction);
    void emit_text_assign(const TACInstruction &instruction);
    void emit_bss_assign(const TACInstruction &instruction);
    void emit_data_assign(const TACInstruction &instruction);
    void emit_literal8_assign(const TACInstruction &instruction);
    void emit_str_assign(const TACInstruction &instruction);

    void emit_comment_instr(const TACInstruction &instr);
    std::string encode_double_hex(const double &value);
    void report_error(const std::string &message);
};