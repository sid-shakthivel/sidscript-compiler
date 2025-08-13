#include <iostream>

#include "../include/tacGenerator.h"
#include "../include/semanticAnalyser.h"

#define REGISTER_HANDLER(nodeType, fn) \
    handlers[NodeType::nodeType] = [this](ASTNode *node) { fn(node); };

#define REGISTER_EXPR_HANDLER(nodeType, fn) \
    expr_handlers[NodeType::nodeType] = [this](ASTNode *node) { return fn(node); };

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

    throw std::runtime_error("Invalid BinOpType");
}

TACOp convert_UnaryOpType_to_TACOp(UnaryOpType op)
{
    switch (op)
    {
    case UnaryOpType::NEGATE:
        return TACOp::NEGATE;
    case UnaryOpType::COMPLEMENT:
        return TACOp::COMPLEMENT;
    case UnaryOpType::NOT:
        return TACOp::NOT;
    case UnaryOpType::DEREF:
        return TACOp::DEREF;
    case UnaryOpType::ADDR_OF:
        return TACOp::ADDR_OF;
    }

    throw std::runtime_error("Invalid UnaryOpType");
}

TacGenerator::TacGenerator(std::shared_ptr<GlobalSymbolTable> gst, std::shared_ptr<SemanticAnalyser> sem_analyser) : gst(gst), sem_analyser(sem_analyser)
{
    // Register main handlers
    REGISTER_HANDLER(NODE_FUNCTION, generate_tac_func);
    REGISTER_HANDLER(NODE_RETURN, generate_tac_rtn);
    REGISTER_HANDLER(NODE_VAR_DECL, generate_tac_var_decl);
    REGISTER_HANDLER(NODE_VAR_ASSIGN, generate_tac_var_assign);
    REGISTER_HANDLER(NODE_IF, generate_tac_if);
    REGISTER_HANDLER(NODE_WHILE, generate_tac_while);
    REGISTER_HANDLER(NODE_FOR, generate_tac_for);
    REGISTER_HANDLER(NODE_LOOP_CONTROL, generate_tac_loop_ctrl);
    REGISTER_HANDLER(NODE_POSTFIX, generate_tac_postfix);
    REGISTER_HANDLER(NODE_FUNC_CALL, generate_tac_func_call);

    // Register expr handlers
    REGISTER_EXPR_HANDLER(NODE_VAR, generate_tac_expr_var);
    REGISTER_EXPR_HANDLER(NODE_BOOL, generate_tac_expr_bool);
    REGISTER_EXPR_HANDLER(NODE_CAST, generate_tac_expr_cast);
    REGISTER_EXPR_HANDLER(NODE_NUMBER, generate_tac_expr_number);
    REGISTER_EXPR_HANDLER(NODE_CHAR, generate_tac_expr_char);
    REGISTER_EXPR_HANDLER(NODE_STRING, generate_tac_expr_string);
    REGISTER_EXPR_HANDLER(NODE_UNARY, generate_tac_expr_unary);
    REGISTER_EXPR_HANDLER(NODE_BINARY, generate_tac_expr_binary);
    REGISTER_EXPR_HANDLER(NODE_POSTFIX, generate_tac_expr_postfix);
    REGISTER_EXPR_HANDLER(NODE_ARRAY_ACCESS, generate_tac_expr_array_access);
    REGISTER_EXPR_HANDLER(NODE_FUNC_CALL, generate_tac_expr_func_call);
    REGISTER_EXPR_HANDLER(NODE_SIZE_OF, generate_tac_expr_size_of);
}

std::string TacGenerator::gen_new_temp_var()
{
    return "t" + std::to_string(tempCounter++);
}

std::string TacGenerator::gen_new_label(std::string label)
{
    return ".L" + label + std::to_string(labelCounter++);
}

std::string TacGenerator::gen_new_const_label()
{
    return ".L" + std::string("const_") + std::to_string(constCounter++);
}

