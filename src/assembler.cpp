#include <cstdio>
#include <cerrno>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <string>
#include <sstream>
#include <cstring>
#include <iomanip>

#include "../include/assembler.h"
#include "../include/parser.h"
#include "../include/tacGenerator.h"

Assembler::Assembler(std::shared_ptr<GlobalSymbolTable> gst, std::string filename) : gst(gst)
{
	file = fopen(filename.c_str(), "w");
	if (file == NULL)
	{
		char errorMessage[256];
		snprintf(errorMessage, sizeof(errorMessage), "Error opening file %s", filename.c_str());
		perror(errorMessage);
		return;
	}

	initialize_handlers();
}

void Assembler::initialize_handlers()
{
	handlers[TACOp::FUNC_BEGIN] = [this](TACInstruction instr)
	{ handle_func_begin(instr); };
	handlers[TACOp::FUNC_END] = [this](TACInstruction instr)
	{ handle_func_end(instr); };
	handlers[TACOp::ASSIGN] = [this](TACInstruction instr)
	{ handle_assign(instr); };
	handlers[TACOp::RETURN] = [this](TACInstruction instr)
	{ handle_return(instr); };
	handlers[TACOp::ADD] = [this](TACInstruction instr)
	{ handle_bin_op(instr, "add"); };
	handlers[TACOp::SUB] = [this](TACInstruction instr)
	{ handle_bin_op(instr, "sub"); };
	handlers[TACOp::MUL] = [this](TACInstruction instr)
	{ handle_bin_op(instr, "imul"); };
	handlers[TACOp::DIV] = [this](TACInstruction instr)
	{ handle_div(instr); };
	handlers[TACOp::MOD] = [this](TACInstruction instr)
	{ handle_mod(instr); };
	handlers[TACOp::COMPLEMENT] = [this](TACInstruction instr)
	{ handle_unary_op(instr, "not"); };
	handlers[TACOp::NEGATE] = [this](TACInstruction instr)
	{ handle_unary_op(instr, "neg"); };
	handlers[TACOp::LT] = [this](TACInstruction instr)
	{ handle_cmp_op(instr, "setl"); };
	handlers[TACOp::LTE] = [this](TACInstruction instr)
	{ handle_cmp_op(instr, "setle"); };
	handlers[TACOp::GT] = [this](TACInstruction instr)
	{ handle_cmp_op(instr, "setg"); };
	handlers[TACOp::GTE] = [this](TACInstruction instr)
	{ handle_cmp_op(instr, "setge"); };
	handlers[TACOp::EQUAL] = [this](TACInstruction instr)
	{ handle_cmp_op(instr, "sete"); };
	handlers[TACOp::NOT_EQUAL] = [this](TACInstruction instr)
	{ handle_cmp_op(instr, "setne"); };
	handlers[TACOp::IF] = [this](TACInstruction instr)
	{ handle_if(instr); };
	handlers[TACOp::GOTO] = [this](TACInstruction instr)
	{ handle_goto(instr); };
	handlers[TACOp::LABEL] = [this](TACInstruction instr)
	{ handle_label(instr); };
	handlers[TACOp::CALL] = [this](TACInstruction instr)
	{ handle_call(instr); };
	handlers[TACOp::MOV] = [this](TACInstruction instr)
	{ handle_mov(instr); };
	handlers[TACOp::NOP] = [this](TACInstruction instr)
	{ handle_nop(instr); };
	handlers[TACOp::ENTER_TEXT] = [this](TACInstruction instr)
	{ handle_section(instr); };
	handlers[TACOp::ENTER_BSS] = [this](TACInstruction instr)
	{ handle_section(instr); };
	handlers[TACOp::ENTER_DATA] = [this](TACInstruction instr)
	{ handle_section(instr); };
	handlers[TACOp::ENTER_LITERAL8] = [this](TACInstruction instr)
	{ handle_section(instr); };
	handlers[TACOp::ENTER_STR] = [this](TACInstruction instr)
	{ handle_section(instr); };
	handlers[TACOp::CONVERT_TYPE] = [this](TACInstruction instr)
	{ handle_convert_type(instr); };
	handlers[TACOp::ADDR_OF] = [this](TACInstruction instr)
	{ handle_addr_of(instr); };
	handlers[TACOp::DEREF] = [this](TACInstruction instr)
	{ handle_deref(instr); };
}

