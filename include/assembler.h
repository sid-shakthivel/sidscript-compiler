#pragma once

#include <string>
#include <cstdio>

#include "symbolTable.h"
#include "globalSymbolTable.h"
#include "tacGenerator.h"

enum class VarType
{
    BSS,
    DATA,
    REGULAR
};

class Assembler
{
public:
    Assembler(GlobalSymbolTable *gst);
    void assemble(std::vector<TACInstruction> instructions, std::string filename);

private:
    GlobalSymbolTable *gst;
    std::string current_func = "";
    VarType current_var_type = VarType::REGULAR;

    void assemble_tac(TACInstruction &instruction, FILE *file);
};