void TacGenerator::generate_all_tac(std::shared_ptr<ProgramNode> program)
{
    for (auto &decl : program->decls)
        generate_tac(decl.get());

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

    if (!literal8_vars.empty())
    {
        instructions.insert(instructions.begin(), literal8_vars.begin(), literal8_vars.end());
        instructions.insert(instructions.begin(), TACInstruction(TACOp::ENTER_LITERAL8));
    }

    if (!str_vars.empty())
    {
        instructions.insert(instructions.begin(), str_vars.begin(), str_vars.end());
        instructions.insert(instructions.begin(), TACInstruction(TACOp::ENTER_STR));
    }
}

void TacGenerator::generate_tac(ASTNode *node)
{
    auto handler = handlers.find(node->node_type);
    if (handler != handlers.end())
        handler->second(node);
    // else
    // error("No handler for node type " + node_type_to_string(node->node_type));
}

void TacGenerator::error(const std::string &message)
{
    throw std::runtime_error("Tac Generator Error: " + message);
}

void TacGenerator::generate_tac_func(ASTNode *element)
{
    FuncNode *func = (FuncNode *)element;

    gst->enter_func_scope(func->name);
    instructions.emplace_back(TACOp::FUNC_BEGIN, func->name, func->specifier == Specifier::STATIC ? "static" : "global");

    FuncSymbol *func_symbol = gst->get_func_symbol(func->name);

    for (int i = 0; i < func->params.size(); i++)
        if (i < 6)
            instructions.emplace_back(TACOp::MOV, func->get_param_name(i), registers[i], "", func_symbol->arg_types[i]);

    for (auto &element : func->elements)
        generate_tac(element.get());

    instructions.emplace_back(TACOp::FUNC_END);

    gst->leave_func_scope();
}

void TacGenerator::generate_tac_rtn(ASTNode *element)
{
    FuncSymbol *func = gst->get_func_symbol(gst->get_current_func());
    RtnNode *rtn = (RtnNode *)element;
    std::string result = "";

    if (rtn->value != nullptr)
        result = generate_tac_expr(rtn->value.get());

    instructions.emplace_back(TACOp::RETURN, result, "", "", func->return_type);
}

void TacGenerator::generate_tac_var_decl(ASTNode *element)
{
    VarDeclNode *var_decl = (VarDeclNode *)element;
    Symbol *var_symbol = gst->get_symbol(var_decl->var->name);

    if (var_decl->var->type.is_array())
        return generate_tac_var_array_assign(var_decl->var.get(), var_symbol, var_decl->value.get());

    std::string result = generate_tac_expr(var_decl->value.get());
    TACInstruction instruction(TACOp::ASSIGN, var_decl->var->name, "", result, var_symbol->type);

    // Check if some sort of global/static
    if (var_symbol->linkage != Linkage::None || var_symbol->storage_duration == StorageDuration::Static)
    {
        instruction.arg3 = var_symbol->linkage == Linkage::External ? "global" : "";

        // Place in BSS (if not initialised); otherwise Data
        if (var_decl->value)
            data_vars.emplace_back(instruction);
        else
            bss_vars.emplace_back(instruction);

        return;
    }

    // Only carry on assigning if initialised to a value
    if (!var_decl->value)
        return;

    if (var_decl->var->type.is_struct())
    {
        CompoundLiteral *compound_init = dynamic_cast<CompoundLiteral *>(var_decl->value.get());

        instructions.emplace_back(TACOp::STRUCT_INIT, var_decl->var->name, "", "", var_decl->var->type);

        size_t field_index = 0;
        for (const auto &value : compound_init->values)
        {
            std::string result = generate_tac_expr(value.get());
            instructions.emplace_back(TACOp::MEMBER_ASSIGN,
                                      var_decl->var->name,
                                      var_decl->var->type.get_field_name(field_index),
                                      result,
                                      sem_analyser->infer_type(value.get()));
            field_index++;
        }

        return;
    }

    instructions.emplace_back(instruction);
}

