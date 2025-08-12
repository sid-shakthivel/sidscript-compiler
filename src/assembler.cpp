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

#define REGISTER_HANDLER(op, fn) handlers[TACOp::op] = [this](TACInstruction instr) { fn(instr); }

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

	REGISTER_HANDLER(FUNC_BEGIN, handle_func_begin);
	REGISTER_HANDLER(FUNC_END, handle_func_end);
	REGISTER_HANDLER(ASSIGN, handle_assign);
	REGISTER_HANDLER(RETURN, handle_return);
	REGISTER_HANDLER(ADD, [&](TACInstruction instr)
					 { handle_bin_op(instr, "add"); });
	REGISTER_HANDLER(SUB, [&](TACInstruction instr)
					 { handle_bin_op(instr, "sub"); });
	REGISTER_HANDLER(MUL, [&](TACInstruction instr)
					 { handle_bin_op(instr, "imul"); });
	REGISTER_HANDLER(DIV, handle_div);
	REGISTER_HANDLER(MOD, handle_mod);
	REGISTER_HANDLER(COMPLEMENT, [&](TACInstruction instr)
					 { handle_unary_op(instr, "not"); });
	REGISTER_HANDLER(NEGATE, [&](TACInstruction instr)
					 { handle_unary_op(instr, "neg"); });
	REGISTER_HANDLER(LT, [&](TACInstruction instr)
					 { handle_cmp_op(instr, "setl"); });
	REGISTER_HANDLER(LTE, [&](TACInstruction instr)
					 { handle_cmp_op(instr, "setle"); });
	REGISTER_HANDLER(GT, [&](TACInstruction instr)
					 { handle_cmp_op(instr, "setg"); });
	REGISTER_HANDLER(GTE, [&](TACInstruction instr)
					 { handle_cmp_op(instr, "setge"); });
	REGISTER_HANDLER(EQUAL, [&](TACInstruction instr)
					 { handle_cmp_op(instr, "sete"); });
	REGISTER_HANDLER(NOT_EQUAL, [&](TACInstruction instr)
					 { handle_cmp_op(instr, "sene"); });
	REGISTER_HANDLER(IF, handle_if);
	REGISTER_HANDLER(GOTO, handle_goto);
	REGISTER_HANDLER(LABEL, handle_label);
	REGISTER_HANDLER(CALL, handle_call);
	REGISTER_HANDLER(MOV, handle_mov);
	REGISTER_HANDLER(NOP, handle_nop);
	REGISTER_HANDLER(ENTER_TEXT, handle_section);
	REGISTER_HANDLER(ENTER_BSS, handle_section);
	REGISTER_HANDLER(ENTER_DATA, handle_section);
	REGISTER_HANDLER(ENTER_LITERAL8, handle_section);
	REGISTER_HANDLER(ENTER_STR, handle_section);
	REGISTER_HANDLER(CONVERT_TYPE, handle_convert_type);
	REGISTER_HANDLER(ADDR_OF, handle_addr_of);
	REGISTER_HANDLER(DEREF, handle_deref);
	REGISTER_HANDLER(PRINTF, handle_printf);
	REGISTER_HANDLER(STRUCT_INIT, handle_struct_init);
	REGISTER_HANDLER(MEMBER_ASSIGN, handle_member_assign);
	REGISTER_HANDLER(MEMBER_ACCESS, handle_member_access);
	REGISTER_HANDLER(NOT, handle_not);
	REGISTER_HANDLER(AND, handle_log_and);
	REGISTER_HANDLER(OR, handle_log_or);
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