void Assembler::assemble(const std::vector<TACInstruction> &instructions)
{
	fprintf(file, ".section __TEXT,__text,regular,pure_instructions\n");
	fprintf(file, ".build_version macos, 15, 0 sdk_version 15, 1\n");
	fprintf(file, ".p2align 4, 0x90\n\n");

	for (const auto &instruction : instructions)
	{
		auto handler = handlers.find(instruction.op);
		if (handler != handlers.end())
			handler->second(instruction);
		else
			fprintf(file, "# Unknown TAC operation: %d\n", static_cast<int>(instruction.op));
	}
}

void Assembler::load_to_reg(const std::string &operand, const char *reg, Type type)
{
	Symbol *rhs = gst->get_symbol(current_func, operand);
	std::string mov_text = type.is_size_8() ? "movq" : "movl";

	std::string reg_name = reg;
	reg_name += !type.is_size_8() && strcmp(reg, "%eax") != 0 ? "d" : "";

	if (rhs == nullptr)
		fprintf(file, "\t%s\t$%s, %s\n", mov_text.c_str(), operand.c_str(), reg_name.c_str());
	else
	{
		if (rhs->has_static_sd() || rhs->is_literal8)
			fprintf(file, "\t%s\t_%s(%%rip), %s\n", mov_text.c_str(), rhs->name.c_str(), reg_name.c_str());
		else
			fprintf(file, "\t%s\t%d(%%rbp), %s\n", mov_text.c_str(), rhs->stack_offset, reg_name.c_str());
	}
}

void Assembler::store_from_reg(const std::string &operand, const char *reg, Type type)
{
	Symbol *rhs = gst->get_symbol(current_func, operand);
	std::string mov_text = type.is_size_8() ? "movq" : "movl";

	std::string reg_name = reg;
	reg_name += !type.is_size_8() && strcmp(reg, "%eax") != 0 ? "d" : "";

	if (rhs != nullptr)
	{
		if (rhs->has_static_sd())
			fprintf(file, "\t%s\t%s, _%s(%%rip)\n", mov_text.c_str(), reg_name.c_str(), operand.c_str());
		else
			fprintf(file, "\t%s\t%s, %d(%%rbp)\n", mov_text.c_str(), reg_name.c_str(), rhs->stack_offset);
	}
	else
		fprintf(file, "\t%s\t%s, $%s\n", mov_text.c_str(), reg_name.c_str(), operand.c_str());
}

void Assembler::apply_bin_op_to_reg(const std::string &operand, const char *reg, const std::string &op, Type type)
{
	if (type.has_base_type(BaseType::DOUBLE))
	{
		fprintf(file, "\t%s\t%s, %%xmm0\n", op.c_str(), reg);
		return;
	}

	Symbol *rhs = gst->get_symbol(current_func, operand);
	std::string op_text = type.is_size_8() ? op + "q" : op + "l";

	std::string reg_name = reg;
	reg_name += !type.is_size_8() && strcmp(reg, "%eax") != 0 ? "d" : "";

	if (rhs == nullptr)
		fprintf(file, "\t%s\t$%s, %s\n", op_text.c_str(), operand.c_str(), reg_name.c_str());
	else
		fprintf(file, "\t%s\t%d(%%rbp), %s\n", op_text.c_str(), rhs->stack_offset, reg_name.c_str());
}

