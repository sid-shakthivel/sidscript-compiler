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

TacGenerator::TacGenerator(std::shared_ptr<GlobalSymbolTable> gst) : gst(gst), tempCounter(0), labelCounter(0) {}

std::string TacGenerator::gen_new_temp_var()
{
    return "t" + std::to_string(tempCounter++);
}

std::string TacGenerator::gen_new_label(std::string label)
{
    return ".L" + label + std::to_string(labelCounter++);
}

void TacGenerator::generate_tac(std::shared_ptr<ProgramNode> program)
{
    for (auto &decl : program->decls)
    {
        if (decl->type == NodeType::NODE_FUNCTION)
            generate_tac_func(dynamic_cast<FuncNode *>(decl.get()));
        else if (decl->type == NodeType::NODE_VAR_DECL)
            generate_tac_element(dynamic_cast<VarDeclNode *>(decl.get()));
    }

    instructions.insert(instructions.begin(), TACInstruction(TACOp::ENTER_TEXT));

    if (!data_vars.empty())
    {
        instructions.insert(instructions.begin(), data_vars.begin(), data_vars.end());
        instructions.insert(instructions.begin(), TACInstruction(TACOp::ENTER_DATA));
    }

    if (!bss_vars.empty())
    {
        instructions.insert(instructions.begin(), bss_vars.begin(), bss_vars.end());
        instructions.insert(instructions.begin(), TACInstruction(TACOp::ENTER_BSS));
    }
}

void TacGenerator::generate_tac_func(FuncNode *func)
{
    current_func = func->name;
    instructions.emplace_back(TACOp::FUNC_BEGIN, func->name, func->specifier == Specifier::STATIC ? "static" : "global");

    current_st = gst->get_func_st(func->name);

    FuncSymbol *func_symbol = gst->get_func_symbol(func->name);

    for (int i = 0; i < func->params.size(); i++)
        if (i < 6)
            instructions.emplace_back(TACOp::MOV, func->get_param_name(i), registers[i], "", func_symbol->arg_types[i]);

    for (auto &element : func->elements)
        generate_tac_element(element.get());
    instructions.emplace_back(TACOp::FUNC_END);

    current_st = nullptr;
    current_func = "";
}