void TacGenerator::generate_tac_var_assign(ASTNode *element)
{
    VarAssignNode *var_assign = (VarAssignNode *)element;
    if (var_assign->var->node_type == NodeType::NODE_VAR)
    {
        VarNode *var = (VarNode *)var_assign->var.get();
        Symbol *var_symbol = gst->get_symbol(var->name);

        if (var->type.is_array())
            return generate_tac_var_array_assign(var, var_symbol, var_assign->value.get());

        std::string result = generate_tac_expr(var_assign->value.get());
        instructions.emplace_back(TACOp::ASSIGN, var->name, "", result, var_symbol->type);
    }
    else if (var_assign->var->node_type == NodeType::NODE_ARRAY_ACCESS)
    {
        ArrayAccessNode *array_access = (ArrayAccessNode *)var_assign->var.get();

        std::string result = generate_tac_expr(var_assign->value.get());
        std::string index = generate_tac_expr(array_access->index.get());

        instructions.emplace_back(TACOp::ASSIGN, array_access->array->name, index, result, array_access->type.get_base_type());
    }
    else if (var_assign->var->node_type == NodeType::NODE_POSTFIX)
    {
        PostfixNode *postfix = dynamic_cast<PostfixNode *>(var_assign->var.get());
        if (postfix->op == TokenType::TOKEN_DOT || postfix->op == TokenType::TOKEN_ARROW)
        {
            std::string base = generate_tac_expr(postfix->value.get());
            std::string result = generate_tac_expr(var_assign->value.get());

            if (postfix->op == TokenType::TOKEN_ARROW)
            {
                std::string deref = gen_new_temp_var();
                instructions.emplace_back(TACOp::DEREF, base, "", deref);
                base = deref;
            }

            instructions.emplace_back(TACOp::MEMBER_ASSIGN,
                                      base,
                                      postfix->field,
                                      result,
                                      sem_analyser->infer_type(var_assign->value.get()));
        }
    }
}

void TacGenerator::generate_tac_if(ASTNode *element)
{
    IfNode *if_stmt = (IfNode *)element;

    std::string label_success = gen_new_label();
    std::string label_failure = gen_new_label();

    // Generate TAC comparison code
    generate_tac_cmp(if_stmt->condition.get(), label_success, label_failure);

    /*
        Previous TAC will jump to "then block" if condition is true
        If condition is false, jump to "else block"/next bit of code if not present
    */
    instructions.emplace_back(TACOp::GOTO, "", "", label_failure);

    // Then block
    instructions.emplace_back(TACOp::LABEL, label_success);
    for (auto &element : if_stmt->then_elements)
        generate_tac(element.get());

    if (if_stmt->else_elements.empty())
    {
        // No else block, jump to end of "if block"
        instructions.emplace_back(TACOp::LABEL, label_failure);
    }
    else
    {
        /*
            Recall we're still in the "then block"
            Therefore jump straight to the end of the entire "if block"
        */
        std::string label_else_end = gen_new_label("else_end");
        instructions.emplace_back(TACOp::GOTO, "", "", label_else_end);

        // Else block
        instructions.emplace_back(TACOp::LABEL, label_failure);
        for (auto &element : if_stmt->else_elements)
            generate_tac(element.get());

        // End of entire "if block"
        instructions.emplace_back(TACOp::LABEL, label_else_end);
    }
}

void TacGenerator::generate_tac_while(ASTNode *element)
{
    WhileNode *while_stmt = (WhileNode *)element;

    std::string label_start = while_stmt->label + "_start";
    std::string label_body = while_stmt->label + "_body";
    std::string label_end = while_stmt->label + "_end";

    instructions.emplace_back(TACOp::LABEL, label_start);

    // Generate CMP
    generate_tac_cmp(while_stmt->condition.get(), label_body, label_end);

    /*
        Previous TAC will jump to "while block" if condition is true
        If condition is false, jump to end of while
    */
    instructions.emplace_back(TACOp::GOTO, "", "", label_end);

    // While block
    instructions.emplace_back(TACOp::LABEL, label_body);
    for (auto &element : while_stmt->elements)
        generate_tac(element.get());

    // Go back to start of while loop (to check condition)
    instructions.emplace_back(TACOp::GOTO, "", "", label_start);

    instructions.emplace_back(TACOp::NOP);
    instructions.emplace_back(TACOp::LABEL, label_end);
}

