#include <iostream>

#include "../include/tacGenerator.h"

TACOp convert_BinOpType_to_TACOp(BinOpType op)
{
    switch (op)
    {
    case BinOpType::ADD:
        return TACOp::ADD;
    case BinOpType::SUB:
        return TACOp::SUB;
    case BinOpType::MUL:
        return TACOp::MUL;
    case BinOpType::DIV:
        return TACOp::DIV;
    case BinOpType::MOD:
        return TACOp::MOD;
    }
}

TACOp convert_UnaryOpType_to_TACOp(UnaryOpType op)
{
    switch (op)
    {
    case UnaryOpType::NEGATE:
        return TACOp::NEGATE;
    case UnaryOpType::COMPLEMENT:
        return TACOp::COMPLEMENT;
    }
}

TacGenerator::TacGenerator(SymbolTable &symbolTable) : symbolTable(symbolTable), tempCounter(0), labelCounter(0) {}

std::string TacGenerator::gen_new_temp_var()
{
    return "t" + std::to_string(tempCounter++);
}

std::string TacGenerator::gen_new_label(std::string label)
{
    return "L" + label + std::to_string(labelCounter++);
}

std::vector<TACInstruction> TacGenerator::generate_tac(ProgramNode *program)
{
    generate_tac_func(program->func);
    return instructions;
}

void TacGenerator::generate_tac_func(FuncNode *func)
{
    symbolTable.enterScope();
    instructions.emplace_back(TACOp::FUNC_BEGIN, func->name);
    for (auto element : func->elements)
        generate_tac_element(element);
    instructions.emplace_back(TACOp::FUNC_END);
    symbolTable.exitScope();
}

void TacGenerator::generate_tac_element(ASTNode *element)
{
    if (element->type == NodeType::NODE_RETURN)
    {
        std::cout << "here\n";
        RtnNode *rtn = (RtnNode *)element;
        std::string result = generate_tac_expr(rtn->value);
        instructions.emplace_back(TACOp::RETURN, result);
    }
    else if (element->type == NodeType::NODE_VAR_DECL || element->type == NodeType::NODE_VAR_ASSIGN)
    {
        VarAssignNode *var_decl = (VarAssignNode *)element;
        std::string result = generate_tac_expr(var_decl->value);
        instructions.emplace_back(TACOp::ASSIGN, var_decl->var->name, result);
    }
}

std::string TacGenerator::generate_tac_expr(ASTNode *expr)
{
    if (expr->type == NodeType::NODE_VAR)
    {
        return ((VarNode *)expr)->name;
    }
    else if (expr->type == NodeType::NODE_INTEGER)
    {
        return std::to_string(((IntegerLiteral *)expr)->value);
    }
    else if (expr->type == NodeType::NODE_UNARY)
    {
        UnaryNode *unary = (UnaryNode *)expr;
        std::string result = generate_tac_expr(unary->value);
        std::string temp_var = gen_new_temp_var();
        symbolTable.declareVariable(temp_var, true);
        instructions.emplace_back(convert_UnaryOpType_to_TACOp(unary->op), result, "", temp_var);
        return temp_var;
    }
    else if (expr->type == NodeType::NODE_BINARY)
    {
        BinaryNode *binary = (BinaryNode *)expr;
        std::string arg1 = generate_tac_expr(binary->left);
        std::string arg2 = generate_tac_expr(binary->right);
        std::string temp_var = gen_new_temp_var();
        symbolTable.declareVariable(temp_var, true);
        instructions.emplace_back(convert_BinOpType_to_TACOp(binary->op), arg1, arg2, temp_var);
        return temp_var;
    }
}

void TacGenerator::print_tac()
{
    auto tacOpToString = [](TACOp op) -> std::string
    {
        switch (op)
        {
        case TACOp::ADD:
            return "ADD";
        case TACOp::SUB:
            return "SUB";
        case TACOp::MUL:
            return "MUL";
        case TACOp::DIV:
            return "DIV";
        case TACOp::ASSIGN:
            return "ASSIGN";
        case TACOp::IF:
            return "IF";
        case TACOp::GOTO:
            return "GOTO";
        case TACOp::LABEL:
            return "LABEL";
        case TACOp::RETURN:
            return "RETURN";
        case TACOp::FUNC_BEGIN:
            return "FUNC_BEGIN";
        case TACOp::FUNC_END:
            return "FUNC_END";
        case TACOp::ALLOC_STACK:
            return "ALLOC STACK";
        default:
            return "UNKNOWN";
        }
    };

    for (const auto &instr : instructions)
    {
        std::cout << tacOpToString(instr.op);
        if (!instr.arg1.empty())
            std::cout << " " << instr.arg1;
        if (!instr.arg2.empty())
            std::cout << ", " << instr.arg2;
        if (!instr.result.empty())
            std::cout << " -> " << instr.result;
        std::cout << std::endl;
    }
}