void TacGenerator::generate_tac_element(ASTNode *element)
{
    // Explain this (used for loops)
    auto switch_condition = [](BinaryNode *node) -> BinOpType
    {
        switch (node->op)
        {
        case BinOpType::EQUAL:
            return BinOpType::NOT_EQUAL;
        case BinOpType::NOT_EQUAL:
            return BinOpType::EQUAL;
        case BinOpType::LESS_THAN:
            return BinOpType::GREATER_OR_EQUAL;
        case BinOpType::LESS_OR_EQUAL:
            return BinOpType::GREATER_THAN;
        case BinOpType::GREATER_THAN:
            return BinOpType::LESS_OR_EQUAL;
        case BinOpType::GREATER_OR_EQUAL:
            return BinOpType::LESS_THAN;
        default:
            return node->op;
        }
    };

    if (element->type == NodeType::NODE_RETURN)
    {
        FuncSymbol *func = gst->get_func_symbol(current_func);
        RtnNode *rtn = (RtnNode *)element;
        std::string result = generate_tac_expr(rtn->value.get());
        instructions.emplace_back(TACOp::RETURN, result, "", "", func->return_type);
    }
    else if (element->type == NodeType::NODE_VAR_DECL)
    {
        VarDeclNode *var_decl = (VarDeclNode *)element;
        Symbol *var_symbol = gst->get_symbol(current_func, var_decl->var->name);

        // Check if some sort of global/static
        if (var_symbol->linkage != Linkage::None || var_symbol->storage_duration == StorageDuration::Static)
        {
            // Place in BSS if not initialised
            if (var_decl->value == nullptr)
                bss_vars.emplace_back(TACOp::ASSIGN, var_decl->var->name, var_symbol->linkage == Linkage::External ? "global" : "", "0", var_symbol->type);
            else
            {
                // Place in Data
                std::string result = generate_tac_expr(var_decl->value.get());
                data_vars.emplace_back(TACOp::ASSIGN, var_decl->var->name, var_symbol->linkage == Linkage::External ? "global" : "", result, var_symbol->type);
            }

            return;
        }

        // Only need to assign anything if initialised to a value otherwise ignore
        if (var_decl->value != nullptr)
        {
            std::string result = generate_tac_expr(var_decl->value.get(), var_symbol->type);
            instructions.emplace_back(TACOp::ASSIGN, var_decl->var->name, result, "", var_symbol->type);
        }
    }
    else if (element->type == NodeType::NODE_VAR_ASSIGN)
    {
        VarAssignNode *var_assign = (VarAssignNode *)element;
        Symbol *var_symbol = gst->get_symbol(current_func, var_assign->var->name);

        std::string result = generate_tac_expr(var_assign->value.get(), var_symbol->type);
        instructions.emplace_back(TACOp::ASSIGN, var_assign->var->name, result, "", var_symbol->type);
    }
    else if (element->type == NodeType::NODE_IF)
    {
        IfNode *if_stmt = (IfNode *)element;

        std::string condition_res = generate_tac_expr(if_stmt->condition.get());
        std::string label_success = gen_new_label();
        std::string label_failure = gen_new_label();

        TACInstruction if_instruction(TACOp::IF, condition_res, "", label_success);
        if_instruction.op2 = convert_BinOpType_to_TACOp(if_stmt->condition->op);

        instructions.emplace_back(if_instruction);
        instructions.emplace_back(TACOp::GOTO, "", "", label_failure);

        instructions.emplace_back(TACOp::LABEL, label_success);

        for (auto &element : if_stmt->then_elements)
            generate_tac_element(element.get());

        instructions.emplace_back(TACOp::LABEL, label_failure);

        for (auto &element : if_stmt->else_elements)
            generate_tac_element(element.get());
    }
    else if (element->type == NodeType::NODE_WHILE)
    {
        WhileNode *while_stmt = (WhileNode *)element;

        std::string start = while_stmt->label + "_start";
        std::string post = while_stmt->label + "_post";
        std::string end = while_stmt->label + "_end";

        instructions.emplace_back(TACOp::LABEL, start);

        while_stmt->condition->op = switch_condition(while_stmt->condition.get());
        std::string condition_res = generate_tac_expr(while_stmt->condition.get());

        TACInstruction if_instruction(TACOp::IF, condition_res, "", end);
        if_instruction.op2 = convert_BinOpType_to_TACOp(while_stmt->condition->op);
        instructions.emplace_back(if_instruction);

        instructions.emplace_back(TACOp::NOP);

        for (auto &element : while_stmt->elements)
            generate_tac_element(element.get());

        instructions.emplace_back(TACOp::LABEL, post);
        instructions.emplace_back(TACOp::GOTO, "", "", start);

        instructions.emplace_back(TACOp::NOP);

        instructions.emplace_back(TACOp::LABEL, end);
    }
    else if (element->type == NodeType::NODE_FOR)
    {
        ForNode *for_stmt = (ForNode *)element;

        generate_tac_element(for_stmt->init.get());

        std::string start = for_stmt->label + "_start";
        std::string post = for_stmt->label + "_post";
        std::string end = for_stmt->label + "_end";

        instructions.emplace_back(TACOp::LABEL, start);

        for_stmt->condition->op = switch_condition(for_stmt->condition.get());
        std::string condition_res = generate_tac_expr(for_stmt->condition.get());

        TACInstruction if_instruction(TACOp::IF, condition_res, "", end);
        if_instruction.op2 = convert_BinOpType_to_TACOp(for_stmt->condition->op);
        instructions.emplace_back(if_instruction);

        instructions.emplace_back(TACOp::NOP);

        for (auto &element : for_stmt->elements)
            generate_tac_element(element.get());

        instructions.emplace_back(TACOp::LABEL, post);
        generate_tac_element(for_stmt->post.get());
        instructions.emplace_back(TACOp::GOTO, "", "", start);

        instructions.emplace_back(TACOp::NOP);

        instructions.emplace_back(TACOp::LABEL, end);
    }
    else if (element->type == NodeType::NODE_BREAK)
    {
        instructions.emplace_back(TACOp::GOTO, "", "", ((BreakNode *)element)->label + "_end");
    }
    else if (element->type == NodeType::NODE_CONTINUE)
    {
        instructions.emplace_back(TACOp::GOTO, "", "", ((ContinueNode *)element)->label + "_post");
    }
    else if (element->type == NodeType::NODE_UNARY)
    {
        UnaryNode *unary = (UnaryNode *)element;
    }
}