void Assembler::compare_and_store_result(const std::string &operand_a, const std::string &operand_b, const std::string &result, const char *reg, const std::string &op, Type type)
{
	load_to_reg(operand_a, reg, type);

	Symbol *potential_var_b = gst->get_symbol(current_func, operand_b);
	std::string cmp_text = type.is_size_8() ? "cmpq" : "cmpl";

	std::string reg_name = reg;
	reg_name += !type.is_size_8() && strcmp(reg, "%eax") != 0 ? "d" : "";

	if (potential_var_b == nullptr)
		fprintf(file, "\t%s\t$%s, %s\n", cmp_text.c_str(), operand_b.c_str(), reg_name.c_str());
	else
		fprintf(file, "\t%s\t%d(%%rbp), %s\n", cmp_text.c_str(), potential_var_b->stack_offset, reg_name.c_str());

	std::string reg_b = reg_name;
	if (!type.is_size_8())
		reg_b.pop_back();
	reg_b += "b";

	fprintf(file, "\t%s\t%s\n", op.c_str(), reg_b.c_str());
	fprintf(file, "\tmovzbl\t%s, %s\n", reg_b.c_str(), reg_name.c_str());

	store_from_reg(result, reg, type);

	fprintf(file, "\n");
}

void Assembler::handle_func_begin(TACInstruction &instruction)
{
	current_func = instruction.arg1;
	if (instruction.arg2 == "global")
		fprintf(file, ".global _%s\n", instruction.arg1.c_str());
	fprintf(file, "_%s: # %s\n", instruction.arg1.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
	fprintf(file, "\tpushq\t%%rbp\n");
	fprintf(file, "\tmovq\t%%rsp, %%rbp\n");
	int stack_space = gst->get_func_st(current_func)->get_stack_size();
	fprintf(file, "\tsubq\t$%d, %%rsp\n\n", stack_space);
}

void Assembler::handle_func_end(TACInstruction &instruction)
{
	fprintf(file, "\n.L%s_end: # %s", current_func.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
	int stack_space = gst->get_func_st(current_func)->get_stack_size();
	fprintf(file, "\taddq\t$%d, %%rsp\n", stack_space);
	fprintf(file, "\tpopq\t%%rbp\n");
	fprintf(file, "\tretq\n\n");
	current_func = "";
}

void Assembler::handle_assign(TACInstruction &instruction)
{
	if (current_var_type == VarType::TEXT)
	{
		Symbol *lhs = gst->get_symbol(current_func, instruction.arg1);
		Symbol *rhs = gst->get_symbol(current_func, instruction.result);
		std::string mov_text = instruction.type.is_size_8() ? "movq" : "movl";
		mov_text = instruction.type.get_size() == 1 ? "movb" : mov_text;
		std::string reg = instruction.type.is_size_8() ? "%r10" : "%r10d";

		fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());

		if (lhs->type.is_array())
		{
			int stack_offset = lhs->stack_offset + std::stod(instruction.arg2) * instruction.type.get_size();
			fprintf(file, "\t%s\t$%s, %d(%%rbp)\n", mov_text.c_str(), instruction.result.c_str(), stack_offset);
			fprintf(file, "\n");
			return;

			// int stack_offset = lhs->stack_offset + (lhs->type.get_size() - 1 - std::stod(instruction.arg2)) * instruction.type.get_size();
			// fprintf(file, "\t%s\t$%s, %d(%%rbp)\n", mov_text.c_str(), instruction.result.c_str(), stack_offset);
			// fprintf(file, "\n");
			// return;
		}

		if (rhs)
		{
			if (rhs->type.is_pointer() && rhs->type.has_base_type(BaseType::CHAR))
			{
				fprintf(file, "\tleaq\t%s(%%rip), %s\n", instruction.arg2.c_str(), reg.c_str());
				fprintf(file, "\tmovq\t%s, %d(%%rbp)\n", reg.c_str(), lhs->stack_offset);
				return;
			}

			if (lhs->has_static_sd() && rhs->has_static_sd())
			{
				fprintf(file, "\t%s\t_%s(%%rip), %s\n", mov_text.c_str(), instruction.result.c_str(), reg.c_str());
				fprintf(file, "\t%s\t%s, _%s(%%rip)\n", mov_text.c_str(), reg.c_str(), instruction.arg1.c_str());
			}
			else if (lhs->has_static_sd())
			{
				fprintf(file, "\t%s\t%d(%%rbp), %s\n", mov_text.c_str(), rhs->stack_offset, reg.c_str());
				fprintf(file, "\t%s\t%s, _%s(%%rip)\n", mov_text.c_str(), reg.c_str(), instruction.arg1.c_str());
			}
			else if (rhs->has_static_sd())
			{
				fprintf(file, "\t%s\t_%s(%%rip), %s\n", mov_text.c_str(), instruction.result.c_str(), reg.c_str());
				fprintf(file, "\t%s\t%s, %d(%%rbp)\n", mov_text.c_str(), reg.c_str(), lhs->stack_offset);
			}
			else if (rhs->is_literal8)
			{
				fprintf(file, "\t%s\t_%s(%%rip), %%xmm0\n", mov_text.c_str(), rhs->name.c_str());
				fprintf(file, "\t%s\t%%xmm0, %d(%%rbp)\n", mov_text.c_str(), lhs->stack_offset);
			}
			else if (rhs->type.is_array())
			{
				if (instruction.arg2.empty())
				{
					// Array-to-pointer decay: get address of array start
					fprintf(file, "\tleaq\t%d(%%rbp), %s\n", rhs->stack_offset, reg.c_str());
					fprintf(file, "\tmovq\t%s, %d(%%rbp)\n", reg.c_str(), lhs->stack_offset);
				}
				else
				{
					// Array element access: calculate offset and load value
					int stack_offset = rhs->stack_offset + std::stod(instruction.arg2) * instruction.type.get_size();
					fprintf(file, "\t%s\t%d(%%rbp), %s\n", mov_text.c_str(), stack_offset, reg.c_str());
					fprintf(file, "\t%s\t%s, %d(%%rbp)\n", mov_text.c_str(), reg.c_str(), lhs->stack_offset);
				}
			}
			else
			{
				fprintf(file, "\t%s\t%d(%%rbp), %s\n", mov_text.c_str(), rhs->stack_offset, reg.c_str());
				fprintf(file, "\t%s\t%s, %d(%%rbp)\n", mov_text.c_str(), reg.c_str(), lhs->stack_offset);
			}
		}
		else
		{
			if (lhs->has_static_sd())
				fprintf(file, "\t%s\t$%s, _%s(%%rip)\n", mov_text.c_str(), instruction.result.c_str(), instruction.arg1.c_str());
			else
				fprintf(file, "\t%s\t$%s, %d(%%rbp)\n", mov_text.c_str(), instruction.result.c_str(), lhs->stack_offset);
		}

		fprintf(file, "\n");
	}
	else if (current_var_type == VarType::BSS)
	{
		if (instruction.arg3 == "global")
			fprintf(file, "\t.global\t_%s\n", instruction.arg1.c_str());
		fprintf(file, "_%s:\n", instruction.arg1.c_str());
		if (instruction.type.is_size_8())
			fprintf(file, "\t.zero 8\n\n");
		else
			fprintf(file, "\t.zero 4\n\n");
	}
	else if (current_var_type == VarType::DATA)
	{
		Symbol *potential_var = gst->get_symbol(current_func, instruction.result);

		if (potential_var == nullptr)
		{
			if (instruction.arg3 == "global")
				fprintf(file, ".global	_%s\n", instruction.arg1.c_str());
			fprintf(file, "_%s:\n", instruction.arg1.c_str());

			if (instruction.type.is_size_8())
				fprintf(file, "\t.quad %s\n\n", instruction.result.c_str());
			else
				fprintf(file, "\t.long %s\n\n", instruction.result.c_str());
		}
	}
	else if (current_var_type == VarType::LITERAL8)
	{
		if (!instruction.type.has_base_type(BaseType::DOUBLE))
			return;

		fprintf(file, "_%s:\n", instruction.arg1.c_str());

		double value = std::stod(instruction.result);
		std::string double_hex = double_to_hex(value);

		fprintf(file, "\t.quad %s # %s\n\n", double_hex.c_str(), instruction.result.c_str());
	}
	else if (current_var_type == VarType::STR)
	{
		if (!instruction.type.has_base_type(BaseType::CHAR))
			return;

		fprintf(file, "_%s:\n", instruction.arg1.c_str());
		fprintf(file, "\t.asciz \"%s\"\n\n", instruction.result.c_str());
	}
}

void Assembler::handle_return(TACInstruction &instruction)
{
	std::string reg = instruction.type.is_size_8() ? "%rax" : "%eax";
	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
	load_to_reg(instruction.arg1, reg.c_str(), instruction.type);
	fprintf(file, "\tjmp\t.L%s_end\n", current_func.c_str());
}

void Assembler::handle_bin_op(TACInstruction &instruction, const std::string &op)
{
	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());

	load_to_reg(instruction.arg1, instruction.type.has_base_type(BaseType::DOUBLE) ? "%xmm0" : "%r10", instruction.type);

	if (instruction.type.has_base_type(BaseType::DOUBLE))
		load_to_reg(instruction.arg2, "%xmm1", instruction.type);

	std::string actual_op = op;
	// if (op == "imul" && !instruction.type.is_signed())
	// 	actual_op = "mul"; // unsigned multiplication

	if (instruction.type.has_base_type(BaseType::DOUBLE))
		actual_op = op + "sd";

	apply_bin_op_to_reg(instruction.arg2, instruction.type.has_base_type(BaseType::DOUBLE) ? "%xmm1" : "%r10", actual_op, instruction.type);
	store_from_reg(instruction.result, instruction.type.has_base_type(BaseType::DOUBLE) ? "%xmm0" : "%r10", instruction.type);
	fprintf(file, "\n");
}

