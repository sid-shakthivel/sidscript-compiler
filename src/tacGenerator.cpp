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
    case BinOpType::AND:
        return TACOp::AND;
    case BinOpType::OR:
        return TACOp::OR;
    case BinOpType::EQUAL:
        return TACOp::EQUAL;
    case BinOpType::NOT_EQUAL:
        return TACOp::NOT_EQUAL;
    case BinOpType::LESS_THAN:
        return TACOp::LT;
    case BinOpType::GREATER_THAN:
        return TACOp::GT;
    case BinOpType::LESS_OR_EQUAL:
        return TACOp::LTE;
    case BinOpType::GREATER_OR_EQUAL:
        return TACOp::GTE;
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

TacGenerator::TacGenerator(SymbolTable *symbolTable) : symbolTable(symbolTable), tempCounter(0), labelCounter(0) {}

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
    instructions.emplace_back(TACOp::FUNC_BEGIN, func->name);
    for (auto element : func->elements)
        generate_tac_element(element);
    instructions.emplace_back(TACOp::FUNC_END);
}

void TacGenerator::generate_tac_element(ASTNode *element)
{
    if (element->type == NodeType::NODE_RETURN)
    {
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
    else if (element->type == NodeType::NODE_IF)
    {
        IfNode *if_stmt = (IfNode *)element;

        std::string condition_res = generate_tac_expr(if_stmt->condition);
        std::string label_success = gen_new_label();
        std::string label_failure = gen_new_label();

        instructions.emplace_back(TACOp::IF, condition_res, "", label_success);
        instructions.emplace_back(TACOp::GOTO, "", "", label_failure);

        instructions.emplace_back(TACOp::LABEL, label_success);

        for (auto element : if_stmt->then_elements)
            generate_tac_element(element);

        instructions.emplace_back(TACOp::LABEL, label_failure);

        for (auto element : if_stmt->else_elements)
            generate_tac_element(element);
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
        symbolTable->add_temporary_variable(temp_var);
        instructions.emplace_back(convert_UnaryOpType_to_TACOp(unary->op), result, "", temp_var);
        return temp_var;
    }
    else if (expr->type == NodeType::NODE_BINARY)
    {
        BinaryNode *binary = (BinaryNode *)expr;
        std::string arg1 = generate_tac_expr(binary->left);
        std::string arg2 = generate_tac_expr(binary->right);
        std::string temp_var = gen_new_temp_var();
        symbolTable->add_temporary_variable(temp_var);
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
        case TACOp::MOD:
            return "MOD";
        case TACOp::GT:
            return "GT";
        case TACOp::LT:
            return "LT";
        case TACOp::GTE:
            return "GTE";
        case TACOp::LTE:
            return "LTE";
        case TACOp::EQUAL:
            return "EQUAL";
        case TACOp::NOT_EQUAL:
            return "NOT_EQUAL";
        case TACOp::AND:
            return "AND";
        case TACOp::OR:
            return "OR";
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
            return "ALLOC_STACK";
        case TACOp::DEALLOC_STACK:
            return "DEALLOC_STACK";
        case TACOp::NEGATE:
            return "NEGATE";
        case TACOp::COMPLEMENT:
            return "COMPLEMENT";
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