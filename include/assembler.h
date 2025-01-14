#pragma once

#include <string>
#include <cstdio>

#include "symbolTable.h"
#include "tacGenerator.h"

class Assembler
{
public:
    Assembler(SymbolTable *symbolTable);
    void assemble(std::vector<TACInstruction> instructions, std::string filename);

private:
    SymbolTable *symbolTable;

    void assemble_tac(TACInstruction &instruction, FILE *file);
};