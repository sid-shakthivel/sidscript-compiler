#include <cstdio>
#include <cerrno>
#include <iostream>
#include <functional>
#include <unordered_map>

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
}

void Assembler::assemble(const std::vector<TACInstruction> &instructions)
{
	fprintf(file, ".section __TEXT,__text,regular,pure_instructions\n");
	fprintf(file, ".build_version macos, 15, 0 sdk_version 15, 1\n");
	fprintf(file, ".p2align 4, 0x90\n\n");

	for (auto &instruction : instructions)
	{
		auto handler = handlers.find(instruction.op);
		if (handler != handlers.end())
		{
			handler->second(instruction);
		}
		else
		{
			fprintf(file, "# Unknown TAC operation: %d\n", static_cast<int>(instruction.op));
		}
	}
}

void Assembler::load_to_reg(const std::string &operand, const char *reg, Type type)
{
	Symbol *rhs = gst->get_symbol(current_func, operand);
	std::string mov_text = type == Type::LONG ? "movq" : "movl";

	std::string reg_name = reg;
	reg_name += type == Type::INT && reg != "%eax" ? "d" : "";

	if (rhs == nullptr)
		fprintf(file, "\t%s\t$%s, %s\n", mov_text.c_str(), operand.c_str(), reg_name.c_str());
	else
	{
		if (rhs->has_static_sd())
			fprintf(file, "\t%s\t_%s(%%rip), %s\n", mov_text.c_str(), rhs->name.c_str(), reg_name.c_str());
		else
			fprintf(file, "\t%s\t%d(%%rbp), %s\n", mov_text.c_str(), rhs->stack_offset, reg_name.c_str());
	}
}

void Assembler::store_from_reg(const std::string &operand, const char *reg, Type type)
{
	Symbol *rhs = gst->get_symbol(current_func, operand);
	std::string mov_text = type == Type::LONG ? "movq" : "movl";

	std::string reg_name = reg;
	reg_name += type == Type::INT ? "d" : "";

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
	Symbol *rhs = gst->get_symbol(current_func, operand);
	std::string op_text = type == Type::LONG ? op + "q" : op + "l";

	std::string reg_name = reg;
	reg_name += type == Type::INT ? "d" : "";

	if (rhs == nullptr)
		fprintf(file, "\t%s\t$%s, %s\n", op_text.c_str(), operand.c_str(), reg_name.c_str());
	else
		fprintf(file, "\t%s\t%d(%%rbp), %s\n", op_text.c_str(), rhs->stack_offset, reg_name.c_str());

	// fprintf(file, "\n");
}

void Assembler::compare_and_store_result(const std::string &operand_a, const std::string &operand_b, const std::string &result, const char *reg, const std::string &op, Type type)
{
	load_to_reg(operand_a, reg, type);

	Symbol *potential_var_b = gst->get_symbol(current_func, operand_b);
	std::string cmp_text = type == Type::LONG ? "cmpq" : "cmpl";

	std::string reg_name = reg;
	reg_name += type == Type::INT ? "d" : "";

	if (potential_var_b == nullptr)
		fprintf(file, "\t%s\t$%s, %s\n", cmp_text.c_str(), operand_b.c_str(), reg_name.c_str());
	else
		fprintf(file, "\t%s\t%d(%%rbp), %s\n", cmp_text.c_str(), potential_var_b->stack_offset, reg_name.c_str());

	std::string reg_b = reg_name;
	if (type == Type::INT)
		reg_b.pop_back();
	reg_b += "b";

	fprintf(file, "\t%s\t%s\n", op.c_str(), reg_b.c_str());
	fprintf(file, "\tmovzbl\t%s, %s\n", reg_b.c_str(), reg);

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
	int stack_space = gst->get_func_st(current_func)->get_var_count() * 8;
	fprintf(file, "\tsubq\t$%d, %%rsp\n\n", stack_space);
}

