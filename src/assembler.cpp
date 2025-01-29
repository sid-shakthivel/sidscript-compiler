#include <cstdio>
#include <cerrno>
#include <iostream>
#include <functional>
#include <numeric>

#include "../include/assembler.h"
#include "../include/parser.h"
#include "../include/tacGenerator.h"

Assembler::Assembler(std::shared_ptr<GlobalSymbolTable> gst, std::string filename) : gst(gst)
{
	// Create new file for the program
	FILE *new_file = fopen(filename.c_str(), "w");
	if (new_file == NULL)
	{
		char errorMessage[256];
		snprintf(errorMessage, sizeof(errorMessage), "Error opening file %s", filename.c_str());
		perror(errorMessage);
		return;
	}

	file = new_file;

	load_to_reg = [this](const std::string &operand, const char *reg, Type type)
	{
		Symbol *rhs = this->gst->get_symbol(current_func, operand);
		std::string mov_text = "mov";
		mov_text += type == Type::LONG ? "q" : ";";

		if (rhs == nullptr)
			fprintf(this->file, "\t%s\t$%s, %s\n", mov_text.c_str(), operand.c_str(), reg);
		else
		{
			if (rhs->has_static_sd())
				fprintf(this->file, "\t%s\t_%s(%%rip), %s\n", mov_text.c_str(), rhs->name.c_str(), reg);
			else
				fprintf(this->file, "\t%s\t%d(%%rbp), %s\n", mov_text.c_str(), rhs->stack_offset, reg);
		}
	};

	store_from_reg = [this](const std::string &operand, const char *reg, Type type)
	{
		Symbol *rhs = this->gst->get_symbol(current_func, operand);

		std::string mov_text = "mov";
		mov_text += type == Type::LONG ? "q" : ";";

		if (rhs != nullptr)
		{
			if (rhs->has_static_sd())
				fprintf(file, "\t%s\t%s, _%s(%%rip)\n", mov_text.c_str(), reg, operand);
			else
				fprintf(file, "\t%s\t%s, %d(%%rbp)\n", mov_text.c_str(), reg, rhs->stack_offset);
		}
		else
			fprintf(file, "\t%s\t%s, $%s\n", mov_text.c_str(), reg, operand.c_str());
	};

	apply_bin_op_to_reg = [this](const std::string &operand, const char *reg, const std::string &op, Type type)
	{
		Symbol *rhs = this->gst->get_symbol(current_func, operand);

		std::string op_adj = op;

		if (type == Type::LONG)
		{
			op_adj.pop_back();
			op_adj += "q";
		}

		if (rhs == nullptr)
			fprintf(file, "\t%s\t$%s, %s\n", op_adj.c_str(), operand.c_str(), reg);
		else
			fprintf(file, "\t%s\t%d(%%rbp), %s\n", op_adj.c_str(), rhs->stack_offset, reg);

		fprintf(file, "\n");
	};

	compare_and_store_result = [this](const std::string &operand_a, const std::string &operand_b, const std::string &result, const char *reg, const std::string &op, Type type)
	{
		load_to_reg(operand_a, reg, type);

		Symbol *potential_var_b = this->gst->get_symbol(current_func, operand_b);

		std::string cmp_text = "cmp";
		cmp_text += type == Type::LONG? "q" : ";";

		if (potential_var_b == nullptr)
			fprintf(file, "\t%s\t$%s, %s\n", cmp_text.c_str(), operand_b.c_str(), reg);
		else
			fprintf(file, "\t%s\t%d(%%rbp), %s\n", cmp_text.c_str(), potential_var_b->stack_offset, reg);

		std::string reg_b = reg;
		reg_b.pop_back();
		reg_b += "b";

		fprintf(file, "\t%s\t%s\n", op.c_str(), reg_b.c_str());
		fprintf(file, "\tmovzbl	%s, %s\n", reg_b.c_str(), reg);

		store_from_reg(result, reg, type);

		fprintf(file, "\n");
	};
};