void TacGenerator::generate_tac_for(ASTNode *element)
{
    ForNode *for_stmt = (ForNode *)element;

    generate_tac(for_stmt->init.get());

    std::string label_start = for_stmt->label + "_start";
    std::string label_body = for_stmt->label + "_body";
    std::string label_post = for_stmt->label + "_post";
    std::string label_end = for_stmt->label + "_end";

    instructions.emplace_back(TACOp::LABEL, label_start);

    generate_tac_cmp(for_stmt->condition.get(), label_body, label_end);

    // For block
    for (auto &element : for_stmt->elements)
        generate_tac(element.get());

    instructions.emplace_back(TACOp::LABEL, label_post);
    generate_tac(for_stmt->post.get());

    instructions.emplace_back(TACOp::GOTO, "", "", label_start);

    instructions.emplace_back(TACOp::LABEL, label_end);
}

void TacGenerator::generate_tac_loop_ctrl(ASTNode *element)
{
    LoopControl *loop_control = (LoopControl *)element;
    if (loop_control->type == TOKEN_BREAK)
        instructions.emplace_back(TACOp::GOTO, "", "", loop_control->label + "_end");
    else if (loop_control->type == TOKEN_CONTINUE)
        instructions.emplace_back(TACOp::GOTO, "", "", loop_control->label + "_post");
}

void TacGenerator::generate_tac_postfix(ASTNode *element)
{
    PostfixNode *postfix = (PostfixNode *)element;

    std::string result = generate_tac_expr(postfix->value.get());

    if (postfix->op == TokenType::TOKEN_INCREMENT)
        instructions.emplace_back(TACOp::ADD, result, "1", result, postfix->type);
    else if (postfix->op == TokenType::TOKEN_DECREMENT)
        instructions.emplace_back(TACOp::SUB, result, "1", result, postfix->type);
}

void TacGenerator::generate_tac_func_call(ASTNode *element)
{
    std::string result = generate_tac_expr(element);
}

std::string TacGenerator::generate_tac_expr(ASTNode *expr)
{
    if (!expr)
        return "";

    if (expr_handlers.find(expr->node_type) != expr_handlers.end())
    {
        return expr_handlers[expr->node_type](expr);
    }
    else
        error("Tac Generation: Invalid expression of type " + node_type_to_string(expr->node_type) + " encountered");
}

void TacGenerator::generate_tac_var_array_assign(VarNode *var_node, Symbol *var_symbol, ASTNode *value)
{
    std::vector<std::string> elements;
    int array_size = var_node->type.get_array_size();
    Type base_type = var_node->type.get_base_type();

    if (base_type == BaseType::CHAR)
    {
        StringLiteral *str = dynamic_cast<StringLiteral *>(value);
        for (size_t i = 0; i < array_size; i++)
            elements.emplace_back(std::to_string(static_cast<int>(str->value[i])));
    }
    else
    {
        CompoundLiteral *array_init = dynamic_cast<CompoundLiteral *>(value);
        for (size_t i = 0; i < array_size; i++)
            elements.emplace_back(generate_tac_expr(array_init->values[i].get()));
    }

    // Assign provided values
    for (size_t i = 0; i < elements.size() && i < (size_t)array_size; i++)
        instructions.emplace_back(TACOp::ASSIGN, var_node->name,
                                  std::to_string(i), elements[i], Type(base_type));

    // Fill remaining space (if any) with zeros
    for (size_t i = elements.size(); i < (size_t)array_size; i++)
        instructions.emplace_back(TACOp::ASSIGN, var_node->name,
                                  std::to_string(i), "0", Type(base_type));
}