void Assembler::handle_cmp_op(TACInstruction &instruction, const std::string &op)
{
	std::string actual_op = op;
	if (!instruction.type.is_signed())
	{
		if (op == "setl")
			actual_op = "setb"; // below
		else if (op == "setle")
			actual_op = "setbe"; // below or equal
		else if (op == "setg")
			actual_op = "seta"; // above
		else if (op == "setge")
			actual_op = "setae"; // above or equal
	}

	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());

	if (instruction.type.has_base_type(BaseType::DOUBLE))
	{
		load_to_reg(instruction.arg1, "%xmm0", instruction.type);
		load_to_reg(instruction.arg2, "%xmm1", instruction.type);

		fprintf(file, "\tcomisd\t%%xmm1, %%xmm0\n");

		fprintf(file, "\tsetb\t%%r10b\n");
		fprintf(file, "\tmovzbl\t%%r10b, %%r10d\n");

		store_from_reg(instruction.result, "%r10", instruction.type);

		fprintf(file, "\n");

		return;
	}

	compare_and_store_result(instruction.arg1, instruction.arg2, instruction.result,
							 "%r10", actual_op, instruction.type);
}

void Assembler::handle_if(TACInstruction &instruction)
{
	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
	Symbol *potential_var = gst->get_symbol(current_func, instruction.arg1);
	std::string cmp_text = instruction.type.is_size_8() ? "cmpq" : "cmpl";

	if (potential_var == nullptr)
		fprintf(file, "\t%s\t$0, %s\n", cmp_text.c_str(), instruction.arg1.c_str());
	else
		fprintf(file, "\t%s\t$0, %d(%%rbp)\n", cmp_text.c_str(), potential_var->stack_offset);

	if (instruction.type.has_base_type(BaseType::DOUBLE))
		fprintf(file, "\tjb\t%s\n", instruction.result.c_str());
	else if (!instruction.type.is_signed())
		fprintf(file, "\tjne\t%s\n", instruction.result.c_str());
	else
		fprintf(file, "\tjg\t%s\n", instruction.result.c_str());
}

