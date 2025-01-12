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
    void assemble_element(ASTNode *element, FILE *file);
    void assemble_expr(ASTNode *expr, FILE *file);
    void assemble_unary(UnaryNode *unary, FILE *file);
    void assemble_binary(BinaryNode *binary, FILE *file);

    int calculate_stack_space(ASTNode *node);
    unsigned int func_temp_var_count = 0;
    unsigned int temp_label_count = 0;
};