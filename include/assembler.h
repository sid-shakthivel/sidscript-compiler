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
    Assembler(std::shared_ptr<GlobalSymbolTable> gst, std::string filename);
    void assemble(const std::vector<TACInstruction> &instructions);

private:
    std::shared_ptr<GlobalSymbolTable> gst;
    VarType current_var_type = VarType::TEXT;
    FILE *file;

    std::unordered_map<TACOp, std::function<void(TACInstruction)>> handlers;

    void initialize_handlers();
    void compare_and_store_result(const std::string &operand_a, const std::string &operand_b, const std::string &result, const char *reg, const std::string &op, Type type);

    void handle_func_begin(TACInstruction &instruction);
    void handle_func_end(TACInstruction &instruction);
    void handle_return(TACInstruction &instruction);
    void handle_bin_op(TACInstruction &instruction, const std::string &op);
    void handle_cmp_op(TACInstruction &instruction, const std::string &op);
    void handle_if(TACInstruction &instruction);
    void handle_goto(TACInstruction &instruction);
    void handle_label(TACInstruction &instruction);
    void handle_call(TACInstruction &instruction);
    void handle_mov(TACInstruction &instruction);
    void handle_nop(TACInstruction &instruction);
    void handle_section(TACInstruction &instruction);
    void handle_unary_op(TACInstruction &instruction, const std::string &op);
    void handle_convert_type(TACInstruction &instruction);
    void handle_deref(TACInstruction &instruction);
    void handle_addr_of(TACInstruction &instruction);
    void handle_printf(TACInstruction &instruction);
    void handle_struct_init(TACInstruction &instruction);
    void handle_member_assign(TACInstruction &instruction);
    void handle_member_access(TACInstruction &instruction);
    void handle_not(TACInstruction &instruction);
    
    std::string get_mov_instruction(Type type);
    std::string get_cmp_instruction(Type type);
    std::string get_reg_name(const char *base_reg, Type type);
    std::string format_memory_ref(const std::string &sym_name);
    std::string format_instr(std::string instr, Type type);
    std::string fix_sign_instr(std::string instr);

    void load_to_reg(const std::string &operand, const char *reg, Type type, std::string arg2 = "");
    void store_from_reg(const std::string &operand, const char *reg, Type type, std::string arg2 = "");
    
    void handle_log_and(TACInstruction &instruction);
    void handle_log_or(TACInstruction &instruction);
    void handle_log_op(TACInstruction &instruction, const std::string &op);

    void handle_mod(TACInstruction &instruction);
    void handle_div(TACInstruction &instruction);
    void handle_div_mod(TACInstruction &instruction, bool is_mod);

    void handle_assign(TACInstruction &instruction);
    void handle_text_assign(TACInstruction &instruction);
    void handle_bss_assign(TACInstruction &instruction);
    void handle_data_assign(TACInstruction &instruction);
    void handle_literal8_assign(TACInstruction &instruction);
    void handle_str_assign(TACInstruction &instruction);

    void comment_instruction(TACInstruction &instr);
    std::string double_to_hex(double value);
    void error(const std::string &message);
};