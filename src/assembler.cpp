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
	/*
		Helper functions which load and store data to and from registers (variables or immediate values)
	*/
	auto load_to_reg = [file, this](const std::string &operand, const char *reg)
	{
		Symbol *potential_var = symbolTable->find_symbol(operand);
		if (potential_var == nullptr)
			fprintf(file, "\tmovl\t$%s, %s\n", operand.c_str(), reg);
		else
			fprintf(file, "\tmovl\t%d(%%rsp), %s\n", potential_var->stack_offset, reg);
	};

	auto store_from_reg = [file, this](const std::string &operand, const char *reg)
	{
		Symbol *potential_var = symbolTable->find_symbol(operand);
		if (potential_var == nullptr)
			fprintf(file, "\tmovl\t%s, $%s\n", reg, operand.c_str());
		else
			fprintf(file, "\tmovl\t%s, %d(%%rsp)\n", reg, potential_var->stack_offset);
	};

	auto bin_op_to_reg = [file, this](const std::string &operand, const char *reg, const std::string &op)
	{
		Symbol *potential_var = symbolTable->find_symbol(operand);

		if (potential_var == nullptr)
			fprintf(file, "\t%s\t$%s, %s\n", op.c_str(), operand.c_str(), reg);
		else
			fprintf(file, "\t%s\t%d(%%rsp), %s\n", op.c_str(), potential_var->stack_offset, reg);
	};

	auto cmp_op_to_reg = [file, this, load_to_reg, store_from_reg](const std::string &operand_a, const std::string &operand_b, const std::string &result, const char *reg, const std::string &op)
	{
		load_to_reg(operand_a, reg);

		Symbol *potential_var_b = symbolTable->find_symbol(operand_b);

		if (potential_var_b == nullptr)
			fprintf(file, "\tcmpl\t$%s, %s\n", operand_b.c_str(), reg);
		else
			fprintf(file, "\tcmpl\t%d(%%rsp), %s\n", potential_var_b->stack_offset, reg);

		std::string reg_b = reg;
		reg_b.pop_back();
		reg_b += "b";

		fprintf(file, "\t%s\t%s\n", op.c_str(), reg_b.c_str());
		fprintf(file, "\tmovzbl	%s, %s\n", reg_b.c_str(), reg);

		store_from_reg(result, reg);
	};

	auto get_jmp_from_op = [file, this](const TACOp &op)
	{
		switch (op)
		{
		case TACOp::EQUAL:
			return "jne";
		case TACOp::NOT_EQUAL:
			return "jne";
		case TACOp::LT:
			return "jne";
		case TACOp::LTE:
			return "jne";
		case TACOp::GT:
			return "jne";
		case TACOp::GTE:
			return "jne";
		case TACOp::AND:
			return "jne";
		case TACOp::OR:
			return "jne";
		default:
			return "";
		};
	};

	if (instruction.op == TACOp::FUNC_BEGIN)
	{
		current_func = instruction.arg1;
		fprintf(file, "_%s:\n", instruction.arg1.c_str());
		/*
			Pushes rbp onto stack (this saves caller's base pointer allowing it to be restored back later)
			Copies the stack pointer into rbp to setup the base pointer for the current function's stack frame (for local variables/parameters)
		*/
		fprintf(file, "\tpushq	%%rbp\n");
		fprintf(file, "\tmovq	%%rsp, %%rbp\n"); // movq is used for 64 bits

		/*
			Allocates space on the stack for local variables by moving the stack pointer down
			Note that this should eventually involve a hashmap of function names and symbol tables
			to retrieve the correct amount of space
		*/
		int stack_space = symbolTable->get_var_count() * 8;
		fprintf(file, "\tsubq	$%d, %%rsp\n\n", stack_space);
	}
	else if (instruction.op == TACOp::FUNC_END)
	{
		fprintf(file, "\n.L%s_end:", current_func.c_str());
		int stack_space = symbolTable->get_var_count() * 8;
		fprintf(file, "\n\taddq	$%d, %%rsp\n", stack_space);
		fprintf(file, "\tpopq	%%rbp\n");
		fprintf(file, "\tretq\n\n");
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
			fprintf(file, "\tmovl	$%s, %d(%%rsp)\n", instruction.arg2.c_str(), var->stack_offset);
		}
		else
		{
			fprintf(file, "\tmovl    %d(%%rsp), %%r10d\n", potential_var->stack_offset);
			fprintf(file, "\tmovl    %%r10d, %d(%%rsp)\n", var->stack_offset);
		}
	}
	else if (instruction.op == TACOp::RETURN)
	{
		load_to_reg(instruction.arg1, "%eax");
		fprintf(file, "\tjmp\t.L%s_end\n", current_func.c_str());
	}
	else if (instruction.op == TACOp::ADD)
	{
		load_to_reg(instruction.arg1, "%r10d");
		bin_op_to_reg(instruction.arg2, "%r10d", "addl");
		store_from_reg(instruction.result, "%r10d");
	}
	else if (instruction.op == TACOp::SUB)
	{
		load_to_reg(instruction.arg1, "%r10d");
		bin_op_to_reg(instruction.arg2, "%r10d", "subl");
		store_from_reg(instruction.result, "%r10d");
	}
	else if (instruction.op == TACOp::MUL)
	{
		load_to_reg(instruction.arg1, "%r10d");
		bin_op_to_reg(instruction.arg2, "%r10d", "imull");
		store_from_reg(instruction.result, "%r10d");
	}
	else if (instruction.op == TACOp::DIV)
	{
		load_to_reg(instruction.arg1, "%r10d");
		fprintf(file, "\tcltd\n");
		load_to_reg(instruction.arg2, "%r10d");
		fprintf(file, "\tidivl\t%%r10d\n");
		store_from_reg(instruction.result, "%eax"); // eax for quotient
	}
	else if (instruction.op == TACOp::MOD)
	{
		load_to_reg(instruction.arg1, "%r10d");
		fprintf(file, "\tcltd\n");
		load_to_reg(instruction.arg2, "%r10d");
		fprintf(file, "\tidivl\t%%r10d\n");
		store_from_reg(instruction.result, "%edx"); // eax for remainder
	}
	else if (instruction.op == TACOp::COMPLEMENT)
	{
		// Refactor into function
		load_to_reg(instruction.arg1, "%r10d");
		fprintf(file, "\notl\t%%r10d\n");
		store_from_reg(instruction.result, "%r10d");
	}
	else if (instruction.op == TACOp::NEGATE)
	{
		load_to_reg(instruction.arg1, "%r10d");
		fprintf(file, "\negl\t%%r10d\n");
		store_from_reg(instruction.result, "%r10d");
	}
	else if (instruction.op == TACOp::LT)
	{
		// (const std::string &operand_a, const std::string &operand_b, const std::string &result, const char *reg, const std::string &op)
		cmp_op_to_reg(instruction.arg1, instruction.arg2, instruction.result, "%r10d", "setl");
	}
	else if (instruction.op == TACOp::LTE)
	{
		cmp_op_to_reg(instruction.arg1, instruction.arg2, instruction.result, "%r10d", "setle");
	}
	else if (instruction.op == TACOp::GT)
	{
		cmp_op_to_reg(instruction.arg1, instruction.arg2, instruction.result, "%r10d", "setg");
	}
	else if (instruction.op == TACOp::GTE)
	{
		cmp_op_to_reg(instruction.arg1, instruction.arg2, instruction.result, "%r10d", "setge");
	}
	else if (instruction.op == TACOp::EQUAL)
	{
		cmp_op_to_reg(instruction.arg1, instruction.arg2, instruction.result, "%r10d", "sete");
	}
	else if (instruction.op == TACOp::NOT_EQUAL)
	{
		cmp_op_to_reg(instruction.arg1, instruction.arg2, instruction.result, "%r10d", "setne");
	}
	else if (instruction.op == TACOp::IF)
	{
		Symbol *potential_var = symbolTable->find_symbol(instruction.arg1);

		if (potential_var == nullptr)
			fprintf(file, "\tcmpl\t$0, %s\n", instruction.arg1.c_str());
		else
			fprintf(file, "\tcmpl\t$0, %d(%%rsp)\n", potential_var->stack_offset);

		fprintf(file, "\t%s\t%s\n", get_jmp_from_op(instruction.op2), instruction.result.c_str());
	}
	else if (instruction.op == TACOp::GOTO)
	{
		fprintf(file, "\tjmp\t%s\n", instruction.result.c_str());
	}
	else if (instruction.op == TACOp::LABEL)
	{
		fprintf(file, "\n%s:\n", instruction.arg1.c_str());
	}
	else if (instruction.op == TACOp::AND)
	{
		load_to_reg(instruction.arg1, "%r10d");
		bin_op_to_reg(instruction.arg2, "%r10d", "andl");
		store_from_reg(instruction.result, "%r10d");
	}
	else if (instruction.op == TACOp::OR)
	{
		load_to_reg(instruction.arg1, "%r10d");
		bin_op_to_reg(instruction.arg2, "%r10d", "orl");
		store_from_reg(instruction.result, "%r10d");
	}
	else if (instruction.op == TACOp::CALL)
	{
		fprintf(file, "\tcall\t_%s\n", instruction.arg1.c_str());
	}
	else if (instruction.op == TACOp::MOV)
	{
		Symbol *potential_var = symbolTable->find_symbol(instruction.arg1);
		Symbol *potential_var_2 = symbolTable->find_symbol(instruction.arg2);

		if (potential_var == nullptr && potential_var_2 == nullptr)
			fprintf(file, "\tmovl\t%s, %s\n", instruction.arg2.c_str(), instruction.arg1.c_str());
		else if (potential_var_2 != nullptr && potential_var == nullptr)
			fprintf(file, "\tmovl\t%d(%%rsp), %s\n", potential_var_2->stack_offset, instruction.arg1.c_str());
		else if (potential_var != nullptr && potential_var_2 == nullptr)
			fprintf(file, "\tmovl\t%s, %d(%%rsp)\n", instruction.arg2.c_str(), potential_var->stack_offset);
		else
			fprintf(file, "\tmovl\t%d(%%rsp), %d(%%rsp)\n", potential_var_2->stack_offset, potential_var->stack_offset);
	}
	else if (instruction.op == TACOp::NOP)
	{
		fprintf(file, "\n");
	}
}