void TacGenerator::generate_tac_cmp(ASTNode *condition, const std::string &label_success,
                                    const std::string &label_failure)
{
    /*
        The cases which should be supported include:
        - Binary comparisons ie
            - a < b / a <= b
            - a < b && a < c / a < b || a < c (short circuiting/recursion is needed)
        - Unary comparisons ie
            - !a
        - Boolean comparisons ie
            - true, false
        - Variable comparions ie
            - a
    */

    switch (condition->node_type)
    {
    case NodeType::NODE_BINARY:
    {
        BinaryNode *bin = dynamic_cast<BinaryNode *>(condition);

        if (bin->op == BinOpType::AND)
        {
            std::string go_next_cond = gen_new_label();

            generate_tac_cmp(bin->left.get(), go_next_cond, label_failure);
            instructions.emplace_back(TACOp::LABEL, go_next_cond);
            generate_tac_cmp(bin->right.get(), label_success, label_failure);

            break;
        }

        if (bin->op == BinOpType::OR)
        {
            std::string go_next_cond = gen_new_label();

            generate_tac_cmp(bin->left.get(), label_success, go_next_cond);
            instructions.emplace_back(TACOp::LABEL, go_next_cond);
            generate_tac_cmp(bin->right.get(), label_success, label_failure);

            break;
        }

        TACInstruction if_instruction(TACOp::IF, generate_tac_expr(bin->left.get()), generate_tac_expr(bin->right.get()), label_success, bin->type);
        if_instruction.cmp_op = bin->op;
        instructions.emplace_back(if_instruction);
        break;
    }
    case NodeType::NODE_BOOL:
    {
        BoolLiteral *bool_node = (BoolLiteral *)condition;
        if (bool_node->value)
            instructions.emplace_back(TACOp::GOTO, label_success);
        else
            instructions.emplace_back(TACOp::GOTO, label_failure);
        break;
    }
    case NodeType::NODE_VAR:
    {
        VarNode *var_node = (VarNode *)condition;
        TACInstruction if_instruction(TACOp::IF, var_node->name, "1", label_success, var_node->type);
        if_instruction.cmp_op = BinOpType::EQUAL;
        instructions.emplace_back(if_instruction);
        break;
    }
    case NodeType::NODE_UNARY:
    {
        UnaryNode *unary_node = (UnaryNode *)condition;
        TACInstruction if_instruction(TACOp::IF, generate_tac_expr(unary_node->value.get()), "1", label_success, unary_node->type);
        if_instruction.cmp_op = BinOpType::NOT_EQUAL;
        instructions.emplace_back(if_instruction);
        break;
    }
    default:
    {
        error("Invalid condition type");
    }
    }
}

std::string TacGenerator::generate_tac_expr_var(ASTNode *expr)
{
    return ((VarNode *)expr)->name;
}

std::string TacGenerator::generate_tac_expr_bool(ASTNode *expr)
{
    return ((BoolLiteral *)expr)->value ? "1" : "0";
}

std::string TacGenerator::generate_tac_expr_cast(ASTNode *expr)
{
    CastNode *cast = (CastNode *)expr;

    std::string result = generate_tac_expr(cast->expr.get());

    if (cast->target_type.has_base_type(BaseType::DOUBLE) && cast->expr.get()->node_type == NodeType::NODE_NUMBER)
    {
        std::string const_var = gen_new_const_label();
        gst->declare_const_var(const_var, cast->target_type);

        literal8_vars.emplace_back(TACOp::ASSIGN, const_var, "", result, cast->target_type);

        return const_var;
    }

    std::string temp_var = gen_new_temp_var();
    gst->declare_temp_var(temp_var, cast->target_type);

    instructions.emplace_back(TACOp::CONVERT_TYPE, result, cast->src_type.to_string(), temp_var, cast->target_type);

    return temp_var;
}

std::string TacGenerator::generate_tac_expr_char(ASTNode *expr)
{
    return std::to_string((int)((CharLiteral *)expr)->value);
}

std::string TacGenerator::generate_tac_expr_string(ASTNode *expr)
{
    StringLiteral *str = (StringLiteral *)expr;
    std::string label = gen_new_const_label();

    gst->declare_str_var(label, str->value_type);

    str_vars.emplace_back(TACOp::ASSIGN, label, "", str->value, str->value_type);
    return label;
}