void Assembler::load_to_reg(const std::string &operand, const char *reg, Type type, std::string arg2)
{
	Symbol *sym = gst->get_symbol(operand);
	std::string mov = get_mov_instruction(type);
	std::string reg_name = get_reg_name(reg, type);

	if (!sym)
	{
		fprintf(file, "\t%s\t%s, %s\n", mov.c_str(), format_memory_ref(operand).c_str(), reg_name.c_str());
		return;
	}

	// Case: src is a char pointer (e.g., char *str = "Hello")
	if (sym->type.has_base_type(BaseType::CHAR) && (sym->type.is_pointer() || sym->type.is_array()))
	{
		fprintf(file, "\tleaq\t_%s(%%rip), %s\n", operand.c_str(), reg_name.c_str());
		return;
	}

	if (sym->type.is_array())
	{
		if (arg2.empty())
		{
			/*
				Case: array-to-pointer decay (get address of array start)
				(e.g, int* p = array;)
			*/
			fprintf(file, "\tleaq\t%d(%%rbp), %s\n", sym->stack_offset, reg_name.c_str());
		}
		else
		{
			/*
				Case: accessing a specific array element
				(e.g., int x = array[2];)
			*/
			int stack_offset = sym->stack_offset + std::stod(arg2) * type.get_size();
			fprintf(file, "\t%s\t%d(%%rbp), %s\n", mov.c_str(), stack_offset, reg_name.c_str());
		}

		return;
	}

	fprintf(file, "\t%s\t%s, %s\n", mov.c_str(), format_memory_ref(operand).c_str(), reg_name.c_str());
}

/*
	The following function is used to store a value from a register to various different memory locations
	(e.g., a variable, an array element, etc.)
*/
void Assembler::store_from_reg(const std::string &operand, const char *reg, Type type, std::string arg2)
{
	Symbol *sym = gst->get_symbol(operand);
	std::string mov = get_mov_instruction(type);
	std::string reg_name = get_reg_name(reg, type);

	if (!sym)
		error("Invalid symbol?: " + operand);

	// Case: dst is an array (e.g., arr[i] = ...)
	if (sym->type.is_array())
	{
		int stack_offset = sym->stack_offset + std::stod(arg2) * type.get_size();
		fprintf(file, "\t%s\t%s, %d(%%rbp)\n", mov.c_str(), reg_name.c_str(), stack_offset);
		return;
	}

	// Case: dst is a variable (of any sort i.e. static, local, etc)
	fprintf(file, "\t%s\t%s, %s\n",
			mov.c_str(),
			reg_name.c_str(),
			format_memory_ref(operand).c_str());
}

void Assembler::compare_and_store_result(const std::string &operand_a, const std::string &operand_b, const std::string &result, const char *reg, const std::string &op, Type type)
{
	load_to_reg(operand_a, reg, type);

	Symbol *potential_var_b = gst->get_symbol(operand_b);
	std::string cmp_text = get_cmp_instruction(type);

	std::string reg_name = get_reg_name(reg, type);

	if (potential_var_b == nullptr)
		fprintf(file, "\t%s\t$%s, %s\n", cmp_text.c_str(), operand_b.c_str(), reg_name.c_str());
	else
		fprintf(file, "\t%s\t%d(%%rbp), %s\n", cmp_text.c_str(), potential_var_b->stack_offset, reg_name.c_str());

	std::string reg_b = reg_name;
	if (!type.is_size_8())
		reg_b.pop_back();
	reg_b += "b";

	fprintf(file, "\t%s\t%s\n", op.c_str(), reg_b.c_str());

	if (reg_b != reg_name)
		fprintf(file, "\tmovzbl\t%s, %s\n", reg_b.c_str(), reg_name.c_str());

	store_from_reg(result, reg, type);

	fprintf(file, "\n");
}

