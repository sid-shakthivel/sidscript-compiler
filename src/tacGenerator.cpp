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
    REGISTER_EXPR_HANDLER(NODE_NULL, generate_tac_expr_null);
}

std::string TacGenerator::gen_new_temp_var()
{
    return "t" + std::to_string(tempCounter++);
}

std::string TacGenerator::gen_new_label(const std::string &label)
{
    return ".L" + label + std::to_string(labelCounter++);
}

std::string TacGenerator::gen_new_const_label()
{
    return ".L" + std::string("const_") + std::to_string(constCounter++);
}

void TacGenerator::generate_all_tac(std::shared_ptr<ProgramNode> &program)
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

    // std::cout << "sorted " << node_type_to_string(node->node_type) << "\n";

    // else
    // error("No handler for node type " + node_type_to_string(node->node_type));
}

void TacGenerator::error(const std::string &message, const SourceLocation &loc)
{
    throw std::runtime_error("Tac Generator Error: " + message);
}

void TacGenerator::generate_tac_func(ASTNode *element)
{
    FuncNode *func = (FuncNode *)element;

    gst->enter_func_scope(func->name);

    std::string status = "";
    if (contains_specifier(func->specifiers, Specifier::STATIC))
        status = "static";
    if (contains_specifier(func->specifiers, Specifier::PUBLIC) || func->name == "main")
        status = "global";

    instructions.emplace_back(TACOp::FUNC_BEGIN, func->name, status);

    FuncSymbol *func_symbol = gst->get_func_symbol(func->name);

    for (int i = 0; i < func->params.size(); i++)
        if (i < 6)
            instructions.emplace_back(TACOp::MOV_BETWEEN_REG, func->get_param_name(i), x64_registers[i], "store", func_symbol->arg_types[i]);

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
    /*
        VarDecl is used to declare a variable
        It can be a global, local, or static variable (and as such all cases must be handled)
        Struct declarations must also be handled
    */
    VarDeclNode *var_decl = (VarDeclNode *)element;
    Symbol *var_symbol = gst->get_symbol(var_decl->var->name);

    if (var_decl->var->type.is_array() && var_decl->value != nullptr)
        return generate_tac_var_array_assign(var_decl->var.get(), var_symbol, var_decl->value.get());

    std::string result = generate_tac_expr(var_decl->value.get());
    TACInstruction instruction(TACOp::ASSIGN, var_decl->var->name, "", result, var_symbol->type);

    // std::cout << "ok here for " << var_decl->var->name << "\n";

    // Check if some sort of global/static
    if (var_symbol->linkage != Linkage::None || var_symbol->storage_duration == StorageDuration::Static)
    {
        if (contains_specifier(var_symbol->specifiers, Specifier::PUBLIC))
            instruction.arg3 = "global";

        // Place in BSS (if not initialised); otherwise Data
        if (var_decl->value)
        {
            if (var_decl->var->type.is_struct())
                return generate_tac_struct_assign(var_decl->var.get(), var_decl->value.get(), "data");

            data_vars.emplace_back(instruction);
        }
        else
            bss_vars.emplace_back(instruction);

        return;
    }

    /*
        Only carry on assigning if initialised to a value
        This check is only performed after checking if a variable is global/static as they can be initialised without a value
    */
    if (!var_decl->value)
        return;

    if (var_decl->var->type.is_struct() && !var_decl->var->type.is_pointer())
        return generate_tac_struct_assign(var_decl->var.get(), var_decl->value.get());

    instructions.emplace_back(instruction);
}

