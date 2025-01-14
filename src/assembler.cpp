#include <cstdio>
#include <cerrno>
#include <iostream>
#include <functional>
#include <numeric>

#include "../include/assembler.h"
#include "../include/parser.h"

Assembler::Assembler(SymbolTable *symbolTable) : symbolTable(symbolTable) {};

void Assembler::assemble(std::vector<TACInstruction> instructions, std::string filename)
{
    // Create new file for the program
    FILE *file = fopen(filename.c_str(), "w");
    if (file == NULL)
    {
        char errorMessage[256];
        snprintf(errorMessage, sizeof(errorMessage), "Error opening file %s", filename.c_str());
        perror(errorMessage);
        return;
    }

    /*
        Specifies instructions are:
            - placed in text section of binary
            - normal and read-only
    */
    fprintf(file, "	.section	__TEXT,__text,regular,pure_instructions\n");

    // Specifies the version of the OS (macOS Sonoma) and the SDK version
    fprintf(file, "	.build_version macos, 15, 0	sdk_version 15, 1\n");

    // Declares the entrypoint of the program when executed (what crt0 calls)
    fprintf(file, "	.globl	_main\n");

    /*
        Aligns the _main function to a 16-byte boundary (2^4 = 16) for performance reasons
        The 0x90 specifies that the padding, if needed, will be filled with NOP (no-operation) instructions to ensure alignment
    */
    fprintf(file, "	.p2align	4, 0x90\n");
    fprintf(file, "\n");

    for (auto &instruction : instructions)
        assemble_tac(instruction, file);
}

void Assembler::assemble_tac(TACInstruction &instruction, FILE *file)
{
    if (instruction.op == TACOp::FUNC_BEGIN)
    {
        fprintf(file, "_%s:\n", instruction.arg1.c_str());
        /*
            Pushes rbp onto stack (this saves caller's base pointer allowing it to be restored back later)
            Copies the stack pointer into rbp to setup the base pointer for the current function's stack frame (for local variables/parameters)
        */
        fprintf(file, "	pushq	%%rbp\n");
        fprintf(file, "	movq	%%rsp, %%rbp\n"); // movq is used for 64 bits

        /*
            Allocates space on the stack for local variables by moving the stack pointer down
            Note that this should eventually involve a hashmap of function names and symbol tables
            to retrieve the correct amount of space
        */
        int stack_space = symbolTable->get_var_count() * 8;
        fprintf(file, "	subq	$%d, %%rsp\n\n", stack_space);
    }
    else if (instruction.op == TACOp::FUNC_END)
    {
        int stack_space = symbolTable->get_var_count() * 8;
        fprintf(file, "\n	addq	$%d, %%rsp\n", stack_space);
        fprintf(file, "	popq	%%rbp\n");
        fprintf(file, "	retq\n");
    }
    else if (instruction.op == TACOp::ASSIGN)
    {
        /*
            For this instruction
            - arg1 is the variable name (this should exist)
            - arg2 is the value to be assigned (variable or constant)
        */
        Symbol *var = symbolTable->find_symbol(instruction.arg1);
        Symbol *potential_var = symbolTable->find_symbol(instruction.arg2);

        if (potential_var == nullptr)
        {
            fprintf(file, "	movl	$%s, %d(%%rsp)\n", instruction.arg2.c_str(), var->stack_offset);
        }
        else
        {
            fprintf(file, "        movl    %d(%%rsp), %%r10d\n", potential_var->stack_offset);
            fprintf(file, "        movl    %%r10d, %d(%%rsp)\n", var->stack_offset);
        }
    }
    else if (instruction.op == TACOp::RETURN)
    {
        Symbol *potential_var = symbolTable->find_symbol(instruction.arg1);

        if (potential_var == nullptr)
        {
            fprintf(file, "	movl	$%s, %%eax\n", instruction.arg1.c_str());
        }
        else
        {
            fprintf(file, "        movl    %d(%%rsp), %%eax\n", potential_var->stack_offset);
        }
    }
    else if (instruction.op == TACOp::ADD)
    {
        Symbol *potential_var = symbolTable->find_symbol(instruction.arg1);
        Symbol *potential_var_2 = symbolTable->find_symbol(instruction.arg2);
        Symbol *result_var = symbolTable->find_symbol(instruction.result);

        if (potential_var == nullptr)
        {
            fprintf(file, "	movl	$%s, %%r10d\n", instruction.arg1.c_str());
        }
        else
        {
            fprintf(file, "        movl    %d(%%rsp), %%r10d\n", potential_var->stack_offset);
        }

        if (potential_var_2 == nullptr)
        {
            fprintf(file, "	addl	$%s, %%r10d\n", instruction.arg2.c_str());
        }
        else
        {
            fprintf(file, "  addl    %d(%%rsp), %%r10d\n", potential_var_2->stack_offset);
        }

        fprintf(file, "        movl    %%r10d, %d(%%rsp)\n", result_var->stack_offset);
    }
}