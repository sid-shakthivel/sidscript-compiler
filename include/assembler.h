#pragma once

#include <string>
#include <cstdio>
#include <memory>
#include <cerrno>
#include <iostream>
#include <fstream>

#include "symbolTable.h"
#include "globalSymbolTable.h"
#include "tacGenerator.h"

enum class VarType
{
    BSS,
    DATA,
    TEXT
};

class Assembler
{
public:
    Assembler(std::shared_ptr<GlobalSymbolTable> gst, std::string filename);
    void assemble(std::vector<TACInstruction> instructions);

private:
    std::shared_ptr<GlobalSymbolTable> gst;
    std::string current_func = "";
    VarType current_var_type = VarType::TEXT;
    FILE *file;

    void assemble_tac(TACInstruction &instruction, FILE *file);

    std::function<void(const std::string &, const char *, Type type)> load_to_reg;
    std::function<void(const std::string &operand, const char *reg, Type type)> store_from_reg;
    std::function<void(const std::string &operand, const char *reg, const std::string &op, Type type)> apply_bin_op_to_reg;
    std::function<void(const std::string &operand_a, const std::string &operand_b, const std::string &result, const char *reg, const std::string &op, Type type)> compare_and_store_result;

    // std::function<void(const std::string &src, const std::string &dest, const std::string &comment)> emit_mov_c_static;
    // std::function<void(const std::string &src, int dest_offset, const std::string &comment)> emit_mov_c_stack;
    // std::function<void(const std::string &src, const std::string &dest, const std::string &comment)> emit_mov_static_static;
    // std::function<void(const std::string &src, int dest_offset, const std::string &comment)> emit_mov_static_stack;
    // std::function<void(int src_offset, int dest_offset, const std::string &comment)> emit_mov_stack_stack;
};