void TacGenerator::generate_tac_var_assign(ASTNode *element)
{
    /*
        VarAssign is used to assign a value to a variable (once they have been declared)
        There are a number of cases:
        - Assigning to a variable i.e. test = 5;
        - Assigning to a array element i.e. arr[2] = 5;
        - Assigning to a struct member i.e. test.struct_member = 5;
        - Assigning to a struct member via a pointer i.e. test->struct_member = 5;
    */

    VarAssignNode *var_assign = (VarAssignNode *)element;
    if (var_assign->var->node_type == NodeType::NODE_VAR)
    {
        VarNode *var = (VarNode *)var_assign->var.get();
        Symbol *var_symbol = gst->get_symbol(var->name);

        if (var->type.is_array())
            return generate_tac_var_array_assign(var, var_symbol, var_assign->value.get());

        if (var->type.is_struct())
            return generate_tac_struct_assign(var, var_assign->value.get());

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

        if (!(postfix->op == TokenType::TOKEN_DOT || postfix->op == TokenType::TOKEN_ARROW))
            error("Cannot assign to this postfix expression", postfix->loc);

        std::string result = generate_tac_expr(var_assign->value.get());

        auto [struct_base, final_offset] = compute_struct_access_offset(postfix);

        if (postfix->op == TokenType::TOKEN_DOT)
            instructions.emplace_back(TACOp::ASSIGN, struct_base, final_offset, result, sem_analyser->infer_type(var_assign->value.get()));
        else if (postfix->op == TokenType::TOKEN_ARROW)
            instructions.emplace_back(TACOp::ASSIGN_DEREF, struct_base, final_offset, result, postfix->type);
    }
    else if (var_assign->var->node_type == NodeType::NODE_UNARY)
    {
        UnaryNode *unary = dynamic_cast<UnaryNode *>(var_assign->var.get());

        if (unary->op != UnaryOpType::DEREF)
            error("Cannot assign to this unary expression", unary->loc);

        std::string var = generate_tac_expr(unary->value.get());
        std::string result = generate_tac_expr(var_assign->value.get());

        instructions.emplace_back(TACOp::ASSIGN_DEREF, var, "", result, unary->type);
    }
    else
        error("Invalid lvalue in assignment", var_assign->var->loc);
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

    if (expr->node_type == NodeType::NODE_AGGREGATE_INIT)
        return "";

    if (expr_handlers.find(expr->node_type) != expr_handlers.end())
        return expr_handlers[expr->node_type](expr);
    else
        error("Tac Generation: Invalid expression of type " + node_type_to_string(expr->node_type) + " encountered", expr->loc);
}

void TacGenerator::generate_tac_var_array_assign(VarNode *var_node, Symbol *var_symbol, ASTNode *value)
{
    std::vector<std::string> elements;
    int array_size = var_node->type.get_array_length();
    Type base_type = var_node->type.get_base_type();

    if (base_type == BaseType::CHAR)
    {
        StringLiteral *str = dynamic_cast<StringLiteral *>(value);
        for (size_t i = 0; i < array_size; i++)
            elements.emplace_back(std::to_string(static_cast<int>(str->value[i])));
    }
    else
    {
        AggregateLiteral *array_init = dynamic_cast<AggregateLiteral *>(value);
        for (size_t i = 0; i < array_size; i++)
            elements.emplace_back(generate_tac_expr(array_init->values[i].get()));
    }

    // Assign provided values
    for (size_t i = 0; i < elements.size() && i < (size_t)array_size; i++)
        instructions.emplace_back(TACOp::ASSIGN, var_node->name, std::to_string(i), elements[i], Type(base_type));

    // Fill remaining space (if any) with zeros
    for (size_t i = elements.size(); i < (size_t)array_size; i++)
        instructions.emplace_back(TACOp::ASSIGN, var_node->name, std::to_string(i), "0", Type(base_type));
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
        error("Invalid condition type", condition->loc);
    }
    }
}