void Assembler::assemble(std::vector<TACInstruction> instructions)
{
	/*
		Specifies instructions are:
			- placed in text section of binary
			- normal and read-only
	*/
	fprintf(file, ".section	__TEXT,__text,regular,pure_instructions\n");

	// Specifies the version of the OS (macOS Sonoma) and the SDK version
	fprintf(file, ".build_version macos, 15, 0	sdk_version 15, 1\n");

	/*
		Aligns the _main function to a 16-byte boundary (2^4 = 16) for performance reasons
		The 0x90 specifies that the padding, if needed, will be filled with NOP (no-operation) instructions to ensure alignment
	*/
	fprintf(file, ".p2align	4, 0x90\n\n");

	for (auto &instruction : instructions)
		assemble_tac(instruction, file);
}

void Assembler::assemble_tac(TACInstruction &instruction, FILE *file)
{
	if (instruction.op == TACOp::FUNC_BEGIN)
	{
		current_func = instruction.arg1;

		if (instruction.arg2 == "global")
			fprintf(file, ".global	_%s\n", instruction.arg1.c_str());
		fprintf(file, "_%s: # %s\n", instruction.arg1.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
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
		int stack_space = gst->get_func_st(current_func)->get_var_count() * 8;
		fprintf(file, "\tsubq	$%d, %%rsp\n\n", stack_space);
	}
	else if (instruction.op == TACOp::FUNC_END)
	{
		fprintf(file, "\n.L%s_end: # %s", current_func.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
		int stack_space = gst->get_func_st(current_func)->get_var_count() * 8;
		fprintf(file, "\taddq	$%d, %%rsp\n", stack_space);
		fprintf(file, "\tpopq	%%rbp\n");
		fprintf(file, "\tretq\n\n");
		current_func = "";
	}
	else if (instruction.op == TACOp::ASSIGN)
	{
		if (current_var_type == VarType::BSS)
		{
			if (instruction.arg2 == "global")
				fprintf(file, ".global\t_%s\n", instruction.arg1.c_str());
			fprintf(file, "_%s:\n", instruction.arg1.c_str());

			if (instruction.type == Type::LONG)
				fprintf(file, "\t.zero 8\n\n");
			else if (instruction.type == Type::INT)
				fprintf(file, "\t.zero 4\n\n");
		}
		else if (current_var_type == VarType::DATA)
		{
			Symbol *var = gst->get_symbol(current_func, instruction.result);
			if (!var)
			{
				if (instruction.arg2 == "global")
					fprintf(file, ".global\t_%s\n", instruction.arg1.c_str());
				fprintf(file, "_%s:\n", instruction.arg1.c_str());

				if (instruction.type == Type::INT)
					fprintf(file, "\t.long %s\n\n", instruction.result.c_str());
				else if (instruction.type == Type::LONG)
					fprintf(file, "\t.quad %s\n\n", instruction.result.c_str());
			}
		}
		else if (current_var_type == VarType::TEXT)
		{
			Symbol *lhs = gst->get_symbol(current_func, instruction.arg1);
			Symbol *rhs = gst->get_symbol(current_func, instruction.arg2);

			std::string mov_text = "mov";
			mov_text += instruction.type == Type::LONG ? "q" : ";";

			if (rhs)
			{
				if (lhs->has_static_sd() && rhs->has_static_sd())
				{
					// mov_static_static
					fprintf(file, "\t%s\t_%s(%%rip), %%r10d # %s\n", mov_text.c_str(), instruction.arg1.c_str(), TacGenerator::gen_tac_str(instruction));
					fprintf(file, "\t%s\t%%r10d, _%s(%%rip)\n", mov_text.c_str(), instruction.arg2.c_str());
				}
				else if (lhs->has_static_sd() && !rhs->has_static_sd())
				{
					// mov_static_stack
					fprintf(file, "\t%s\t_%s(%%rip), %%r10d # %s\n", mov_text.c_str(), instruction.arg1.c_str(), TacGenerator::gen_tac_str(instruction));
					fprintf(file, "\t%s\t%%r10d, %d(%%rbp)\n", mov_text.c_str(), lhs->stack_offset);
				}
				else if (!lhs->has_static_sd() && rhs->has_static_sd())
				{
					// mov_stack_static
					fprintf(file, "\t%s\t_%s(%%rip), %%r10d # %s\n", mov_text.c_str(), instruction.arg2.c_str(), TacGenerator::gen_tac_str(instruction));
					fprintf(file, "\t%s\t%%r10d, %d(%%rbp)\n", mov_text.c_str(), rhs->stack_offset);
				}
				else if (!lhs->has_static_sd() && !rhs->has_static_sd())
				{
					// mov_stack_stack
					fprintf(file, "\t%s\t%d(%%rbp), %%r10d # %s\n", mov_text.c_str(), lhs->stack_offset, TacGenerator::gen_tac_str(instruction));
					fprintf(file, "\t%s\t%%r10d, %d(%%rbp)\n", mov_text.c_str(), rhs->stack_offset);
				}
			}
			else
			{
				if (lhs->has_static_sd())
					fprintf(file, "\t%s\t$%s, _%s(%%rip) # %s\n", mov_text.c_str(), instruction.arg2.c_str(), instruction.arg1.c_str(), TacGenerator::gen_tac_str(instruction));
				else
					fprintf(file, "\t%s\t$%s, %d(%%rbp) # %s\n", mov_text.c_str(), instruction.arg2.c_str(), lhs->stack_offset, TacGenerator::gen_tac_str(instruction));
			}
		}
	}
	else if (instruction.op == TACOp::RETURN)
	{
		fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
		load_to_reg(instruction.arg1, "%eax", instruction.type);
		fprintf(file, "\tjmp\t.L%s_end\n", current_func.c_str());
	}
	else if (instruction.op == TACOp::ADD)
	{
		fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
		load_to_reg(instruction.arg1, "%r10d", instruction.type);
		apply_bin_op_to_reg(instruction.arg2, "%r10d", "addl", instruction.type);
		store_from_reg(instruction.result, "%r10d", instruction.type);
	}
	else if (instruction.op == TACOp::SUB)
	{
		fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
		load_to_reg(instruction.arg1, "%r10d", instruction.type);
		apply_bin_op_to_reg(instruction.arg2, "%r10d", "subl", instruction.type);
		store_from_reg(instruction.result, "%r10d", instruction.type);
	}
	else if (instruction.op == TACOp::MUL)
	{
		fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
		load_to_reg(instruction.arg1, "%r10d", instruction.type);
		apply_bin_op_to_reg(instruction.arg2, "%r10d", "imull", instruction.type);
		store_from_reg(instruction.result, "%r10d", instruction.type);
	}
	else if (instruction.op == TACOp::DIV)
	{
		// Method will not work with Long
		fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
		load_to_reg(instruction.arg1, "%r10d", instruction.type);
		fprintf(file, "\tcltd\n");
		load_to_reg(instruction.arg2, "%r10d", instruction.type);
		fprintf(file, "\tidivl\t%%r10d\n");
		store_from_reg(instruction.result, "%eax", instruction.type); // eax for quotient
	}
	else if (instruction.op == TACOp::MOD)
	{
		// Method will not work with long
		fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
		load_to_reg(instruction.arg1, "%r10d", instruction.type);
		fprintf(file, "\tcltd\n");
		load_to_reg(instruction.arg2, "%r10d", instruction.type);
		fprintf(file, "\tidivl\t%%r10d\n");
		store_from_reg(instruction.result, "%edx", instruction.type); // eax for remainder
	}
	else if (instruction.op == TACOp::COMPLEMENT)
	{
		std::string notl_text = "notl";
		notl_text += instruction.type == Type::LONG ? "q" : "l";

		fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
		load_to_reg(instruction.arg1, "%r10d", instruction.type);
		fprintf(file, "\n%s\t%%r10d\n", notl_text.c_str());
		store_from_reg(instruction.result, "%r10d", instruction.type);
	}
	else if (instruction.op == TACOp::NEGATE)
	{
		std::string negl_text = "neg";
		negl_text += instruction.type == Type::LONG ? "q" : "l";

		fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
		load_to_reg(instruction.arg1, "%r10d", instruction.type);
		fprintf(file, "\n%s\t%%r10d\n", negl_text.c_str());
		store_from_reg(instruction.result, "%r10d", instruction.type);
	}
	else if (instruction.op == TACOp::LT)
	{
		fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
		compare_and_store_result(instruction.arg1, instruction.arg2, instruction.result, "%r10d", "setl", instruction.type);
	}
	else if (instruction.op == TACOp::LTE)
	{
		fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
		compare_and_store_result(instruction.arg1, instruction.arg2, instruction.result, "%r10d", "setle", instruction.type);
	}
	else if (instruction.op == TACOp::GT)
	{
		fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
		compare_and_store_result(instruction.arg1, instruction.arg2, instruction.result, "%r10d", "setg", instruction.type);
	}
	else if (instruction.op == TACOp::GTE)
	{
		fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
		compare_and_store_result(instruction.arg1, instruction.arg2, instruction.result, "%r10d", "setge", instruction.type);
	}
	else if (instruction.op == TACOp::EQUAL)
	{
		fprintf(file, "\n\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
		compare_and_store_result(instruction.arg1, instruction.arg2, instruction.result, "%r10d", "sete", instruction.type);
	}
	else if (instruction.op == TACOp::NOT_EQUAL)
	{
		fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
		compare_and_store_result(instruction.arg1, instruction.arg2, instruction.result, "%r10d", "setne", instruction.type);
	}
	else if (instruction.op == TACOp::IF)
	{
		fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
		Symbol *potential_var = gst->get_symbol(current_func, instruction.arg1);

		std::string cmp_text = "cmp";
		cmp_text += instruction.type == Type::LONG ? "q" : "l";

		if (potential_var == nullptr)
			fprintf(file, "\t%s\t$0, %s\n", cmp_text.c_str(), instruction.arg1.c_str());
		else
			fprintf(file, "\t%s\t$0, %d(%%rbp)\n", cmp_text.c_str(), potential_var->stack_offset);

		fprintf(file, "\t%s\t%s\n", "jne", instruction.result.c_str());
	}
	else if (instruction.op == TACOp::GOTO)
	{
		fprintf(file, "\tjmp\t%s # %s\n", instruction.result.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
	}
	else if (instruction.op == TACOp::LABEL)
	{
		fprintf(file, "\n%s: # %s\n", instruction.arg1.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
	}
	else if (instruction.op == TACOp::AND)
	{
		fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
		load_to_reg(instruction.arg1, "%r10d", instruction.type);
		apply_bin_op_to_reg(instruction.arg2, "%r10d", "andl", instruction.type);
		store_from_reg(instruction.result, "%r10d", instruction.type);
	}
	else if (instruction.op == TACOp::OR)
	{
		fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
		load_to_reg(instruction.arg1, "%r10d", instruction.type);
		apply_bin_op_to_reg(instruction.arg2, "%r10d", "orl", instruction.type);
		store_from_reg(instruction.result, "%r10d", instruction.type);
	}
	else if (instruction.op == TACOp::CALL)
	{
		fprintf(file, "\tcall\t_%s # %s\n", instruction.arg1.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
	}
	else if (instruction.op == TACOp::MOV)
	{
		// Does not support static variables yet
		fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
		Symbol *potential_var = gst->get_symbol(current_func, instruction.arg1);
		Symbol *potential_var_2 = gst->get_symbol(current_func, instruction.arg2);

		std::string mov_text = "mov";

		if (instruction.type == Type::LONG)
			mov_text += "q";
		else
			mov_text += "l";

		if (potential_var == nullptr && potential_var_2 == nullptr)
			fprintf(file, "\t%s\t%s, %s\n", mov_text.c_str(), instruction.arg2.c_str(), instruction.arg1.c_str());
		else if (potential_var_2 != nullptr && potential_var == nullptr)
			fprintf(file, "\t%s\t%d(%%rbp), %s\n", mov_text.c_str(), potential_var_2->stack_offset, instruction.arg1.c_str());
		else if (potential_var != nullptr && potential_var_2 == nullptr)
			fprintf(file, "\t%s\t%s, %d(%%rbp)\n", mov_text.c_str(), instruction.arg2.c_str(), potential_var->stack_offset);
		else
			fprintf(file, "\t%s\t%d(%%rbp), %d(%%rbp)\n", mov_text.c_str(), potential_var_2->stack_offset, potential_var->stack_offset);
	}
	else if (instruction.op == TACOp::NOP)
	{
		fprintf(file, "# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
	}
	else if (instruction.op == TACOp::ENTER_TEXT)
	{
		fprintf(file, ".text\n");
		current_var_type = VarType::TEXT;
	}
	else if (instruction.op == TACOp::ENTER_BSS)
	{
		fprintf(file, ".bss\n");
		fprintf(file, ".balign 4\n\n");
		current_var_type = VarType::BSS;
	}
	else if (instruction.op == TACOp::ENTER_DATA)
	{
		fprintf(file, ".data\n");
		fprintf(file, ".balign 4\n\n");
		current_var_type = VarType::DATA;
	}
}