void Assembler::handle_func_begin(TACInstruction &instruction)
{
	gst->enter_func_scope(instruction.arg1);
	if (instruction.arg2 == "global")
		fprintf(file, ".global _%s\n", instruction.arg1.c_str());
	fprintf(file, ".extern _printf\n");
	fprintf(file, "_%s: # %s\n", instruction.arg1.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
	fprintf(file, "\tpushq\t%%rbp\n");
	fprintf(file, "\tmovq\t%%rsp, %%rbp\n");
	int stack_space = gst->get_func_st(gst->get_current_func())->get_stack_size();
	fprintf(file, "\tsubq\t$%d, %%rsp\n\n", stack_space);
}

void Assembler::handle_func_end(TACInstruction &instruction)
{
	std::string current_func = gst->get_current_func();
	fprintf(file, ".L%s_end: # %s", current_func.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
	int stack_space = gst->get_func_st(current_func)->get_stack_size();
	fprintf(file, "\taddq\t$%d, %%rsp\n", stack_space);
	fprintf(file, "\tpopq\t%%rbp\n");
	fprintf(file, "\tretq\n\n");
	gst->leave_func_scope();
}

void Assembler::handle_assign(TACInstruction &instruction)
{
	switch (current_var_type)
	{
	case VarType::TEXT:
		return handle_text_assign(instruction);
	case VarType::BSS:
		return handle_bss_assign(instruction);
	case VarType::DATA:
		return handle_data_assign(instruction);
	case VarType::LITERAL8:
		return handle_literal8_assign(instruction);
	case VarType::STR:
		return handle_str_assign(instruction);
	}
}

void Assembler::handle_text_assign(TACInstruction &instruction)
{
	comment_instruction(instruction);

	Symbol *dst = gst->get_symbol(instruction.arg1);
	Symbol *src = gst->get_symbol(instruction.result);

	load_to_reg(instruction.result, "%r10", instruction.type, instruction.arg2);

	store_from_reg(instruction.arg1, "%r10", instruction.type, instruction.arg2);

	fprintf(file, "\n");
}

void Assembler::handle_bss_assign(TACInstruction &instruction)
{
	if (instruction.arg3 == "global")
		fprintf(file, "\t.global\t_%s\n", instruction.arg1.c_str());
	fprintf(file, "_%s:\n", instruction.arg1.c_str());
	fprintf(file, "\t.zero %zu\n\n", instruction.type.get_size());
}

void Assembler::handle_data_assign(TACInstruction &instruction)
{
	Symbol *potential_var = gst->get_symbol(instruction.result);

	if (potential_var != nullptr)
		return;

	if (instruction.arg3 == "global")
		fprintf(file, ".global	_%s\n", instruction.arg1.c_str());

	fprintf(file, "_%s:\n", instruction.arg1.c_str());
	fprintf(file, "\t.%s %s\n\n", instruction.type.is_size_8() ? "quad" : "long", instruction.result.c_str());
}

void Assembler::handle_literal8_assign(TACInstruction &instruction)
{
	if (!instruction.type.has_base_type(BaseType::DOUBLE))
		return;

	fprintf(file, "_%s:\n", instruction.arg1.c_str());

	double value = std::stod(instruction.result);
	std::string double_hex = double_to_hex(value);

	fprintf(file, "\t.quad %s # %s\n\n", double_hex.c_str(), instruction.result.c_str());
}

void Assembler::handle_str_assign(TACInstruction &instruction)
{
	if (!instruction.type.has_base_type(BaseType::CHAR))
		return;

	fprintf(file, "_%s:\n", instruction.arg1.c_str());
	fprintf(file, "\t.asciz \"%s\"\n\n", instruction.result.c_str());
}

void Assembler::handle_return(TACInstruction &instruction)
{
	comment_instruction(instruction);

	std::string reg = get_reg_name("%rax", instruction.type);

	// Optionally load the return value into rax/eax
	if (instruction.arg1 != "")
		load_to_reg(instruction.arg1, reg.c_str(), instruction.type);

	fprintf(file, "\tjmp\t.L%s_end\n\n", gst->get_current_func().c_str());
}

void Assembler::handle_bin_op(TACInstruction &instruction, const std::string &op)
{
	comment_instruction(instruction);

	std::string reg = get_reg_name("%r10", instruction.type);

	load_to_reg(instruction.arg1, reg.c_str(), instruction.type);

	if (instruction.type.has_base_type(BaseType::DOUBLE))
		load_to_reg(instruction.arg2, "%xmm1", instruction.type);

	std::string instr = format_instr(op, instruction.type);

	// Have to use xmm0 explicitely
	if (instruction.type.has_base_type(BaseType::DOUBLE))
		fprintf(file, "\t%s\t%s, %%xmm0\n", op.c_str(), reg.c_str());
	else
		fprintf(file, "\t%s\t%s, %s\n", instr.c_str(), format_memory_ref(instruction.arg2).c_str(), reg.c_str());

	store_from_reg(instruction.result, reg.c_str(), instruction.type);

	fprintf(file, "\n");
}

void Assembler::handle_cmp_op(TACInstruction &instruction, const std::string &op)
{
	comment_instruction(instruction);

	std::string actual_op = op;

	if (!instruction.type.is_signed())
		actual_op = fix_sign_instr(op);

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
	comment_instruction(instruction);

	Symbol *sym = gst->get_symbol(instruction.arg1);

	std::string cmp_text = get_cmp_instruction(instruction.type);
	std::string jmp = get_conditional_jump(instruction.cmp_op, instruction.type);

	fprintf(file, "\t%s\t%s, %s\n", cmp_text.c_str(), format_memory_ref(instruction.arg2).c_str(), format_memory_ref(instruction.arg1).c_str());

	fprintf(file, "\t%s\t%s\n\n", jmp.c_str(), instruction.result.c_str());
}

void Assembler::handle_goto(TACInstruction &instruction)
{
	comment_instruction(instruction);
	fprintf(file, "\tjmp\t%s\n\n", instruction.result.c_str());
}

void Assembler::handle_label(TACInstruction &instruction)
{
	fprintf(file, "%s: # %s\n", instruction.arg1.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
}

void Assembler::handle_call(TACInstruction &instruction)
{
	fprintf(file, "\tcall\t_%s # %s\n", instruction.arg1.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
}

// This may not work
void Assembler::handle_mov(TACInstruction &instruction)
{
	comment_instruction(instruction);

	std::string mov = get_mov_instruction(instruction.type);

	std::string arg1 = format_memory_ref(instruction.arg2);
	std::string arg2 = format_memory_ref(instruction.arg1);

	if (!arg1.empty() && arg1[0] == '$')
		arg1.erase(0, 1);

	if (!arg2.empty() && arg2[0] == '$')
		arg2.erase(0, 1);

	fprintf(file, "\t%s\t%s, %s\n\n", mov.c_str(), arg1.c_str(), arg2.c_str());
}

void Assembler::handle_nop(TACInstruction &instruction)
{
	comment_instruction(instruction);
}

void Assembler::handle_div(TACInstruction &instruction)
{
	return handle_div_mod(instruction, false);
}

void Assembler::handle_mod(TACInstruction &instruction)
{
	return handle_div_mod(instruction, true);
}

void Assembler::handle_div_mod(TACInstruction &instruction, bool is_mod)
{
	comment_instruction(instruction);

	if (instruction.type.has_base_type(BaseType::DOUBLE))
	{
		// Only division is valid for doubles (no mod)
		if (is_mod)
			throw std::runtime_error("Modulus not supported for doubles.");

		load_to_reg(instruction.arg1, "%xmm0", instruction.type);
		load_to_reg(instruction.arg2, "%xmm1", instruction.type);
		fprintf(file, "\tdivsd %%xmm1, %%xmm0\n");
		store_from_reg(instruction.result, "%xmm0", instruction.type);
		fprintf(file, "\n");
		return;
	}

	load_to_reg(instruction.arg1, "%rax", instruction.type);

	if (instruction.type.is_signed())
		fprintf(file, "\t%s\n", instruction.type.is_size_8() ? "cqto" : "cdq");
	else
		fprintf(file, "\txor\t%%rdx, %%rdx\n");

	std::string op = format_instr("idiv", instruction.type);
	std::string reg = get_reg_name("%r10", instruction.type);

	load_to_reg(instruction.arg2, "%r10", instruction.type);
	fprintf(file, "\t%s\t%s\n", op.c_str(), reg.c_str());

	const char *result_reg = is_mod ? "%rdx" : "%rax";
	store_from_reg(instruction.result, result_reg, instruction.type);
}

void Assembler::handle_unary_op(TACInstruction &instruction, const std::string &op)
{
	comment_instruction(instruction);

	if (instruction.type.has_base_type(BaseType::DOUBLE) && op == "neg")
	{
		load_to_reg(instruction.arg1, "%xmm0", instruction.type);
		load_to_reg(instruction.arg2, "%xmm1", instruction.type);

		fprintf(file, "\txorpd\t%%xmm1, %%xmm0\n");

		store_from_reg(instruction.result, "%xmm0", instruction.type);

		fprintf(file, "\n");
		return;
	}

	std::string reg = get_reg_name("%r10", instruction.type);
	std::string op_text = format_instr(op, instruction.type);

	load_to_reg(instruction.arg1, reg.c_str(), instruction.type);
	fprintf(file, "\t%s\t%s\n", op_text.c_str(), reg.c_str());
	store_from_reg(instruction.result, reg.c_str(), instruction.type);
}

// !
void Assembler::handle_not(TACInstruction &instruction)
{
	comment_instruction(instruction);

	load_to_reg(instruction.arg1, "%r10", instruction.type);

	std::string cmp_text = get_cmp_instruction(instruction.type);

	compare_and_store_result(instruction.arg1, instruction.arg2, instruction.result,
							 "%r10", "sete", instruction.type);

	store_from_reg(instruction.result, "%r10", instruction.type);

	fprintf(file, "\n");
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
	Symbol *src = gst->get_symbol(instruction.arg1);
	Symbol *dst = gst->get_symbol(instruction.result);

	Type src_type = instruction.type;
	Type dst_type = get_type_from_str(instruction.arg2);

	comment_instruction(instruction);

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
	comment_instruction(instruction);

	Symbol *src = gst->get_symbol(instruction.arg1);
	Symbol *dst = gst->get_symbol(instruction.result);

	// First, get the pointer value into a register
	fprintf(file, "\tmovq\t%d(%%rbp), %%rax\n", src->stack_offset);

	std::string mov = get_mov_instruction(instruction.type);
	std::string reg = get_reg_name("%r10", instruction.type);

	// Now dereference it and store the value
	fprintf(file, "\t%s\t(%%rax), %s\n", mov.c_str(), reg.c_str());
	store_from_reg(instruction.result, "%r10", instruction.type);

	fprintf(file, "\n");
}

void Assembler::handle_addr_of(TACInstruction &instruction)
{
	comment_instruction(instruction);

	Symbol *src = gst->get_symbol(instruction.arg1);
	Symbol *dst = gst->get_symbol(instruction.result);

	if (dst->type.get_size() != 8)
		error("Pointer should be 8 bytes");

	// Address-of always produces an 8-byte pointer
	fprintf(file, "\tleaq %d(%%rbp), %%rax\n", src->stack_offset);
	fprintf(file, "\tmovq %%rax, %d(%%rbp)\n\n", dst->stack_offset);
}

void Assembler::handle_printf(TACInstruction &instruction)
{
	comment_instruction(instruction);

	fprintf(file, "\tleaq\t_%s(%%rip), %%rdi\n", instruction.arg1.c_str());

	// Set float argument count
	// fprintf(file, "\tmovb\t$%s, %%al\n", instruction.arg2.c_str());

	fprintf(file, "\tcall\t_printf\n\n");
}

void Assembler::handle_struct_init(TACInstruction &instruction)
{
	comment_instruction(instruction);
	// No assembly needed - struct space is already allocated on stack
}

void Assembler::handle_member_assign(TACInstruction &instruction)
{
	comment_instruction(instruction);

	Symbol *struct_sym = gst->get_symbol(instruction.arg1);
	Symbol *value_sym = gst->get_symbol(instruction.result);

	// Calculate field offset from struct base
	int field_offset;
	if (instruction.arg2.find_first_not_of("0123456789") == std::string::npos)
	{
		// Array element access within a field
		int base_offset = struct_sym->type.get_field_offset(instruction.arg3);
		int elem_offset = std::stoi(instruction.arg2) * instruction.type.get_size();
		field_offset = base_offset + elem_offset;
	}
	else
	{
		// Named field offset
		field_offset = struct_sym->type.get_field_offset(instruction.arg2);
	}

	// Calculate actual stack offset
	int stack_offset = struct_sym->stack_offset - field_offset;

	std::string mov_text = get_mov_instruction(instruction.type.get_base_type());
	std::string reg_name = get_reg_name("%r10", instruction.type.get_base_type());

	// Load value into temporary register and then store to struct field
	if (value_sym != nullptr)
	{
		if (value_sym->is_literal8)
		{
			fprintf(file, "\tmovsd\t_%s(%%rip), %%xmm0\n", value_sym->name.c_str());
			fprintf(file, "\tmovsd\t%%xmm0, %d(%%rbp)\n", stack_offset);
		}
		else if (value_sym->type.is_array())
		{
			// Handle array field assignment
			int array_size = value_sym->type.get_array_size();
			for (int i = 0; i < array_size; i++)
			{
				fprintf(file, "\t%s\t%zu(%%rbp), %s\n",
						mov_text.c_str(),
						value_sym->stack_offset + i * instruction.type.get_size(),
						reg_name.c_str());
				fprintf(file, "\t%s\t%s, %zu(%%rbp)\n",
						mov_text.c_str(),
						reg_name.c_str(),
						stack_offset + i * instruction.type.get_size());
			}
		}
		else
		{
			fprintf(file, "\t%s\t%d(%%rbp), %s\n",
					mov_text.c_str(),
					value_sym->stack_offset,
					reg_name.c_str());
			fprintf(file, "\t%s\t%s, %d(%%rbp)\n",
					mov_text.c_str(),
					reg_name.c_str(),
					stack_offset);
		}
	}
	else
	{
		// Immediate value
		fprintf(file, "\t%s\t$%s, %d(%%rbp)\n",
				mov_text.c_str(),
				instruction.result.c_str(),
				stack_offset);
	}
	fprintf(file, "\n");
}

void Assembler::handle_member_access(TACInstruction &instruction)
{
	comment_instruction(instruction);

	Symbol *struct_sym = gst->get_symbol(instruction.arg1);

	int field_offset = struct_sym->type.get_field_offset(instruction.arg2);

	std::string mov = get_mov_instruction(instruction.type);
	std::string reg = get_reg_name("%r10", instruction.type);

	// Load from struct field
	fprintf(file, "\t%s\t%d(%%rbp), %s\n",
			mov.c_str(),
			struct_sym->stack_offset - field_offset,
			reg.c_str());

	// Store to destination
	store_from_reg(instruction.result, reg.c_str(), instruction.type);

	fprintf(file, "\n");
}

void Assembler::handle_log_and(TACInstruction &instruction)
{
	handle_log_op(instruction, "and");
}

void Assembler::handle_log_or(TACInstruction &instruction)
{
	handle_log_op(instruction, "or");
}

void Assembler::handle_log_op(TACInstruction &instruction, const std::string &op)
{
	comment_instruction(instruction);

	std::string instr = format_instr(op, instruction.type);
	std::string reg_a = get_reg_name("%r11", instruction.type);
	std::string reg_b = get_reg_name("%r10", instruction.type);

	load_to_reg(instruction.arg1, "%r11", instruction.type);
	load_to_reg(instruction.arg2, "%r10", instruction.type);

	fprintf(file, "\t%s\t%s, %s\n", instr.c_str(), reg_a.c_str(), reg_b.c_str());

	store_from_reg(instruction.result, "%r10", instruction.type);

	fprintf(file, "\n");
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

std::string Assembler::get_mov_instruction(Type type)
{
	return format_instr("mov", type);
}

std::string Assembler::get_cmp_instruction(Type type)
{
	if (type.has_base_type(BaseType::DOUBLE))
		return "comisd";

	return format_instr("cmp", type);
}

std::string Assembler::format_instr(std::string instr, Type type)
{
	if (!type.is_signed())
		instr = fix_sign_instr(instr);

	if (type.has_base_type(BaseType::DOUBLE))
		return instr + "sd";

	switch (type.get_size())
	{
	case 1:
		return instr + "b";
	case 4:
		return instr + "l";
	case 8:
		return instr + "q";
	default:
		std::cerr << "Assembler Error: Invalid type size for " << instr << ": " << type.get_size() << std::endl;
		type.print();
		return instr + "l";
	}
}

/*
	Fixes instruction to adhere if they are performed on unsigned values
*/
std::string Assembler::fix_sign_instr(std::string instr)
{
	if (instr == "imul")
		return "mul";
	else if (instr == "idiv")
		return "div";
	else if (instr == "setl")
		return "setb"; // below
	else if (instr == "setle")
		return "setbe"; // below or equal
	else if (instr == "setg")
		return "seta"; // above
	else if (instr == "setge")
		return "setae"; // above or equal

	// Issue may occur here
	return instr;
}

std::string Assembler::get_reg_name(const char *base_reg, Type type)
{
	if (type.has_base_type(BaseType::DOUBLE))
		return "%xmm1";

	if (strcmp(base_reg, "%rax") == 0 && type.get_size() == 4)
		return "%eax";

	std::string reg_name = base_reg;

	if (strcmp(base_reg, "%eax") == 0 && type.get_size() == 4)
		return reg_name;

	switch (type.get_size())
	{
	case 1:
		return reg_name + "b";
	case 4:
		return reg_name + "d";
	case 8:
		return reg_name;
	default:
		std::cerr << "Invalid type size for register: " << type.get_size() << std::endl;
		type.print();
		return reg_name + "d";
	}
}

std::string Assembler::format_memory_ref(const std::string &sym_name)
{
	Symbol *sym = gst->get_symbol(sym_name);

	if (!sym)
		return "$" + sym_name;

	if (sym->has_static_sd() || sym->is_literal8)
	{
		return "_" + sym->name + "(%rip)";
	}
	else
		return std::to_string(sym->stack_offset) + "(%rbp)";
}

std::string Assembler::get_conditional_jump(BinOpType op, Type type)
{

	switch (op)
	{
	case BinOpType::EQUAL:
		return "je";
	case BinOpType::NOT_EQUAL:
		return "jne";
	case BinOpType::LESS_THAN:
		return type.is_signed() ? "jl" : "jb";
	case BinOpType::GREATER_THAN:
		return type.is_signed() ? "jg" : "ja";
	case BinOpType::LESS_OR_EQUAL:
		return type.is_signed() ? "jle" : "jbe";
	case BinOpType::GREATER_OR_EQUAL:
		return type.is_signed() ? "jge" : "jae";
	default:
		throw std::runtime_error("No conditional jump for this BinOpType");
	}
}

void Assembler::comment_instruction(TACInstruction &instr)
{
	fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instr).c_str());
}

void Assembler::error(const std::string &message)
{
	throw std::runtime_error("Assembler Error: " + message);
}