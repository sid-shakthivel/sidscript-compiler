#include <cstdio>
#include <cerrno>
#include <iostream>
#include <functional>
#include <numeric>

#include "../include/assembler.h"
#include "../include/parser.h"
#include "../include/tacGenerator.h"

Assembler::Assembler(std::shared_ptr<GlobalSymbolTable> gst) : gst(gst) {}

void Assembler::assemble(std::vector<TACInstruction> instructions, std::string filename)
{
    FILE *file = fopen(filename.c_str(), "w");
    if (file == NULL)
    {
        char errorMessage[256];
        snprintf(errorMessage, sizeof(errorMessage), "Error opening file %s", filename.c_str());
        perror(errorMessage);
        return;
    }

    fprintf(file, ".section __TEXT,__text,regular,pure_instructions\n");
    fprintf(file, ".build_version macos, 15, 0 sdk_version 15, 1\n");
    fprintf(file, ".p2align 4, 0x90\n\n");

    for (auto &instruction : instructions)
        assemble_tac(instruction, file);
}

void Assembler::assemble_tac(TACInstruction &instruction, FILE *file)
{
    bool isLong = (instruction.type == Type::LONG);

    auto load_to_reg = [file, this, isLong](const std::string &operand, const char *reg)
    {
        Symbol *potential_var = gst->get_symbol(current_func, operand);
        if (potential_var == nullptr)
            fprintf(file, "\t%s\t$%s, %s\n", isLong ? "movq" : "movl", operand.c_str(), reg);
        else
        {
            if (potential_var->storage_duration == StorageDuration::Static)
                fprintf(file, "\t%s\t_%s(%%rip), %s\n", isLong ? "movq" : "movl", potential_var->name.c_str(), reg);
            else
                fprintf(file, "\t%s\t%d(%%rbp), %s\n", isLong ? "movq" : "movl", potential_var->stack_offset, reg);
        }
    };

    auto store_from_reg = [file, this, isLong](const std::string &operand, const char *reg)
    {
        Symbol *potential_var = gst->get_symbol(current_func, operand);
        if (potential_var == nullptr)
            fprintf(file, "\t%s\t%s, $%s\n", isLong ? "movq" : "movl", reg, operand.c_str());
        else
            fprintf(file, "\t%s\t%s, %d(%%rbp)\n", isLong ? "movq" : "movl", reg, potential_var->stack_offset);
    };

    auto bin_op_to_reg = [file, this, isLong](const std::string &operand, const char *reg, const std::string &op)
    {
        Symbol *potential_var = gst->get_symbol(current_func, operand);
        if (potential_var == nullptr)
            fprintf(file, "\t%s\t$%s, %s\n", op.c_str(), operand.c_str(), reg);
        else
            fprintf(file, "\t%s\t%d(%%rbp), %s\n", op.c_str(), potential_var->stack_offset, reg);
        fprintf(file, "\n");
    };

    auto cmp_op_to_reg = [file, this, load_to_reg, store_from_reg](const std::string &operand_a, const std::string &operand_b, const std::string &result, const char *reg, const std::string &op)
    {
        load_to_reg(operand_a, reg);
        Symbol *potential_var_b = gst->get_symbol(current_func, operand_b);
        if (potential_var_b == nullptr)
            fprintf(file, "\tcmpl\t$%s, %s\n", operand_b.c_str(), reg);
        else
            fprintf(file, "\tcmpl\t%d(%%rbp), %s\n", potential_var_b->stack_offset, reg);

        std::string reg_b = reg;
        reg_b.pop_back();
        reg_b += "b";

        fprintf(file, "\t%s\t%s\n", op.c_str(), reg_b.c_str());
        fprintf(file, "\tmovzbl\t%s, %s\n", reg_b.c_str(), reg);
        store_from_reg(result, reg);
        fprintf(file, "\n");
    };

    switch (instruction.op)
    {
    case TACOp::FUNC_BEGIN:
        handle_func_begin(instruction, file);
        break;
    case TACOp::FUNC_END:
        handle_func_end(instruction, file);
        break;
    case TACOp::ASSIGN:
        handle_assign(instruction, file, load_to_reg, store_from_reg);
        break;
    case TACOp::RETURN:
        handle_return(instruction, file, load_to_reg);
        break;
    case TACOp::ADD:
        handle_bin_op(instruction, file, load_to_reg, bin_op_to_reg, store_from_reg, "addl");
        break;
    case TACOp::SUB:
        handle_bin_op(instruction, file, load_to_reg, bin_op_to_reg, store_from_reg, "subl");
        break;
    case TACOp::MUL:
        handle_bin_op(instruction, file, load_to_reg, bin_op_to_reg, store_from_reg, "imull");
        break;
    case TACOp::DIV:
        handle_div(instruction, file, load_to_reg, store_from_reg);
        break;
    case TACOp::MOD:
        handle_mod(instruction, file, load_to_reg, store_from_reg);
        break;
    case TACOp::COMPLEMENT:
        handle_unary_op(instruction, file, load_to_reg, store_from_reg, "notl");
        break;
    case TACOp::NEGATE:
        handle_unary_op(instruction, file, load_to_reg, store_from_reg, "negl");
        break;
    case TACOp::LT:
        handle_cmp_op(instruction, file, cmp_op_to_reg, "setl");
        break;
    case TACOp::LTE:
        handle_cmp_op(instruction, file, cmp_op_to_reg, "setle");
        break;
    case TACOp::GT:
        handle_cmp_op(instruction, file, cmp_op_to_reg, "setg");
        break;
    case TACOp::GTE:
        handle_cmp_op(instruction, file, cmp_op_to_reg, "setge");
        break;
    case TACOp::EQUAL:
        handle_cmp_op(instruction, file, cmp_op_to_reg, "sete");
        break;
    case TACOp::NOT_EQUAL:
        handle_cmp_op(instruction, file, cmp_op_to_reg, "setne");
        break;
    case TACOp::IF:
        handle_if(instruction, file);
        break;
    case TACOp::GOTO:
        handle_goto(instruction, file);
        break;
    case TACOp::LABEL:
        handle_label(instruction, file);
        break;
    case TACOp::AND:
        handle_bin_op(instruction, file, load_to_reg, bin_op_to_reg, store_from_reg, "andl");
        break;
    case TACOp::OR:
        handle_bin_op(instruction, file, load_to_reg, bin_op_to_reg, store_from_reg, "orl");
        break;
    case TACOp::CALL:
        handle_call(instruction, file);
        break;
    case TACOp::MOV:
        handle_mov(instruction, file);
        break;
    case TACOp::NOP:
        fprintf(file, "# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
        break;
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
        fprintf(file, "# Unknown TAC operation\n");
        break;
    }
}

void Assembler::handle_func_begin(TACInstruction &instruction, FILE *file)
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

void Assembler::handle_func_end(TACInstruction &instruction, FILE *file)
{
    fprintf(file, "\n.L%s_end: # %s", current_func.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
    int stack_space = gst->get_func_st(current_func)->get_var_count() * 8;
    fprintf(file, "\taddq\t$%d, %%rsp\n", stack_space);
    fprintf(file, "\tpopq\t%%rbp\n");
    fprintf(file, "\tretq\n\n");
    current_func = "";
}

void Assembler::handle_assign(TACInstruction &instruction, FILE *file, std::function<void(const std::string &, const char *)> load_to_reg, std::function<void(const std::string &, const char *)> store_from_reg)
{
    Symbol *var = gst->get_symbol(current_func, instruction.arg1);
    Symbol *potential_var = gst->get_symbol(current_func, instruction.arg2);

    if (potential_var == nullptr)
    {
        if (var->storage_duration == StorageDuration::Static)
            fprintf(file, "\tmovl\t$%s, _%s(%%rip) # %s\n", instruction.arg2.c_str(), var->name.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
        else
            fprintf(file, "\tmovl\t$%s, %d(%%rbp) # %s\n", instruction.arg2.c_str(), var->stack_offset, TacGenerator::gen_tac_str(instruction).c_str());
    }
    else
    {
        if (var->storage_duration == StorageDuration::Static)
        {
            if (potential_var->storage_duration == StorageDuration::Static)
                fprintf(file, "\tmovl\t%%r10d, _%s(%%rip) # %s\n", potential_var->name.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
            else
                fprintf(file, "\tmovl\t%%r10d, %d(%%rbp) # %s\n", potential_var->stack_offset, TacGenerator::gen_tac_str(instruction).c_str());

            fprintf(file, "\tmovl\t%%r10d, _%s(%%rip)\n", var->name.c_str());
        }
        else
        {
            if (potential_var->storage_duration == StorageDuration::Static)
                fprintf(file, "\tmovl\t_%s(%%rip), %%r10d # %s\n", potential_var->name.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
            else
                fprintf(file, "\tmovl\t%d(%%rbp), %%r10d # %s\n", potential_var->stack_offset, TacGenerator::gen_tac_str(instruction).c_str());

            fprintf(file, "\tmovl\t%%r10d, %d(%%rbp)\n", var->stack_offset);
        }
    }
}

void Assembler::handle_return(TACInstruction &instruction, FILE *file, std::function<void(const std::string &, const char *)> load_to_reg)
{
    fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
    load_to_reg(instruction.arg1, "%eax");
    fprintf(file, "\tjmp\t.L%s_end\n", current_func.c_str());
}

void Assembler::handle_bin_op(TACInstruction &instruction, FILE *file, std::function<void(const std::string &, const char *)> load_to_reg, std::function<void(const std::string &, const char *, const std::string &)> bin_op_to_reg, std::function<void(const std::string &, const char *)> store_from_reg, const std::string &op)
{
    fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
    load_to_reg(instruction.arg1, "%r10d");
    bin_op_to_reg(instruction.arg2, "%r10d", op);
    store_from_reg(instruction.result, "%r10d");
}

void Assembler::handle_div(TACInstruction &instruction, FILE *file, std::function<void(const std::string &, const char *)> load_to_reg, std::function<void(const std::string &, const char *)> store_from_reg)
{
    fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
    load_to_reg(instruction.arg1, "%r10d");
    fprintf(file, "\tcltd\n");
    load_to_reg(instruction.arg2, "%r10d");
    fprintf(file, "\tidivl\t%%r10d\n");
    store_from_reg(instruction.result, "%eax");
}

void Assembler::handle_mod(TACInstruction &instruction, FILE *file, std::function<void(const std::string &, const char *)> load_to_reg, std::function<void(const std::string &, const char *)> store_from_reg)
{
    fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
    load_to_reg(instruction.arg1, "%r10d");
    fprintf(file, "\tcltd\n");
    load_to_reg(instruction.arg2, "%r10d");
    fprintf(file, "\tidivl\t%%r10d\n");
    store_from_reg(instruction.result, "%edx");
}

void Assembler::handle_unary_op(TACInstruction &instruction, FILE *file, std::function<void(const std::string &, const char *)> load_to_reg, std::function<void(const std::string &, const char *)> store_from_reg, const std::string &op)
{
    fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
    load_to_reg(instruction.arg1, "%r10d");
    fprintf(file, "\t%s\t%%r10d\n", op.c_str());
    store_from_reg(instruction.result, "%r10d");
}

void Assembler::handle_cmp_op(TACInstruction &instruction, FILE *file, std::function<void(const std::string &, const std::string &, const std::string &, const char *, const std::string &)> cmp_op_to_reg, const std::string &op)
{
    fprintf(file, "\t# %s", TacGenerator::gen_tac_str(instruction).c_str());
    cmp_op_to_reg(instruction.arg1, instruction.arg2, instruction.result, "%r10d", op);
}

void Assembler::handle_if(TACInstruction &instruction, FILE *file)
{
    fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
    Symbol *potential_var = gst->get_symbol(current_func, instruction.arg1);
    if (potential_var == nullptr)
        fprintf(file, "\tcmpl\t$0, %s\n", instruction.arg1.c_str());
    else
        fprintf(file, "\tcmpl\t$0, %d(%%rbp)\n", potential_var->stack_offset);
    fprintf(file, "\tjne\t%s\n", instruction.result.c_str());
}

void Assembler::handle_goto(TACInstruction &instruction, FILE *file)
{
    fprintf(file, "\tjmp\t%s # %s\n", instruction.result.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
}

void Assembler::handle_label(TACInstruction &instruction, FILE *file)
{
    fprintf(file, "\n%s: # %s\n", instruction.arg1.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
}

void Assembler::handle_call(TACInstruction &instruction, FILE *file)
{
    fprintf(file, "\tcall\t_%s # %s\n", instruction.arg1.c_str(), TacGenerator::gen_tac_str(instruction).c_str());
}

void Assembler::handle_mov(TACInstruction &instruction, FILE *file)
{
    fprintf(file, "\t# %s\n", TacGenerator::gen_tac_str(instruction).c_str());
    Symbol *potential_var = gst->get_symbol(current_func, instruction.arg1);
    Symbol *potential_var_2 = gst->get_symbol(current_func, instruction.arg2);

    if (potential_var == nullptr && potential_var_2 == nullptr)
        fprintf(file, "\tmovl\t%s, %s\n", instruction.arg2.c_str(), instruction.arg1.c_str());
    else if (potential_var_2 != nullptr && potential_var == nullptr)
        fprintf(file, "\tmovl\t%d(%%rbp), %s\n", potential_var_2->stack_offset, instruction.arg1.c_str());
    else if (potential_var != nullptr && potential_var_2 == nullptr)
        fprintf(file, "\tmovl\t%s, %d(%%rbp)\n", instruction.arg2.c_str(), potential_var->stack_offset);
    else
        fprintf(file, "\tmovl\t%d(%%rbp), %d(%%rbp)\n", potential_var_2->stack_offset, potential_var->stack_offset);
}