std::string TacGenerator::generate_tac_expr_unary(ASTNode *expr)
{
    UnaryNode *unary = (UnaryNode *)expr;

    std::string result = generate_tac_expr(unary->value.get());

    std::string temp_var = gen_new_temp_var();
    gst->declare_temp_var(temp_var, unary->type);

    if (unary->type.has_base_type(BaseType::DOUBLE))
    {
        if (unary->type.is_pointer())
            error("Cannot take the address of a double yet?");

        gst->declare_const_var("_.Lsign_bit", Type(BaseType::DOUBLE));
        literal8_vars.emplace_back(TACOp::ASSIGN, "_.Lsign_bit", "", "9223372036854775808", Type(BaseType::DOUBLE));
        instructions.emplace_back(convert_UnaryOpType_to_TACOp(unary->op), result, "_.Lsign_bit", temp_var, unary->type);

        return temp_var;
    }

    instructions.emplace_back(convert_UnaryOpType_to_TACOp(unary->op), result, "", temp_var, unary->type);
    return temp_var;
}

std::string TacGenerator::generate_tac_expr_binary(ASTNode *expr)
{
    BinaryNode *bin_node = dynamic_cast<BinaryNode *>(expr);

    std::string arg1 = generate_tac_expr(bin_node->left.get());
    std::string arg2 = generate_tac_expr(bin_node->right.get());

    std::string temp_var = gen_new_temp_var();
    gst->declare_temp_var(temp_var, bin_node->type);

    instructions.emplace_back(convert_BinOpType_to_TACOp(bin_node->op), arg1, arg2, temp_var, bin_node->type);

    return temp_var;
}

std::string TacGenerator::generate_tac_expr_postfix(ASTNode *expr)
{
    PostfixNode *postfix = (PostfixNode *)expr;

    if (postfix->op == TokenType::TOKEN_DOT || postfix->op == TokenType::TOKEN_ARROW)
    {
        std::string base = generate_tac_expr(postfix->value.get());
        std::string temp = gen_new_temp_var();

        if (postfix->op == TokenType::TOKEN_ARROW)
        {
            std::string deref = gen_new_temp_var();
            instructions.emplace_back(TACOp::DEREF, base, "", deref);
            base = deref;
        }

        postfix->type.print();

        gst->declare_temp_var(temp, postfix->type);

        instructions.emplace_back(TACOp::MEMBER_ACCESS,
                                  base,
                                  postfix->field,
                                  temp,
                                  postfix->type);
        return temp;
    }

    std::string result = generate_tac_expr(postfix->value.get());

    if (postfix->op == TokenType::TOKEN_INCREMENT)
        instructions.emplace_back(TACOp::ADD, result, "1", result, postfix->type);
    else if (postfix->op == TokenType::TOKEN_DECREMENT)
        instructions.emplace_back(TACOp::SUB, result, "1", result, postfix->type);

    return result;
}

std::string TacGenerator::generate_tac_expr_array_access(ASTNode *expr)
{
    ArrayAccessNode *array_access = (ArrayAccessNode *)expr;
    std::string base = generate_tac_expr(array_access->array.get());
    std::string index = generate_tac_expr(array_access->index.get());
    std::string temp = gen_new_temp_var();
    gst->declare_temp_var(temp, array_access->type.get_base_type());
    instructions.emplace_back(TACOp::ASSIGN, temp, index, base, array_access->type.get_base_type());
    return temp;
}

