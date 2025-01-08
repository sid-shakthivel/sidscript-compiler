#pragma once

#include <string>
#include <cstdio>

#include "ast.h"

class Assembler
{
public:
    Assembler();
    void assemble(ProgramNode *program, std::string filename);

private:
    void assemble_func(FuncNode *func, FILE *file);
    void assemble_stmt(ASTNode *stmt, FILE *file);
    void assemble_expr(ASTNode *expr, FILE *file);
};