void Assembler::handle_goto(TACInstruction &instruction)
{
	fprintf(file, "\tjmp\t%s # %s\n", instruction.result.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
}

void Assembler::handle_label(TACInstruction &instruction)
{
	fprintf(file, "\n%s: # %s\n", instruction.arg1.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
}

void Assembler::handle_call(TACInstruction &instruction)
{
	fprintf(file, "\tcall\t_%s # %s\n", instruction.arg1.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
}

void Assembler::handle_mov(TACInstruction &instruction)
{
	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
	Symbol *potential_var = gst->get_symbol(current_func, instruction.arg1);
	Symbol *potential_var_2 = gst->get_symbol(current_func, instruction.arg2);
	std::string mov_text = instruction.type.is_size_8() ? "movq" : "movl";

	if (potential_var == nullptr && potential_var_2 == nullptr)
		fprintf(file, "\t%s\t%s, %s\n", mov_text.c_str(), instruction.arg2.c_str(), instruction.arg1.c_str());
	else if (potential_var_2 != nullptr && potential_var == nullptr)
		fprintf(file, "\t%s\t%d(%%rbp), %s\n", mov_text.c_str(), potential_var_2->stack_offset, instruction.arg1.c_str());
	else if (potential_var != nullptr && potential_var_2 == nullptr)
		fprintf(file, "\t%s\t%s, %d(%%rbp)\n", mov_text.c_str(), instruction.arg2.c_str(), potential_var->stack_offset);
	else
		fprintf(file, "\t%s\t%d(%%rbp), %d(%%rbp)\n", mov_text.c_str(), potential_var_2->stack_offset, potential_var->stack_offset);
}

void Assembler::handle_nop(TACInstruction &instruction)
{
	fprintf(file, "# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
}

void Assembler::handle_div(TACInstruction &instruction)
{
	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());

	if (instruction.type.has_base_type(BaseType::DOUBLE))
	{
		load_to_reg(instruction.arg1, "%xmm0", instruction.type);
		load_to_reg(instruction.arg2, "%xmm1", instruction.type);
		fprintf(file, "\tdivsd %%xmm1, %%xmm0\n");
		store_from_reg(instruction.result, "%xmm0", instruction.type);

		fprintf(file, "\n");
		return;
	}

	load_to_reg(instruction.arg1, "%rax", instruction.type);

	if (instruction.type.is_signed())
		fprintf(file, "\t%s\n", instruction.type.is_size_8() ? "cqto" : "cdq"); // Sign-extend for signed division
	else
		fprintf(file, "\txor\t%%rdx, %%rdx\n"); // Clear rdx for unsigned division

	load_to_reg(instruction.arg2, "%r10", instruction.type);
	fprintf(file, "\t%s\t%%r10%s\n",
			instruction.type.is_signed() ? "idiv" : "div",
			instruction.type.has_base_type(BaseType::INT) ? "d" : "");
	store_from_reg(instruction.result, "%rax", instruction.type);
}

void Assembler::handle_mod(TACInstruction &instruction)
{
	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
	load_to_reg(instruction.arg1, "%rax", instruction.type);

	if (instruction.type.is_signed())
		fprintf(file, "\t%s\n", instruction.type.is_size_8() ? "cqto" : "cdq");
	else
		fprintf(file, "\txor\t%%rdx, %%rdx\n");

	load_to_reg(instruction.arg2, "%r10", instruction.type);
	fprintf(file, "\t%s\t%%r10%s\n",
			instruction.type.is_signed() ? "idiv" : "div",
			instruction.type.has_base_type(BaseType::INT) ? "d" : "");
	store_from_reg(instruction.result, "%rdx", instruction.type);
}

void Assembler::handle_unary_op(TACInstruction &instruction, const std::string &op)
{
	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());

	if (instruction.type.has_base_type(BaseType::DOUBLE) && op == "neg")
	{
		load_to_reg(instruction.arg1, "%xmm0", instruction.type);
		load_to_reg(instruction.arg2, "%xmm1", instruction.type);

		fprintf(file, "\txorpd\t%%xmm1, %%xmm0\n");

		store_from_reg(instruction.result, "%xmm0", instruction.type);

		fprintf(file, "\n");
		return;
	}

	std::string op_text = instruction.type.is_size_8() ? op + "q" : op + "l";
	load_to_reg(instruction.arg1, "%r10", instruction.type);
	fprintf(file, "\t%s\t%%r10%s\n", op_text.c_str(), instruction.type.has_base_type(BaseType::INT) ? "d" : "");
	store_from_reg(instruction.result, "%r10", instruction.type);
}

void Assembler::handle_section(TACInstruction &instruction)
{
	/*
		Use the largest alignment needed for variables in a section
		So for now use 8
	*/
	switch (instruction.op)
	{
	case TACOp::ENTER_TEXT:
	{
		fprintf(file, ".text\n");
		current_var_type = VarType::TEXT;
		break;
	}
	case TACOp::ENTER_BSS:
	{
		fprintf(file, ".bss\n");
		fprintf(file, ".balign 8\n\n");
		current_var_type = VarType::BSS;
		break;
	}
	case TACOp::ENTER_DATA:
	{
		fprintf(file, ".data\n");
		fprintf(file, ".balign 8\n\n");
		current_var_type = VarType::DATA;
		break;
	}
	case TACOp::ENTER_LITERAL8:
	{
		fprintf(file, ".section __TEXT,__literal8,8byte_literals\n");
		current_var_type = VarType::LITERAL8;
		break;
	}
	case TACOp::ENTER_STR:
	{
		fprintf(file, ".section __TEXT,__cstring,cstring_literals\n");
		current_var_type = VarType::STR;
		break;
	}
	default:
		break;
	}
}

void Assembler::handle_convert_type(TACInstruction &instruction)
{
	Symbol *src = gst->get_symbol(current_func, instruction.arg1);
	Symbol *dst = gst->get_symbol(current_func, instruction.result);

	printf("%s %s\n", instruction.arg1.c_str(), instruction.arg2.c_str());

	Type src_type = instruction.type;
	Type dst_type = get_type_from_str(instruction.arg2);

	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());

	// int -> long (sign extend)
	if (src_type.has_base_type(BaseType::INT) && dst_type.has_base_type(BaseType::LONG))
	{
		fprintf(file, "\tmovl %d(%%rbp), %%r10d\n", src->stack_offset);
		fprintf(file, "\tmovslq %%r10d, %%r10\n");
		fprintf(file, "\tmovq %%r10, %d(%%rbp)\n", dst->stack_offset);
	}
	// uint -> ulong (zero extend)
	else if (src_type.has_base_type(BaseType::UINT) && dst_type.has_base_type(BaseType::ULONG))
	{
		fprintf(file, "\tmovl %d(%%rbp), %%r10d\n", src->stack_offset);
		fprintf(file, "\tmovzxd %%r10d, %%r10\n"); // zero extend using movzx
		fprintf(file, "\tmovq %%r10, %d(%%rbp)\n", dst->stack_offset);
	}
	// long/ulong -> int/uint (truncate)
	else if ((src_type.has_base_type(BaseType::LONG) || src_type.has_base_type(BaseType::ULONG)) &&
			 (dst_type.has_base_type(BaseType::INT) || dst_type.has_base_type(BaseType::UINT)))
	{
		fprintf(file, "\tmovq %d(%%rbp), %%r10\n", src->stack_offset);
		fprintf(file, "\tmovl %%r10d, %d(%%rbp)\n", dst->stack_offset);
		if (dst_type.has_base_type(BaseType::UINT))
			fprintf(file, "\tandl $0xFFFFFFFF, %d(%%rbp)\n", dst->stack_offset);
	}
	// int <-> uint (reinterpret, but mask for uint)
	else if ((src_type.has_base_type(BaseType::INT) && dst_type.has_base_type(BaseType::UINT)) ||
			 (src_type.has_base_type(BaseType::UINT) && dst_type.has_base_type(BaseType::INT)))
	{
		fprintf(file, "\tmovl %d(%%rbp), %%r10d\n", src->stack_offset);
		fprintf(file, "\tmovl %%r10d, %d(%%rbp)\n", dst->stack_offset);
		if (dst_type.has_base_type(BaseType::UINT))
			fprintf(file, "\tandl $0xFFFFFFFF, %d(%%rbp)\n", dst->stack_offset);
	}
	// double -> int
	else if (src_type.has_base_type(BaseType::DOUBLE) && dst_type.has_base_type(BaseType::INT))
	{
		fprintf(file, "\tmovsd %d(%%rbp), %%xmm0\n", src->stack_offset);
		fprintf(file, "\tcvttsd2si %%xmm0, %%r10d\n"); // truncate double to signed int
		fprintf(file, "\tmovl %%r10d, %d(%%rbp)\n", dst->stack_offset);
	}
	// double -> uint
	else if (src_type.has_base_type(BaseType::DOUBLE) && dst_type.has_base_type(BaseType::UINT))
	{
		fprintf(file, "\tmovsd %d(%%rbp), %%xmm0\n", src->stack_offset);
		fprintf(file, "\tcvttsd2si %%xmm0, %%r10d\n"); // truncate double to signed int
		fprintf(file, "\tmovl %%r10d, %d(%%rbp)\n", dst->stack_offset);
		fprintf(file, "\tandl $0xFFFFFFFF, %d(%%rbp)\n", dst->stack_offset);
	}
	// int -> double
	else if (src_type.has_base_type(BaseType::INT) && dst_type.has_base_type(BaseType::DOUBLE))
	{
		fprintf(file, "\tmovl %d(%%rbp), %%r10d\n", src->stack_offset);
		fprintf(file, "\tcvtsi2sd %%r10d, %%xmm0\n"); // convert signed int to double
		fprintf(file, "\tmovsd %%xmm0, %d(%%rbp)\n", dst->stack_offset);
	}
	// uint -> double
	else if (src_type.has_base_type(BaseType::UINT) && dst_type.has_base_type(BaseType::DOUBLE))
	{
		fprintf(file, "\tmovl %d(%%rbp), %%r10d\n", src->stack_offset);
		fprintf(file, "\tmovl %%r10d, %%r10d\n");	 // zero extend to 64 bits
		fprintf(file, "\tcvtsi2sd %%r10, %%xmm0\n"); // convert unsigned int to double
		fprintf(file, "\tmovsd %%xmm0, %d(%%rbp)\n", dst->stack_offset);
	}

	fprintf(file, "\n");
}

void Assembler::handle_deref(TACInstruction &instruction)
{
	Symbol *src = gst->get_symbol(current_func, instruction.arg1);
	Symbol *dst = gst->get_symbol(current_func, instruction.result);

	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());

	// fprintf(file, "\tmovq %d(%%rbp), %%rax\n", src->stack_offset);
	// fprintf(file, "\tmovl (%%rax), %%r10d\n");
	// fprintf(file, "\tmovl %%r10d, %d(%%rbp)\n\n", dst->stack_offset);

	// First, get the pointer value into a register
	fprintf(file, "\tmovq\t%d(%%rbp), %%rax\n", src->stack_offset);

	// Now dereference it and store the value
	fprintf(file, "\tmovl\t(%%rax), %%r10d\n");
	fprintf(file, "\tmovl\t%%r10d, %d(%%rbp)\n", dst->stack_offset);

	fprintf(file, "\n");
}

void Assembler::handle_addr_of(TACInstruction &instruction)
{
	Symbol *src = gst->get_symbol(current_func, instruction.arg1);
	Symbol *dst = gst->get_symbol(current_func, instruction.result);

	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());

	fprintf(file, "\tleaq %d(%%rbp), %%rax\n", src->stack_offset);
	fprintf(file, "\tmovq %%rax, %d(%%rbp)\n\n", dst->stack_offset);
}

std::string Assembler::double_to_hex(double value)
{
	union
	{
		double d;
		uint64_t i;
	} converter;

	converter.d = value;

	std::stringstream ss;
	ss << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(16) << converter.i;

	return ss.str();
}