std::string TacGenerator::generate_tac_expr_func_call(ASTNode *expr)
{
    FuncCallNode *func = (FuncCallNode *)expr;
    FuncSymbol *func_node = gst->get_func_symbol(func->name);

    if (func->name == "printf")
    {
        std::string fmt_label = gen_new_const_label();
        gst->declare_str_var(fmt_label, Type(BaseType::CHAR));
        StringLiteral *first_arg = (StringLiteral *)func->args[0].get();
        str_vars.emplace_back(TACOp::ASSIGN, fmt_label, "", first_arg->value, Type(BaseType::CHAR));

        // Handle printf arguments
        for (size_t i = 1; i < func->args.size(); i++)
        {
            std::string result = generate_tac_expr(func->args[i].get());
            if (i < 6)
            {
                Type type = sem_analyser->infer_type(func->args[i].get());

                if (type.is_size_8())
                    instructions.emplace_back(TACOp::MOV, x64_registers[i], result, "", type);
                else
                    instructions.emplace_back(TACOp::MOV, registers[i], result, "", type);
            }
        }

        instructions.emplace_back(TACOp::PRINTF, fmt_label, "", "", Type(BaseType::VOID));
        return "";
    }

    // Handle regular function calls
    for (size_t i = 0; i < func->args.size(); i++)
    {
        std::string arg_result = generate_tac_expr(func->args[i].get());
        Type arg_type = sem_analyser->infer_type(func->args[i].get());

        if (i < 6)
        {
            // First 6 arguments go in registers
            instructions.emplace_back(TACOp::MOV, registers[i], arg_result, "", arg_type);
        }
        else
        {
            // Remaining arguments get pushed to stack
            instructions.emplace_back(TACOp::PUSH, arg_result, "", "", arg_type);
        }
    }

    // Handle stack alignment
    int stack_offset = (func->args.size() > 6) ? (func->args.size() - 6) : 0;
    if (stack_offset % 2 != 0 && stack_offset > 0)
        instructions.emplace_back(TACOp::DEALLOC_STACK, "8");

    // Make the function call
    instructions.emplace_back(TACOp::CALL, func->name);

    // Restore stack alignment
    if (stack_offset % 2 != 0 && stack_offset > 0)
        instructions.emplace_back(TACOp::ALLOC_STACK, "8");

    if (func_node->return_type.has_base_type(BaseType::VOID))
        return "";

    // Handle return value
    std::string temp_var = gen_new_temp_var();
    Type return_type = gst->get_func_symbol(func->name)->return_type;
    gst->declare_temp_var(temp_var, return_type);
    instructions.emplace_back(TACOp::MOV, temp_var, "%eax", "", return_type);

    return temp_var;
}

std::string TacGenerator::generate_tac_expr_number(ASTNode *expr)
{
    NumericLiteral *num = (NumericLiteral *)expr;
    if (num->value_type.has_base_type(BaseType::UINT))
        return std::to_string(((UIntegerLiteral *)num)->value);
    else if (num->value_type.has_base_type(BaseType::ULONG))
        return std::to_string(((ULongLiteral *)num)->value);
    else if (num->value_type.has_base_type(BaseType::LONG))
        return std::to_string(((LongLiteral *)num)->value);
    else if (num->value_type.has_base_type(BaseType::INT))
        return std::to_string(((IntegerLiteral *)num)->value);
    else if (num->value_type.has_base_type(BaseType::DOUBLE))
    {
        // All double literals must be placed in a constant section
        std::string const_val = gen_new_const_label();
        gst->declare_const_var(const_val, Type(BaseType::DOUBLE));
        literal8_vars.emplace_back(TACOp::ASSIGN, const_val, "", std::to_string(((DoubleLiteral *)num)->value), Type(BaseType::DOUBLE));
        return const_val;
    }
}

std::string TacGenerator::generate_tac_expr_size_of(ASTNode *expr)
{
    SizeOfNode *sizeof_node = (SizeOfNode *)expr;

    std::string temp_var = gen_new_temp_var();
    gst->declare_temp_var(temp_var, BaseType::INT);

    if (sizeof_node->var)
        instructions.emplace_back(TACOp::ASSIGN, temp_var, "", std::to_string(sizeof_node->var->type.get_size()), BaseType::INT);
    else
        instructions.emplace_back(TACOp::ASSIGN, temp_var, "", std::to_string(sizeof_node->type.get_size()), BaseType::INT);

    return temp_var;
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
        case TACOp::NOT:
            return "NOT";
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
        case TACOp::ENTER_LITERAL8:
            return "ENTER_LITERAL8";
        case TACOp::ENTER_STR:
            return "ENTER_STR";
        case TACOp::CONVERT_TYPE:
            return "CONVERT_TYPE";
        case TACOp::DEREF:
            return "DEREF";
        case TACOp::ADDR_OF:
            return "ADDR_OF";
        case TACOp::PRINTF:
            return "PRINTF";
        case TACOp::STRUCT_INIT:
            return "STRUCT_INIT";
        case TACOp::MEMBER_ACCESS:
            return "MEMBER_ACCESS";
        case TACOp::MEMBER_ASSIGN:
            return "MEMBER_ASSIGN";
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

    if (!instr.type.has_base_type(BaseType::VOID))
        str += " (" + instr.type.to_string() + ")";

    return str;
}
