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
};

class Assembler
{
public:
    Assembler(std::shared_ptr<GlobalSymbolTable> gst, std::string filename);
    void assemble(const std::vector<TACInstruction> &instructions);

private:
    std::shared_ptr<GlobalSymbolTable> gst;
    std::string current_func = "";
    VarType current_var_type = VarType::TEXT;
    FILE *file;

    std::unordered_map<TACOp, std::function<void(TACInstruction)>> handlers;

    void initialize_handlers();
    void load_to_reg(const std::string &operand, const char *reg, Type type);
    void store_from_reg(const std::string &operand, const char *reg, Type type);
    void apply_bin_op_to_reg(const std::string &operand, const char *reg, const std::string &op, Type type);
    void compare_and_store_result(const std::string &operand_a, const std::string &operand_b, const std::string &result, const char *reg, const std::string &op, Type type);

    void handle_func_begin(TACInstruction &instruction);
    void handle_func_end(TACInstruction &instruction);
    void handle_assign(TACInstruction &instruction);
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
    void handle_mod(TACInstruction &instruction);
    void handle_div(TACInstruction &instruction);
    void handle_convert_type(TACInstruction &instruction);
    void handle_deref(TACInstruction &instruction);
    void handle_addr_of(TACInstruction &instruction);

    bool is_signed(Type &type);
    bool is_8_bytes(Type &type);

    std::string double_to_hex(double value);
};