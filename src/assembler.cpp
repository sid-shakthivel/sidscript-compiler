#include <cstdio>
#include <cerrno>
#include <iostream>
#include <functional>
#include <numeric>

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

    /*
    Three-Address Code (TAC) is an IR to simplify code generation
    Breaks down complex expressions/statements into a sequence of instructions where:
        - Each instruction has at most three operands
        - Temporary variables are used to hold intermediate results
    Helps for debugging and optimisation

    This method calcualtes the stack space required for the function
    */
    int stack_space = std::accumulate(func->stmts.begin(), func->stmts.end(), 0,
                                      [this](int acc, ASTNode *stmt)
                                      {
                                          return acc + this->calculate_stack_space(stmt);
                                      }) *
                      8; // 8 bits is word size

    fprintf(file, "	subq	$%d, %%rsp\n\n", stack_space);

    func_temp_var_count = 1;

    for (auto stmt : func->stmts)
    {
        assemble_stmt(stmt, file);
    }

    /*
    Function epilogue
    Clean up stack
    Restores the previous base pointer by popping the saved value from the stack into %rbp (cleans up stack frame)
    Returns control to the caller by popping the return address off the stack and jumping to it.
    */
    fprintf(file, "\n	addq	$%d, %%rsp\n", stack_space);
    fprintf(file, "	popq	%%rbp\n");
    fprintf(file, "	retq\n");
}

void Assembler::assemble_stmt(ASTNode *stmt, FILE *file)
{

    if (stmt->type == NodeType::NODE_RETURN)
    {
        ASTNode *value = ((RtnNode *)stmt)->value;

        // /*
        //     Moves the immediate value into the eax register (which holds return value of functions)
        //     System V AMD64 calling convention is used by macOS
        //     */
        // int rtn_value = ((IntegerLiteral *)value)->value;
        // fprintf(file, "	movl	$%d, %%eax\n", rtn_value);

        assemble_expr(value, file);
        fprintf(file, "	movl	%d(%%rbp), %%eax\n", func_temp_var_count * -4);
    }
}

void Assembler::assemble_expr(ASTNode *expr, FILE *file)
{
    if (expr->type == NodeType::NODE_INTEGER)
    {
        IntegerLiteral *literal = (IntegerLiteral *)expr;
        fprintf(file, "        movl    $%d, %d(%%rbp)\n", literal->value, func_temp_var_count * -4);
    }
    else if (expr->type == NodeType::NODE_UNARY)
    {
        UnaryNode *unary = (UnaryNode *)expr;
        assemble_unary(unary, file);
    }
    else if (expr->type == NodeType::NODE_BINARY)
    {
        BinaryNode *binary = (BinaryNode *)expr;
        assemble_binary(binary, file);
    }
}