std::string TacGenerator::generate_tac_expr(ASTNode *expr, Type type)
{
    if (expr->type == NodeType::NODE_VAR)
    {
        return ((VarNode *)expr)->name;
    }
    else if (expr->type == NodeType::NODE_CAST)
    {
        CastNode *cast = (CastNode *)expr;

        std::string temp_var = gen_new_temp_var();
        current_st->declare_temp_variable(temp_var, cast->target_type);

        std::string result = generate_tac_expr(cast->expr.get(), cast->target_type);
        instructions.emplace_back(TACOp::CONVERT_TYPE, result, get_type_str(cast->src_type), temp_var, cast->target_type);

        return temp_var;
    }
    else if (expr->type == NodeType::NODE_NUMBER)
    {
        NumericLiteral *num = (NumericLiteral *)expr;
        if (num->value_type == Type::UINT)
            return std::to_string(((UIntegerLiteral *)num)->value);
        else if (num->value_type == Type::ULONG)
            return std::to_string(((ULongLiteral *)num)->value);
        else if (num->value_type == Type::LONG)
            return std::to_string(((LongLiteral *)num)->value);
        else
            return std::to_string(((IntegerLiteral *)num)->value);
    }
    else if (expr->type == NodeType::NODE_UNARY)
    {
        UnaryNode *unary = (UnaryNode *)expr;
        std::string result = generate_tac_expr(unary->value.get());
        std::string temp_var = gen_new_temp_var();
        current_st->declare_temp_variable(temp_var, Type::INT);
        instructions.emplace_back(convert_UnaryOpType_to_TACOp(unary->op), result, "", temp_var, unary->type);
        return temp_var;
    }
    else if (expr->type == NodeType::NODE_BINARY)
    {
        BinaryNode *binary = (BinaryNode *)expr;
        std::string arg1 = generate_tac_expr(binary->left.get());
        std::string arg2 = generate_tac_expr(binary->right.get());
        std::string temp_var = gen_new_temp_var();
        current_st->declare_temp_variable(temp_var, Type::INT);
        instructions.emplace_back(convert_BinOpType_to_TACOp(binary->op), arg1, arg2, temp_var, binary->type);
        return temp_var;
    }
    else if (expr->type == NodeType::NODE_FUNC_CALL)
    {
        FuncCallNode *func = (FuncCallNode *)expr;

        for (int i = 0; i < func->args.size(); i++)
        {
            if (i < 6)
            {
                ASTNode *arg = func->args[i].get();

                if (arg->type == NodeType::NODE_VAR)
                {
                    VarNode *var_node = dynamic_cast<VarNode *>(arg);
                    Type type = gst->get_symbol(current_func, var_node->name)->type;
                    instructions.emplace_back(TACOp::MOV, registers[i], var_node->name, "", type);
                }
                else if (arg->type == NodeType::NODE_NUMBER)
                {
                    NumericLiteral *num_literal = dynamic_cast<IntegerLiteral *>(arg);
                    if (num_literal->value_type == Type::INT)
                        instructions.emplace_back(TACOp::MOV, registers[i], "$" + std::to_string(((IntegerLiteral *)num_literal)->value), "", Type::INT);
                    else if (num_literal->value_type == Type::LONG)
                        instructions.emplace_back(TACOp::MOV, registers[i], "$" + std::to_string(((LongLiteral *)num_literal)->value), "", Type::LONG);
                }
            }
            else
            {
                // Need to consider what happens when pushing a variable - need to reference the bit of memory
                // instructions.emplace_back(TACOp::PUSH, func->args[i], "", "");
            }
        }

        int stack_offset = (func->args.size() - 6) + current_st->get_stack_size();

        if (stack_offset % 2 != 0 && stack_offset > 0)
        {
            instructions.emplace_back(TACOp::DEALLOC_STACK, "8");
        }

        instructions.emplace_back(TACOp::CALL, func->name);

        if (stack_offset % 2 != 0 && stack_offset > 0)
        {
            instructions.emplace_back(TACOp::ALLOC_STACK, "8");
        }

        std::string temp_var = gen_new_temp_var();
        current_st->declare_temp_variable(temp_var, Type::INT);

        instructions.emplace_back(TACOp::MOV, temp_var, "%eax");

        return temp_var;
    }
}

void TacGenerator::print_all_tac()
{
    for (auto &instr : instructions)
        std::cout << gen_tac_str(instr) << std::endl;
}

std::string TacGenerator::gen_tac_str(TACInstruction &instr)
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
            return "FUNC_END\n";
        case TACOp::ALLOC_STACK:
            return "ALLOC_STACK";
        case TACOp::DEALLOC_STACK:
            return "DEALLOC_STACK";
        case TACOp::NEGATE:
            return "NEGATE";
        case TACOp::COMPLEMENT:
            return "COMPLEMENT";
        case TACOp::NOP:
            return "NOP";
        case TACOp::PUSH:
            return "PUSH";
        case TACOp::CALL:
            return "CALL";
        case TACOp::MOV:
            return "MOV";
        case TACOp::INCREMENT:
            return "INCREMENT";
        case TACOp::DECREMENT:
            return "DECREMENT";
        case TACOp::ENTER_BSS:
            return "ENTER_BSS";
        case TACOp::ENTER_DATA:
            return "ENTER_DATA";
        case TACOp::ENTER_TEXT:
            return "ENTER_TEXT";
        case TACOp::CONVERT_TYPE:
            return "CONVERT_TYPE";
        default:
            return "UNKNOWN";
        }
    };

    std::string str = tacOpToString(instr.op);

    if (!instr.arg1.empty())
        str += " " + instr.arg1;
    if (!instr.arg2.empty())
        str += ", " + instr.arg2;
    if (!instr.result.empty())
        str += " -> " + instr.result;

    if (instr.type != Type::VOID)
        str += " (" + get_type_str(instr.type) + ")";

    return str;
}