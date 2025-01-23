#pragma once

#include <string>
#include <cstdio>

#include "symbolTable.h"
#include "globalSymbolTable.h"
#include "tacGenerator.h"

class Assembler
{
public:
    Assembler(GlobalSymbolTable *gst);
    void assemble(std::vector<TACInstruction> instructions, std::string filename);

private:
    GlobalSymbolTable *gst;
    std::string current_func = "";

    void assemble_tac(TACInstruction &instruction, FILE *file);
};