void TacGenerator::generate_tac_struct_assign(VarNode *var, ASTNode *value, std::string memory_region)
{
    AggregateLiteral *compound_init = dynamic_cast<AggregateLiteral *>(value);

    if (memory_region == "text")
        instructions.emplace_back(TACOp::STRUCT_INIT, var->name, "", "", var->type);
    else if (memory_region == "data")
        data_vars.emplace_back(TACOp::STRUCT_INIT, var->name, "", "", var->type);

    Symbol *struct_sym = gst->get_symbol(var->name);

    size_t field_index = 0;
    for (const auto &value : compound_init->values)
    {
        std::string result = generate_tac_expr(value.get());
        std::string field_name = var->type.get_field_name(field_index);
        int offset = var->type.get_field_offset(field_name);
        Type result_type = sem_analyser->infer_type(value.get());

        if (result_type.is_array())
        {
            if (value.get()->node_type == NodeType::NODE_AGGREGATE_INIT)
            {
                AggregateLiteral *aggregate_init = dynamic_cast<AggregateLiteral *>(value.get());
                for (size_t i = 0; i < aggregate_init->values.size(); i++)
                {
                    result = generate_tac_expr(aggregate_init->values[i].get());
                    int arr_offset = offset + (i * result_type.get_base_size());
                    int final_offset = struct_sym->stack_offset + arr_offset;

                    if (memory_region == "text")
                        instructions.emplace_back(TACOp::ASSIGN, var->name, std::to_string(final_offset), result, aggregate_init->type.get_base_type());
                    else if (memory_region == "data")
                        data_vars.emplace_back(TACOp::ASSIGN, var->name, std::to_string(final_offset), result, aggregate_init->type.get_base_type());
                }
            }
            field_index++;
        }

        else
        {
            int final_offset = struct_sym->stack_offset + offset;

            if (memory_region == "text")
                instructions.emplace_back(TACOp::ASSIGN, var->name, std::to_string(final_offset), result, result_type);
            else if (memory_region == "data")
            {
                TACInstruction instruction(TACOp::ASSIGN, var->name, std::to_string(final_offset), result, result_type);
                instruction.arg3 = field_index == 0 ? "" : "struct_not_first";
                data_vars.emplace_back(instruction);
            }

            field_index++;
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
        return get_const_label(std::stod(result));

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
            error("Cannot take the address of a double yet?", unary->loc);

        instructions.emplace_back(convert_UnaryOpType_to_TACOp(unary->op), result, get_const_label(9223372036854775808), temp_var, unary->type);

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
        std::cout << "are we here?\n";

        std::string temp = gen_new_temp_var();
        gst->declare_temp_var(temp, postfix->type);

        auto [struct_base, final_offset] = compute_struct_access_offset(postfix);

        instructions.emplace_back(TACOp::ASSIGN, temp, final_offset,
                                  struct_base, postfix->type);
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

    // Handle regular function calls
    for (size_t i = 0; i < func->args.size(); i++)
    {
        std::string arg_result = generate_tac_expr(func->args[i].get());
        Type arg_type = sem_analyser->infer_type(func->args[i].get());

        /*
            The first 6 arguments go in registers
            Remaining arguments get pushed onto the stack
        */

        if (i < 6)
            instructions.emplace_back(TACOp::MOV_BETWEEN_REG, arg_result, x64_registers[i], "load", arg_type);
        else
            instructions.emplace_back(TACOp::PUSH, arg_result, "", "", arg_type);
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

    bool found = (std::find(void_func_names.begin(), void_func_names.end(), func->name) != void_func_names.end());
    // No check here if the return_type isn't null (it could be if its a function not defined ie printf)

    if (found)
        return "";

    // Handle return value
    std::string temp_var = gen_new_temp_var();
    Type return_type = gst->get_func_symbol(func->name)->return_type;
    gst->declare_temp_var(temp_var, return_type);
    instructions.emplace_back(TACOp::MOV_BETWEEN_REG, temp_var, "%rax", "store", return_type);

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
        return get_const_label(((DoubleLiteral *)num)->value);
}

std::string TacGenerator::generate_tac_expr_size_of(ASTNode *expr)
{
    SizeOfNode *sizeof_node = (SizeOfNode *)expr;

    std::string temp_var = gen_new_temp_var();
    gst->declare_temp_var(temp_var, BaseType::INT);

    /*
        If the variable exists, get the size of it
        Otherwise get the size of the type
    */
    instructions.emplace_back(TACOp::ASSIGN, temp_var, "", sizeof_node->var ? std::to_string(sizeof_node->var->type.get_size()) : std::to_string(sizeof_node->type.get_size()), BaseType::INT);

    return temp_var;
}

std::string TacGenerator::generate_tac_expr_null(ASTNode *expr)
{
    return "0";
}

std::string TacGenerator::get_const_label(double value)
{
    /*
        Attempt to find the constant label for the value
        Recall all double literals must be placed in a constant section
        Otherwise make a new label, declare + assign it and then return
    */
    if (const_labels.find(value) != const_labels.end())
        return const_labels[value];

    std::string const_val = gen_new_const_label();
    gst->declare_const_var(const_val, Type(BaseType::DOUBLE));
    literal8_vars.emplace_back(TACOp::ASSIGN, const_val, "", std::to_string(value), Type(BaseType::DOUBLE));

    const_labels.insert({value, const_val});

    return const_val;
}

std::tuple<std::string, std::string> TacGenerator::compute_struct_access_offset(PostfixNode *postfix)
{
    /*
        This is a helper function to compute struct member access offset
        To correctly identify the slot in memory of a field
    */

    std::string struct_base;
    std::string final_field_offset;

    // Handle pointer dereference if required
    struct_base = postfix->struct_name;

    Symbol *struct_symbol = gst->get_symbol(postfix->struct_name);
    int field_offset = struct_symbol->type.get_field_offset(postfix->field_name);
    final_field_offset = std::to_string(field_offset);

    /*
        If the value is an array access node, this adds a bit more complexity
        The index could be a number or it could be a variable i.e.
        - example.arr[0]
        - example.arr[i]
        Therefore we need to come up with a mechanism to get the correct offset
        This is done by first getting the index  value and scaling it by the size of the base type
        Then we add this to the field offset
    */
    if (postfix->value.get()->node_type == NodeType::NODE_ARRAY_ACCESS)
    {
        ArrayAccessNode *array_access = (ArrayAccessNode *)postfix->value.get();

        std::string index = generate_tac_expr(array_access->index.get());

        bool is_number = !index.empty() &&
                         std::all_of(index.begin(), index.end(), ::isdigit);

        /*
            If it's a number, then we can scale it by the size of the type here
            Instead of emitting assembly as it reduces code size
        */

        int arr_element_type_size = postfix->type.get_size();

        if (is_number)
        {
            int scaled_index = std::stoi(index) * arr_element_type_size;
            final_field_offset = std::to_string(field_offset + scaled_index);
        }
        else
        {
            std::string scaled_index = gen_new_temp_var();
            gst->declare_temp_var(scaled_index, Type(BaseType::INT));

            instructions.emplace_back(TACOp::MUL, index, std::to_string(arr_element_type_size), scaled_index, Type(BaseType::INT));

            std::string arr_field_offset = gen_new_temp_var();
            gst->declare_temp_var(arr_field_offset, Type(BaseType::INT));

            instructions.emplace_back(TACOp::ADD, std::to_string(field_offset), scaled_index, arr_field_offset, postfix->type);

            final_field_offset = arr_field_offset;
        }
    }

    /*
        So far we've calculated the field offset assuming the struct is at memory address 0
        We now need to add the struct base address

        Again, if we have a number, no need to emit assembly
    */

    bool is_number = !final_field_offset.empty() &&
                     std::all_of(final_field_offset.begin(), final_field_offset.end(), ::isdigit);

    if (is_number)
    {
        if (postfix->op == TOKEN_ARROW)
            return {struct_base, final_field_offset};

        final_field_offset = std::to_string(std::stoi(final_field_offset) + struct_symbol->stack_offset);
        return {struct_base, final_field_offset};
    }

    std::string final_offset = gen_new_temp_var();
    gst->declare_temp_var(final_offset, Type(BaseType::INT));

    instructions.emplace_back(TACOp::ADD, std::to_string(struct_symbol->stack_offset), final_field_offset, final_offset, postfix->type);

    return {struct_base, final_offset};
}

void TacGenerator::print_all_tac()
{
    for (auto &instr : instructions)
        std::cout << gen_tac_str(instr) << std::endl;
}

std::string TacGenerator::gen_tac_str(const TACInstruction &instr)
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
        case TACOp::MOV_BETWEEN_REG:
            return "MOV_BETWEEN_REG";
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
        case TACOp::STRUCT_INIT:
            return "STRUCT_INIT";
        case TACOp::ASSIGN_DEREF:
            return "ASSIGN_DEREF";
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
