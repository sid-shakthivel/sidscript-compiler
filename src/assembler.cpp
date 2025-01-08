#include <cstdio>
#include <cerrno>
#include <iostream>

#include "../include/assembler.h"
#include "../include/parser.h"

Assembler::Assembler() {}

void Assembler::assemble(ProgramNode *program, std::string filename)
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

    // Initialise program

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

    assemble_func(program->func, file);
}

void Assembler::assemble_func(FuncNode *func, FILE *file)
{
    /*
        For now the only function is the main function
        so no checks are performed to check otherwise
    */

    fprintf(file, "_main:\n");

    /*
    Pushes rbp onto stack (this saves caller's base pointer allowing it to be restored back later)
    Copies the stack pointer into rbp to setup the base pointer for the current function's stack frame (for local variables/parameters)
    */
    fprintf(file, "	pushq	%%rbp\n");
    fprintf(file, "	movq	%%rsp, %%rbp\n"); // movq is used for 64 bits

    for (auto stmt : func->stmts)
    {
        assemble_stmt(stmt, file);
    }
}

void Assembler::assemble_stmt(ASTNode *stmt, FILE *file)
{
    // For now since the only stmt is return assume it is return

    /*
    Moves the immediate value into the eax register (which holds return value of functions)
    System V AMD64 calling convention is used by macOS
    Assume for now that all return values are integers
    */
    int rtn_value = ((IntegerLiteral *)((RtnNode *)stmt)->value)->value;
    fprintf(file, "	movl	$%d, %%eax\n", rtn_value); // mov1 is used for 32 bits

    /*
        Restores the previous base pointer by popping the saved value from the stack into %rbp (cleans up stack frame)
        Returns control to the caller by popping the return address off the stack and jumping to it.
    */
    fprintf(file, "	popq	%%rbp\n");
    fprintf(file, "	retq\n");
}