void Assembler::handle_func_end(TACInstruction &instruction)
{
	fprintf(file, "\n.L%s_end: # %s", current_func.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
	int stack_space = gst->get_func_st(current_func)->get_var_count() * 8;
	fprintf(file, "\taddq\t$%d, %%rsp\n", stack_space);
	fprintf(file, "\tpopq\t%%rbp\n");
	fprintf(file, "\tretq\n\n");
	current_func = "";
}

void Assembler::handle_assign(TACInstruction &instruction)
{
	Symbol *lhs = gst->get_symbol(current_func, instruction.arg1);
	Symbol *rhs = gst->get_symbol(current_func, instruction.arg2);
	std::string mov_text = instruction.type == Type::LONG ? "movq" : "movl";
	std::string reg = instruction.type == Type::LONG ? "%r10" : "%r10d";

	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());

	if (rhs)
	{
		if (lhs->has_static_sd() && rhs->has_static_sd())
		{
			fprintf(file, "\t%s\t_%s(%%rip), %s\n", mov_text.c_str(), instruction.arg2.c_str(), reg.c_str());
			fprintf(file, "\t%s\t%s, _%s(%%rip)\n", mov_text.c_str(), reg.c_str(), instruction.arg1.c_str());
		}
		else if (lhs->has_static_sd())
		{
			fprintf(file, "\t%s\t%d(%%rbp), %s\n", mov_text.c_str(), rhs->stack_offset, reg.c_str());
			fprintf(file, "\t%s\t%s, _%s(%%rip)\n", mov_text.c_str(), reg.c_str(), instruction.arg1.c_str());
		}
		else if (rhs->has_static_sd())
		{
			fprintf(file, "\t%s\t_%s(%%rip), %s\n", mov_text.c_str(), instruction.arg2.c_str(), reg.c_str());
			fprintf(file, "\t%s\t%s, %d(%%rbp)\n", mov_text.c_str(), reg.c_str(), lhs->stack_offset);
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
			fprintf(file, "\t%s\t$%s, _%s(%%rip) # %s\n", mov_text.c_str(), instruction.arg2.c_str(), instruction.arg1.c_str());
		else
			fprintf(file, "\t%s\t$%s, %d(%%rbp) # %s\n", mov_text.c_str(), instruction.arg2.c_str(), lhs->stack_offset);
	}

	fprintf(file, "\n");
}

void Assembler::handle_return(TACInstruction &instruction)
{
	std::string reg = instruction.type == Type::LONG ? "%rax" : "%eax";
	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
	load_to_reg(instruction.arg1, reg.c_str(), instruction.type);
	fprintf(file, "\tjmp\t.L%s_end\n", current_func.c_str());
}

void Assembler::handle_bin_op(TACInstruction &instruction, const std::string &op)
{
	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
	load_to_reg(instruction.arg1, "%r10", instruction.type);
	apply_bin_op_to_reg(instruction.arg2, "%r10", op, instruction.type);
	store_from_reg(instruction.result, "%r10", instruction.type);
	fprintf(file, "\n");
}

void Assembler::handle_cmp_op(TACInstruction &instruction, const std::string &op)
{
	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
	compare_and_store_result(instruction.arg1, instruction.arg2, instruction.result, "%r10", op, instruction.type);
}

void Assembler::handle_if(TACInstruction &instruction)
{
	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
	Symbol *potential_var = gst->get_symbol(current_func, instruction.arg1);
	std::string cmp_text = instruction.type == Type::LONG ? "cmpq" : "cmpl";

	if (potential_var == nullptr)
		fprintf(file, "\t%s\t$0, %s\n", cmp_text.c_str(), instruction.arg1.c_str());
	else
		fprintf(file, "\t%s\t$0, %d(%%rbp)\n", cmp_text.c_str(), potential_var->stack_offset);

	fprintf(file, "\tjne\t%s\n", instruction.result.c_str());
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
	std::string mov_text = instruction.type == Type::LONG ? "movq" : "movl";

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
	load_to_reg(instruction.arg1, "%rax", instruction.type);
	fprintf(file, "\tcqto\n"); // Sign-extend rax into rdx:rax
	load_to_reg(instruction.arg2, "%r10", instruction.type);
	fprintf(file, "\tidivq\t%%r10%s\n", instruction.type == Type::INT ? "d" : "");
	store_from_reg(instruction.result, "%rax", instruction.type); // rax for quotient
}

void Assembler::handle_mod(TACInstruction &instruction)
{
	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
	load_to_reg(instruction.arg1, "%rax", instruction.type);
	fprintf(file, "\tcqto\n"); // Sign-extend rax into rdx:rax
	load_to_reg(instruction.arg2, "%r10", instruction.type);
	fprintf(file, "\tidivq\t%%r10%s\n", instruction.type == Type::INT ? "d" : "");
	store_from_reg(instruction.result, "%rdx", instruction.type); // rdx for remainder
}

void Assembler::handle_unary_op(TACInstruction &instruction, const std::string &op)
{
	std::string op_text = instruction.type == Type::LONG ? op + "q" : op + "l";
	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
	load_to_reg(instruction.arg1, "%r10", instruction.type);
	fprintf(file, "\t%s\t%%r10%s\n", op_text.c_str(), instruction.type == Type::INT ? "d" : "");
	store_from_reg(instruction.result, "%r10", instruction.type);
}

void Assembler::handle_section(TACInstruction &instruction)
{
	switch (instruction.op)
	{
	case TACOp::ENTER_TEXT:
		fprintf(file, ".text\n");
		current_var_type = VarType::TEXT;
		break;
	case TACOp::ENTER_BSS:
		fprintf(file, ".bss\n");
		fprintf(file, ".balign 4\n\n");
		current_var_type = VarType::BSS;
		break;
	case TACOp::ENTER_DATA:
		fprintf(file, ".data\n");
		fprintf(file, ".balign 4\n\n");
		current_var_type = VarType::DATA;
		break;
	default:
		break;
	}
}