void Assembler::assemble_binary(BinaryNode *binary, FILE *file)
{
    assemble_expr(binary->left, file);
    int old_temp_var_count = func_temp_var_count;
    func_temp_var_count += 1;

    assemble_expr(binary->right, file);

    switch (binary->op)
    {
    case BinOpType::ADD:
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", func_temp_var_count * -4);
        fprintf(file, "        addl    %d(%%rbp), %%r10d\n", old_temp_var_count * -4);
        break;
    case BinOpType::SUB:
        // Ensure that the second operand is subtracted from the first ie (6 - 3 = 3, not 3 - 6 = -3)
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", old_temp_var_count * -4);
        fprintf(file, "        subl    %d(%%rbp), %%r10d\n", func_temp_var_count * -4);
        break;
    case BinOpType::MUL:
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", func_temp_var_count * -4);
        fprintf(file, "        imull   %d(%%rbp), %%r10d\n", old_temp_var_count * -4);
        break;
    case BinOpType::DIV:
        fprintf(file, "        movl    %d(%%rbp), %%eax\n", old_temp_var_count * -4);
        fprintf(file, "        cdq\n");
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", func_temp_var_count * -4);
        fprintf(file, "        idivl   %%r10d\n");
        fprintf(file, "        movl    %%eax, %%r10d\n");
        break;
    case BinOpType::MOD:
        fprintf(file, "        movl    %d(%%rbp), %%eax\n", old_temp_var_count * -4);
        fprintf(file, "        cdq\n");
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", func_temp_var_count * -4);
        fprintf(file, "        idivl   %%r10d\n");
        fprintf(file, "        movl    %%edx, %%r10d\n");
        break;
    case BinOpType::LESS_THAN:
        // The cmpl instruction compares the first operand (src) to the second operand (dest) as dest - src.
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", old_temp_var_count * -4);
        fprintf(file, "        cmpl    %d(%%rbp), %%r10d\n", func_temp_var_count * -4);
        fprintf(file, "        setl    %%r10b\n");
        fprintf(file, "        movzbl  %%r10b, %%r10d\n");
        break;
    case BinOpType::GREATER_THAN:
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", old_temp_var_count * -4);
        fprintf(file, "        cmpl    %d(%%rbp), %%r10d\n", func_temp_var_count * -4);
        fprintf(file, "        setg    %%r10b\n");
        fprintf(file, "        movzbl  %%r10b, %%r10d\n");
        break;
    case BinOpType::LESS_OR_EQUAL:
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", old_temp_var_count * -4);
        fprintf(file, "        cmpl    %d(%%rbp), %%r10d\n", func_temp_var_count * -4);
        fprintf(file, "        setle    %%r10b\n");
        fprintf(file, "        movzbl  %%r10b, %%r10d\n");
        break;
    case BinOpType::GREATER_OR_EQUAL:
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", old_temp_var_count * -4);
        fprintf(file, "        cmpl    %d(%%rbp), %%r10d\n", func_temp_var_count * -4);
        fprintf(file, "        setge    %%r10b\n");
        fprintf(file, "        movzbl  %%r10b, %%r10d\n");
        break;
    case BinOpType::EQUAL:
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", old_temp_var_count * -4);
        fprintf(file, "        cmpl    %d(%%rbp), %%r10d\n", func_temp_var_count * -4);
        fprintf(file, "        sete    %%r10b\n");
        fprintf(file, "        movzbl  %%r10b, %%r10d\n");
        break;
    case BinOpType::NOT_EQUAL:
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", old_temp_var_count * -4);
        fprintf(file, "        cmpl    %d(%%rbp), %%r10d\n", func_temp_var_count * -4);
        fprintf(file, "        setne    %%r10b\n");
        fprintf(file, "        movzbl  %%r10b, %%r10d\n");
        break;
    case BinOpType::OR:
    {
        std::string true_label = ".true_table_" + std::to_string(temp_label_count);
        std::string end_label = ".end_table_" + std::to_string(temp_label_count);
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", old_temp_var_count * -4);
        fprintf(file, "        testl %%r10d, %%r10d\n");
        fprintf(file, "        jnz %s\n", true_label.c_str());
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", func_temp_var_count * -4);
        fprintf(file, "        testl %%r10d, %%r10d\n");
        fprintf(file, "        jnz %s\n", true_label.c_str());
        fprintf(file, "        movl    $0, %%r10d\n");
        fprintf(file, "        jmp %s\n", end_label.c_str());
        fprintf(file, "%s:\n", true_label.c_str());
        fprintf(file, "        movl    $1, %%r10d\n");
        fprintf(file, "%s:\n", end_label.c_str());
        temp_label_count++;
        break;
    }
    case BinOpType::AND:
    {
        std::string false_label = ".false_table_" + std::to_string(temp_label_count);
        std::string end_label = ".end_table_" + std::to_string(temp_label_count);
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", old_temp_var_count * -4);
        fprintf(file, "        testl %%r10d, %%r10d\n");
        fprintf(file, "        jz %s\n", false_label.c_str());
        fprintf(file, "        movl    %d(%%rbp), %%r10d\n", func_temp_var_count * -4);
        fprintf(file, "        testl %%r10d, %%r10d\n");
        fprintf(file, "        jz %s\n", false_label.c_str());
        fprintf(file, "        movl    $1, %%r10d\n");
        fprintf(file, "        jmp %s\n", end_label.c_str());
        fprintf(file, "%s:\n", false_label.c_str());
        fprintf(file, "        movl    $0, %%r10d\n");
        fprintf(file, "%s:\n", end_label.c_str());
        temp_label_count++;
        break;
    }
    default:
        fprintf(stderr, "Error: Unsupported binary operation\n");
        exit(1);
    }

    fprintf(file, "        movl    %%r10d, %d(%%rbp)\n", func_temp_var_count * -4);
}

void Assembler::assemble_unary(UnaryNode *unary, FILE *file)
{
    assemble_expr(unary->value, file);
    int old_temp_var_count = func_temp_var_count;
    func_temp_var_count += 1;

    // Apply the unary operation
    switch (unary->op)
    {
    case NEGATE:
        fprintf(file, "        negl    %d(%%rbp)\n", old_temp_var_count * -4);
        break;

    case COMPLEMENT:
        fprintf(file, "        notl    %d(%%rbp)\n", old_temp_var_count * -4);
        break;

    default:
        fprintf(stderr, "Error: Unsupported unary operation\n");
        exit(1);
    }

    fprintf(file, "        movl    %d(%%rbp), %%r10d\n", old_temp_var_count * -4);
    fprintf(file, "        movl    %%r10d, %d(%%rbp)\n", func_temp_var_count * -4);
}

// Determines the number of temporary variables needed on the stack during execution.
int Assembler::calculate_stack_space(ASTNode *node)
{
    switch (node->type)
    {
    case NodeType::NODE_INTEGER:
        return 1;
    case NodeType::NODE_RETURN:
        return calculate_stack_space(((RtnNode *)node)->value);
    case NodeType::NODE_UNARY:
        return 1 + calculate_stack_space(((UnaryNode *)node)->value);
    case NodeType::NODE_BINARY:
        return 1 + calculate_stack_space(((BinaryNode *)node)->left) + calculate_stack_space(((BinaryNode *)node)->right);
    default:
        return 0;
    }
    return 0;
}