#include "../include/semanticAnalyser.h"

#include <iostream>

#define REGISTER_HANDLER(node_type, fn) handlers[node_type] = [this](ASTNode *node) { fn(node); }

SemanticAnalyser::SemanticAnalyser(std::shared_ptr<GlobalSymbolTable> gst) : gst(gst)
{
    // Initialise analysers
    REGISTER_HANDLER(NodeType::NODE_FUNCTION, analyse_func);
    REGISTER_HANDLER(NodeType::NODE_VAR_DECL, analyse_var_decl);
    REGISTER_HANDLER(NodeType::NODE_VAR_ASSIGN, analyse_var_assign);
    REGISTER_HANDLER(NodeType::NODE_RETURN, analyse_rtn);
    REGISTER_HANDLER(NodeType::NODE_IF, analyse_if_stmt);
    REGISTER_HANDLER(NodeType::NODE_WHILE, analyse_while_stmt);
    REGISTER_HANDLER(NodeType::NODE_FOR, analyse_for_stmt);
    REGISTER_HANDLER(NodeType::NODE_LOOP_CONTROL, analyse_loop_control);
    REGISTER_HANDLER(NodeType::NODE_FUNC_CALL, analyse_func_call);
    REGISTER_HANDLER(NodeType::NODE_CAST, analyse_cast);
    REGISTER_HANDLER(NodeType::NODE_BINARY, analyse_binary);
    REGISTER_HANDLER(NodeType::NODE_UNARY, analyse_unary);
    REGISTER_HANDLER(NodeType::NODE_VAR, analyse_var);
    REGISTER_HANDLER(NodeType::NODE_POSTFIX, analyse_postfix);
    REGISTER_HANDLER(NodeType::NODE_STRUCT_DECL, analyse_struct_decl);
    REGISTER_HANDLER(NodeType::NODE_AGGREGATE_INIT, analyse_aggregate_literal);
}

void SemanticAnalyser::analyse(std::shared_ptr<ProgramNode> &program)
{
    for (auto &decl : program->decls)
        analyse_node(decl.get());
}

void SemanticAnalyser::analyse_func(ASTNode *node)
{
    FuncNode *func_node = (FuncNode *)node;

    // Find and create new function symbol using params
    std::vector<Type> arg_types;
    for (auto &param : func_node->params)
    {
        VarDeclNode *test = dynamic_cast<VarDeclNode *>(param.get());
        arg_types.push_back(test->var->type);
    }

    std::unique_ptr<FuncSymbol> func_symbol = std::make_unique<FuncSymbol>(func_node->name, func_node->params.size(), arg_types, func_node->return_type);
    std::shared_ptr<SymbolTable> symbol_table = std::make_unique<SymbolTable>();

    gst->create_new_func(func_node->name, std::move(func_symbol), symbol_table);
    gst->enter_func_scope(func_node->name);

    for (auto &param : func_node->params)
    {
        VarDeclNode *param_decl = dynamic_cast<VarDeclNode *>(param.get());
        gst->declare_var(param_decl->var.get());
    }

    for (auto &element : func_node->elements)
        analyse_node(element.get());

    // Ensure return is present at end of function
    if (!func_node->return_type.has_base_type(BaseType::VOID))
        if (func_node->elements.back()->node_type != NodeType::NODE_RETURN)
            error("Return statement missing from function " + func_node->name);

    gst->leave_func_scope();
}

void SemanticAnalyser::analyse_node(ASTNode *node)
{
    auto handler = handlers.find(node->node_type);
    if (handler != handlers.end())
        handler->second(node);
    // else
    // error("Unknown node of type " + node_type_to_string(node->node_type) + " encountered");
}

void SemanticAnalyser::analyse_var_decl(ASTNode *node)
{
    VarDeclNode *var_decl_node = (VarDeclNode *)node;

    // Global variables are only allowed to have constant values ie 5 rather then expressions
    if (gst->is_global_scope())
        if (var_decl_node->value != nullptr)
            if (var_decl_node->value->node_type == NodeType::NODE_BINARY || var_decl_node->value->node_type == NodeType::NODE_UNARY)
                error("Global variable " + var_decl_node->var->name + " must have constant value");

    // Add each field from the struct to the type of the variable
    if (var_decl_node->var->type.is_struct())
    {
        std::string struct_name = var_decl_node->var->type.get_struct_name();

        for (const auto &[field_name, field_type] : struct_table[struct_name])
            var_decl_node->var->type.add_field(field_name, field_type);
    }

    if (var_decl_node->value != nullptr)
    {
        analyse_node(var_decl_node->value.get());

        Type var_type = var_decl_node->var->type;
        Type value_type = infer_type(var_decl_node->value.get());

        if (var_type.is_array() && var_type.has_base_type(BaseType::CHAR))
        {
            if (var_decl_node->value->node_type != NodeType::NODE_STRING)
                error("String initialisation of " + var_decl_node->var->name + " requires string literal");

            StringLiteral *string_literal = dynamic_cast<StringLiteral *>(var_decl_node->value.get());

            // +1 due to null terminator
            if (string_literal->value.size() + 1 > var_type.get_size())
                error("Too many characters in string initialisation of " + var_decl_node->var->name);
        }
        else if (var_type.is_array() || var_type.is_struct())
            if (var_decl_node->value->node_type != NodeType::NODE_AGGREGATE_INIT)
                error("Compound initialisation of " + var_decl_node->var->name + " requires compound literal");

        validate_type_assignment(var_type, var_decl_node->value, var_decl_node->var->name);
    }

    gst->declare_var(var_decl_node->var.get());
    analyse_var(var_decl_node->var.get());
}

void SemanticAnalyser::analyse_aggregate_literal(ASTNode *node, const Type &var_type)
{
    AggregateLiteral *aggregate_literal = (AggregateLiteral *)node;

    Type type_to_cmp = var_type.has_base_type(BaseType::VOID) ? aggregate_literal->type : var_type;

    if (type_to_cmp.is_array())
    {
        /*
            This is used when declaring an array
            - int arr[3] = {1, 2, 3};
        */
        if (aggregate_literal->values.size() > type_to_cmp.get_size())
            error("Too many elements in array initialisation");

        for (auto &element : aggregate_literal->values)
        {
            analyse_node(element.get());

            if (!infer_type(element.get()).has_base_type(type_to_cmp.get_base_type()))
                error("Type in array initialisation of some variable doesn't match");
        }
    }
    else if (type_to_cmp.is_struct())
    {
        std::string struct_name = type_to_cmp.get_struct_name();

        if (struct_table.find(struct_name) == struct_table.end())
            error("Struct '" + struct_name + "' not defined");

        std::unordered_map<std::string, Type> struct_fields = struct_table[struct_name];

        if (struct_fields.size() != aggregate_literal->values.size())
            error("Struct '" + struct_name + "' has " + std::to_string(struct_fields.size()) +
                  " fields, but " + std::to_string(aggregate_literal->values.size()) + " were provided");

        int i = 0;
        for (const auto &[field_name, field_type] : struct_fields)
        {
            analyse_node(aggregate_literal->values[i].get());

            validate_type_assignment(field_type, aggregate_literal->values[i],
                                     "in initialisation of struct field '" + field_name + "'");
            i++;
        }
    }
    else
    {
        /*
            This will likely be used when declaring a array within a struct
        */
        for (auto &element : aggregate_literal->values)
            analyse_node(element.get());

        Type arr_type = infer_type(aggregate_literal->values[0].get());
        arr_type.add_array_dimension(aggregate_literal->values.size());
        aggregate_literal->type = arr_type;
    }
}

void SemanticAnalyser::analyse_var_assign(ASTNode *node)
{
    VarAssignNode *var_assign_node = (VarAssignNode *)node;

    if (var_assign_node->var->node_type == NodeType::NODE_VAR)
    {
        VarNode *var = (VarNode *)var_assign_node->var.get();
        var->name = gst->check_var_defined(var->name);

        analyse_node(var_assign_node->value.get());
        Type var_type = gst->get_symbol(var->name)->type;
        Type value_type = infer_type(var_assign_node->value.get());

        var->type = var_type;

        if (var_type.is_struct())
            analyse_aggregate_literal(var_assign_node->value.get(), var_type);
        else
            validate_type_assignment(var_type, var_assign_node->value, var->name);
    }
    else if (var_assign_node->var->node_type == NodeType::NODE_ARRAY_ACCESS)
    {
        ArrayAccessNode *array_access = dynamic_cast<ArrayAccessNode *>(var_assign_node->var.get());

        Symbol *symbol = gst->get_symbol(array_access->array->name);

        if (symbol == nullptr)
            error("Array '" + array_access->array->name + "' not defined");
        if (!symbol->type.is_array())
            error("Array '" + array_access->array->name + "' is not an array");

        Type value_type = infer_type(var_assign_node->value.get());

        if (symbol->type.has_base_type(BaseType::CHAR) && !symbol->type.is_pointer())
            if (var_assign_node->value->node_type == NodeType::NODE_STRING)
                error("Cannot assign string literal to single char element in array '" + array_access->array->name + "'");

        if (!(Type(symbol->type.get_base_type())).can_assign_from(value_type)) // Check whether the .get_base_type() is even needed here
            error("Cannot assign " + value_type.to_string() + " to array element of type " + Type(symbol->type.get_base_type()).to_string() + " in array '" + array_access->array->name + "'");
    }
    else if (var_assign_node->var->node_type == NodeType::NODE_POSTFIX)
    {
        analyse_node(var_assign_node->var.get());

        Type left = infer_type(var_assign_node->var.get());

        validate_type_assignment(left, var_assign_node->value, "in assignment");
    }
}

void SemanticAnalyser::analyse_rtn(ASTNode *node)
{
    RtnNode *rtn_node = (RtnNode *)node;
    FuncSymbol *func = gst->get_func_symbol(gst->get_current_func());
    Type expected_rtn_type = func->return_type;

    if (expected_rtn_type.is_void())
    {
        if (rtn_node->value != nullptr)
            error("Function '" + func->name + "' has void return type but return statement provides a value");
    }
    else
    {
        if (rtn_node->value == nullptr)
            error("Function '" + func->name + "' must return a value of type " + expected_rtn_type.to_string());
        else
        {
            analyse_node(rtn_node->value.get());
            validate_type_assignment(expected_rtn_type, rtn_node->value, "return from '" + func->name + "'");
        }
    }
}

void SemanticAnalyser::analyse_if_stmt(ASTNode *node)
{
    IfNode *if_node = (IfNode *)node;

    analyse_node(if_node->condition.get());
    infer_type(if_node->condition.get());

    gst->enter_scope();
    for (auto &stmt : if_node->then_elements)
        analyse_node(stmt.get());
    gst->exit_scope();

    if (!if_node->else_elements.empty())
    {
        gst->enter_scope();
        for (auto &stmt : if_node->else_elements)
            analyse_node(stmt.get());
        gst->exit_scope();
    }
}

void SemanticAnalyser::analyse_while_stmt(ASTNode *node)
{
    WhileNode *while_node = (WhileNode *)node;
    analyse_node(while_node->condition.get());

    std::string label = gen_new_loop_label();
    while_node->label = label;

    enter_loop_scope(label);
    gst->enter_scope();

    for (auto &stmt : while_node->elements)
        analyse_node(stmt.get());

    gst->exit_scope();
    exit_loop_scope();
}

void SemanticAnalyser::analyse_for_stmt(ASTNode *node)
{
    ForNode *for_node = (ForNode *)node;

    std::string label = gen_new_loop_label();
    for_node->label = label;

    enter_loop_scope(label);
    gst->enter_scope();

    analyse_node(for_node->init.get());
    analyse_node(for_node->condition.get());
    analyse_node(for_node->post.get());

    for (auto &stmt : for_node->elements)
        analyse_node(stmt.get());

    gst->exit_scope();
    exit_loop_scope();
}

void SemanticAnalyser::analyse_binary(ASTNode *node)
{
    BinaryNode *bin_node = (BinaryNode *)node;
    analyse_node(bin_node->left.get());
    analyse_node(bin_node->right.get());

    bin_node->type = infer_type(bin_node);
}

void SemanticAnalyser::analyse_unary(ASTNode *node)
{
    UnaryNode *unary_node = (UnaryNode *)node;

    analyse_node(unary_node->value.get());
    Type expr_type = infer_type(unary_node->value.get());

    if (unary_node->op == UnaryOpType::ADDR_OF || unary_node->op == UnaryOpType::DEREF)
        if (unary_node->value->node_type != NodeType::NODE_VAR)
            error("Can only take address of variables");

    if (unary_node->op == UnaryOpType::ADDR_OF)
        unary_node->type = Type(expr_type.get_base_type(), expr_type.get_ptr_depth() + 1);
    else if (unary_node->op == UnaryOpType::DEREF)
        unary_node->type = Type(expr_type.get_base_type(), expr_type.get_ptr_depth() - 1);
    else
        unary_node->type = expr_type;
}

void SemanticAnalyser::analyse_var(ASTNode *node)
{
    VarNode *var_node = (VarNode *)node;
    var_node->name = gst->check_var_defined(var_node->name);
    var_node->type = gst->get_symbol(var_node->name)->type;
}

std::string SemanticAnalyser::gen_new_loop_label()
{
    return ".Lloop_" + std::to_string(loop_label_counter++);
}

void SemanticAnalyser::analyse_loop_control(ASTNode *node)
{
    LoopControl *loop_control = (LoopControl *)node;
    loop_control->label = loop_scopes.top();
}

void SemanticAnalyser::analyse_func_call(ASTNode *node)
{
    FuncCallNode *fc_node = (FuncCallNode *)node;
    FuncSymbol *func = gst->get_func_symbol(fc_node->name);

    if (fc_node->name == "printf")
        return;

    if (func == nullptr)
        error("Function '" + fc_node->name + "' not defined");

    if (func->arg_count != fc_node->args.size())
        error("Function '" + fc_node->name + "' has " + std::to_string(func->arg_count) +
              " arguments, but " + std::to_string(fc_node->args.size()) + " were provided");

    for (int i = 0; i < fc_node->args.size(); i++)
    {
        Type param_type = func->arg_types[i];

        analyse_node(fc_node->args[i].get());
        validate_type_assignment(param_type, fc_node->args[i], "in call to '" + fc_node->name + "'");
    }
}

/*
    These loop scope methods have one distinct purpose:
    - When there are nested loops, it's difficult to determine which loop
      loop controls (break/continue) should use hence the loop_scopes is a stack of all loops
      so the most inner loop is used for loop controls
*/

void SemanticAnalyser::enter_loop_scope(const std::string &label)
{
    loop_scopes.emplace(label);
}

void SemanticAnalyser::exit_loop_scope()
{
    if (loop_scopes.empty())
        error("No scope to exit when attempting to exit loop scope");

    loop_scopes.pop();
}

void SemanticAnalyser::analyse_cast(ASTNode *node)
{
    CastNode *cast_node = (CastNode *)node;
    Type src_type = infer_type((ASTNode *)(cast_node->expr.get()));

    if (!src_type.can_convert_to(cast_node->target_type))
        error("Cannot cast " + src_type.to_string() + " to " + cast_node->target_type.to_string());

    cast_node->src_type = src_type;
}

void SemanticAnalyser::analyse_struct_decl(ASTNode *node)
{
    StructDeclNode *struct_decl_node = (StructDeclNode *)node;

    if (struct_table.find(struct_decl_node->name) != struct_table.end())
        error("Struct '" + struct_decl_node->name + "' already defined");

    std::unordered_map<std::string, Type> members;

    for (const auto &member : struct_decl_node->members)
    {
        VarDeclNode *member_decl = dynamic_cast<VarDeclNode *>(member.get());
        Type member_type = member_decl->var->type;

        if (members.find(member_decl->var->name) != members.end())
            error("Duplicate member '" + member_decl->var->name + "' in struct '" + struct_decl_node->name + "'");

        if (member_type.is_struct() && !member_type.is_pointer() && struct_decl_node->name == member_type.get_struct_name())
            error("Struct member '" + member_decl->var->name + "' cannot be a struct of itself");

        members[member_decl->var->name] = member_decl->var->type;
    }

    struct_table[struct_decl_node->name] = std::move(members);
}

void SemanticAnalyser::analyse_postfix(ASTNode *node)
{
    PostfixNode *postfix_node = (PostfixNode *)node;

    if (postfix_node->op == TOKEN_DOT || postfix_node->op == TOKEN_ARROW)
    {
        /*
            This is used when accessing a struct member
            - struct s s1;
            - struct *s s2;
            - s1.x = 5;
            - s2->x = 10;

            This should set the type of the postfix node to the type of the struct member
        */

        // Infer the type of the struct member
        Type rtn_type = infer_type(postfix_node->value.get(), postfix_node->struct_name);

        // Get the symbol for the struct variable
        Symbol *symbol = gst->get_symbol(postfix_node->struct_name);

        // Type expr_type = infer_type(postfix_node->value.get(), postfix_node->field);
        Type expr_type = symbol->type;

        if (postfix_node->op == TOKEN_DOT && (!expr_type.is_struct() || expr_type.is_pointer()))
            error("Cannot access member of non-struct type");

        if (postfix_node->op == TOKEN_ARROW && (!expr_type.is_struct() || !expr_type.is_pointer()))
            error("Cannot access member of non-pointer type");

        postfix_node->type = rtn_type;

        return;
    }

    analyse_node(postfix_node->value.get());
    postfix_node->type = infer_type(postfix_node->value.get());
}

void SemanticAnalyser::validate_type_assignment(const Type &target_type, std::unique_ptr<ASTNode> &source_expr, const std::string &context)
{
    Type source_type = infer_type(source_expr.get());

    if (target_type == source_type)
        return;

    // Allow array-to-pointer decay
    if (target_type.is_pointer() && source_type.is_array())
        if (target_type.get_base_type() == source_type.get_base_type())
            return;

    // Try literal-only, exact rewrite
    if (target_type.can_assign_from(source_type) && try_promote_literal(source_expr, target_type))
        return;

    // If still incompatible, fail
    error("Cannot assign " + source_type.to_string() +
          " to " + target_type.to_string() + " in " + context);
}

void SemanticAnalyser::error(const std::string &message)
{
    throw std::runtime_error("Semantic Error: " + message);
}

Type SemanticAnalyser::infer_type(ASTNode *node, std::optional<std::string> field_name)
{
    switch (node->node_type)
    {
    case NodeType::NODE_NUMBER:
        return ((NumericLiteral *)node)->value_type;
    case NodeType::NODE_BOOL:
        return ((BoolLiteral *)node)->value_type;
    case NodeType::NODE_VAR:
    {
        VarNode *var_node = (VarNode *)node;

        if (field_name.has_value())
        {
            /*
                First get the struct symbol for the struct variable
                Then check whether the struct has the appropriate field
                If it does -> return the type of the field (using the struct table)
            */
            Symbol *struct_symbol = gst->get_symbol(field_name.value());

            if (struct_symbol == nullptr)
                error("Variable '" + field_name.value() + "' not defined");

            std::string struct_name = struct_symbol->type.get_struct_name();

            if (struct_table.find(struct_name) == struct_table.end())
                error("Struct '" + struct_name + "' not defined");

            std::unordered_map<std::string, Type> struct_fields = struct_table[struct_name];

            if (struct_fields.find(var_node->name) == struct_fields.end())
                error("Struct '" + struct_name + "' has no field '" + var_node->name + "'");

            return struct_fields[var_node->name];
        }
        else
        {
            auto rtn = gst->get_symbol(var_node->name);

            if (rtn == nullptr)
                error("Variable '" + var_node->name + "' not defined");

            return rtn->type;
        }
    }
    case NodeType::NODE_FUNC_CALL:
    {
        FuncSymbol *func = gst->get_func_symbol(((FuncCallNode *)node)->name);
        return func->return_type;
    }
    case NodeType::NODE_CHAR:
        return ((CharLiteral *)node)->value_type;
    case NodeType::NODE_STRING:
        return ((StringLiteral *)node)->value_type;
    case NodeType::NODE_BINARY:
    {
        BinaryNode *bin_node = dynamic_cast<BinaryNode *>(node);
        Type left = infer_type(bin_node->left.get());
        Type right = infer_type(bin_node->right.get());

        // Handle pointer artithmetic
        if (bin_node->op == BinOpType::ADD || bin_node->op == BinOpType::SUB)
        {
            if (left.is_pointer() && right.is_integral())
            {
                // here for now
                auto scale_node = std::make_unique<BinaryNode>(
                    BinOpType::MUL,
                    std::move(bin_node->right),
                    std::make_unique<IntegerLiteral>(Type(left.get_base_type()).get_size()));
                scale_node->type = left;
                bin_node->right = std::move(scale_node);
                bin_node->type = left;
                return left;
            }
            else if (right.is_pointer() && left.is_integral())
            {
                auto scale_node = std::make_unique<BinaryNode>(
                    BinOpType::MUL,
                    std::move(bin_node->left),
                    std::make_unique<IntegerLiteral>(Type(right.get_base_type()).get_size()));
                scale_node->type = left;
                bin_node->left = std::move(scale_node);
                bin_node->type = right;
                return right;
            }
        }

        if (left == right)
        {
            bin_node->type = left;
            return left;
        }

        if (left.can_convert_to(right))
        {
            auto cast_node = std::make_unique<CastNode>(std::move(bin_node->left), right);
            bin_node->left = std::move(cast_node);
            bin_node->type = right;
            return right;
        }
        else if (right.can_convert_to(left))
        {
            auto cast_node = std::make_unique<CastNode>(std::move(bin_node->right), left);
            bin_node->right = std::move(cast_node);
            bin_node->type = left;
            return left;
        }

        error("Cannot perform operation between " +
              left.to_string() + " and " + right.to_string());
    }
    case NodeType::NODE_UNARY:
    {
        UnaryNode *un_node = dynamic_cast<UnaryNode *>(node);

        Type type = infer_type(un_node->value.get());

        if (type.has_base_type(BaseType::DOUBLE) && un_node->op == UnaryOpType::COMPLEMENT)
            error("Cannot take bitwise complement of a double");

        if (un_node->op == UnaryOpType::ADDR_OF)
            type = Type(type.get_base_type(), type.get_ptr_depth() + 1);
        else if (un_node->op == UnaryOpType::DEREF)
            type = Type(type.get_base_type(), type.get_ptr_depth() - 1);

        return type;
    }
    case NodeType::NODE_CAST:
    {
        return ((CastNode *)node)->target_type;
    }
    case NodeType::NODE_POSTFIX:
    {
        // PostfixNode *postfix_node = (PostfixNode *)node;
        // Type type = infer_type(postfix_node->value.get());
        // postfix_node->type = type;
        // return type;

        // ((PostfixNode *)node)->type.print();

        return ((PostfixNode *)node)->type;
    }
    case NodeType::NODE_AGGREGATE_INIT:
    {
        return ((AggregateLiteral *)node)->type;
    }
    case NodeType::NODE_ARRAY_ACCESS:
    {
        ArrayAccessNode *array_access_node = (ArrayAccessNode *)node;

        /*
            Note that an array_access node can be one of two things (both must be accomodated for):
            - Just a regular variable i.e. arr[1]
            - A struct member i.e. struct.arr[1] / struct->arr[1]
        */
        if (field_name.has_value())
        {
            // First get the struct symbol using the struct name
            Symbol *struct_symbol = gst->get_symbol(field_name.value());

            std::string struct_name = struct_symbol->type.get_struct_name();

            /*
                Now check whether the 'field' for the struct matches properly
            */
            if (struct_table.find(struct_name) == struct_table.end())
                error("Struct '" + struct_name + "' not defined");

            std::unordered_map<std::string, Type> struct_fields = struct_table[struct_name];

            if (struct_fields.find(array_access_node->array->name) == struct_fields.end())
                error("Struct '" + struct_name + "' has no field '" + array_access_node->array->name + "'");

            Type type = struct_fields[array_access_node->array->name];

            array_access_node->type = type;
            array_access_node->array->type = type;

            if (auto index_literal = dynamic_cast<IntegerLiteral *>(array_access_node->index.get()))
            {
                if (index_literal->value < 0 || index_literal->value >= type.get_array_length())
                {
                    error("Array index " +
                          std::to_string(index_literal->value) +
                          " out of bounds for '" + array_access_node->array->name + "' within struct '" + field_name.value() +
                          "' of length " + std::to_string(type.get_array_length()));
                }
            }

            return type.get_base_type();
        }
        else
        {
            Symbol *array_symbol = gst->get_symbol(array_access_node->array->name);

            // Check if the index is a constant + in range
            if (auto index_literal = dynamic_cast<IntegerLiteral *>(array_access_node->index.get()))
            {
                if (index_literal->value < 0 || index_literal->value >= array_symbol->type.get_array_length())
                {
                    error("Array index " +
                          std::to_string(index_literal->value) +
                          " out of bounds for array '" + array_access_node->array->name +
                          "' of length " + std::to_string(array_symbol->type.get_array_length()));
                }
            }

            array_access_node->type = array_symbol->type;
            array_access_node->array->type = array_symbol->type;

            return array_symbol->type.get_base_type();
        }

        return ((ArrayAccessNode *)node)->type;
    }
    case NodeType::NODE_SIZE_OF:
    {
        /*
            SIZE_OF nodes have a default size of 4 (ints)
            As presumably the size of things will not be too large

            They can either be used for the following:
            - Get size of a type ie int
            - Get size of any variable ie 'a'
            - This check could be moved as it's not strictly related to inferring types
        */

        SizeOfNode *size_of_node = (SizeOfNode *)node;

        if (size_of_node->type.is_struct())
        {
            if (struct_table[size_of_node->type.get_struct_name()].empty())
                error("Struct '" + size_of_node->type.get_struct_name() + "' not defined");

            for (auto &member : struct_table[size_of_node->type.get_struct_name()])
                size_of_node->type.add_field(member.first, member.second);
        }

        if (size_of_node->var)
        {
            auto var_node = (VarNode *)size_of_node->var.get();

            analyse_var(var_node);

            auto rtn = gst->get_symbol(var_node->name);

            if (rtn == nullptr)
                error("Variable '" + ((VarNode *)node)->name + "' not defined");
        }

        return Type(BaseType::INT);
    }
    default:
        error("Cannot infer type of node of type ");
    }
}

bool SemanticAnalyser::try_promote_literal(std::unique_ptr<ASTNode> &expr, const Type &target)
{
    switch (target.get_base_type())
    {
    case BaseType::DOUBLE:
    {
        if (auto *i = dynamic_cast<IntegerLiteral *>(expr.get()))
        {
            if (std::llabs((long long)i->value) <= (1LL << 53))
            {
                expr = std::make_unique<DoubleLiteral>(static_cast<double>(i->value));
                return true;
            }
            return false;
        }
        if (auto *l = dynamic_cast<LongLiteral *>(expr.get()))
        {
            if (std::llabs(l->value) <= (1LL << 53))
            {
                expr = std::make_unique<DoubleLiteral>(static_cast<double>(l->value));
                return true;
            }
            return false;
        }
        if (auto *ui = dynamic_cast<UIntegerLiteral *>(expr.get()))
        {
            if ((unsigned long long)ui->value <= (1ULL << 53))
            {
                expr = std::make_unique<DoubleLiteral>(static_cast<double>(ui->value));
                return true;
            }
            return false;
        }
        if (auto *ul = dynamic_cast<ULongLiteral *>(expr.get()))
        {
            if ((unsigned long long)ul->value <= (1ULL << 53))
            {
                expr = std::make_unique<DoubleLiteral>(static_cast<double>(ul->value));
                return true;
            }
            return false;
        }
        if (dynamic_cast<DoubleLiteral *>(expr.get()))
            return true;
        break;
    }

    case BaseType::INT:
    {
        if (auto *i = dynamic_cast<IntegerLiteral *>(expr.get()))
            return true;
        if (auto *l = dynamic_cast<LongLiteral *>(expr.get()))
        {
            if (l->value >= INT_MIN && l->value <= INT_MAX)
            {
                expr = std::make_unique<IntegerLiteral>(static_cast<int>(l->value));
                return true;
            }
            return false;
        }
        if (auto *ui = dynamic_cast<UIntegerLiteral *>(expr.get()))
        {
            if (ui->value <= static_cast<unsigned int>(INT_MAX))
            {
                expr = std::make_unique<IntegerLiteral>(static_cast<int>(ui->value));
                return true;
            }
            return false;
        }
        break;
    }

    case BaseType::LONG:
    {
        if (auto *i = dynamic_cast<IntegerLiteral *>(expr.get()))
        {
            expr = std::make_unique<LongLiteral>(static_cast<long>(i->value));
            return true;
        }
        if (dynamic_cast<LongLiteral *>(expr.get()))
            return true;
        break;
    }

    case BaseType::UINT:
    {
        if (auto *i = dynamic_cast<IntegerLiteral *>(expr.get()))
        {
            if (i->value >= 0)
            {
                expr = std::make_unique<UIntegerLiteral>(static_cast<unsigned int>(i->value));
                return true;
            }
            return false;
        }
        if (dynamic_cast<UIntegerLiteral *>(expr.get()))
            return true;
        break;
    }

    case BaseType::ULONG:
    {
        if (auto *i = dynamic_cast<IntegerLiteral *>(expr.get()))
        {
            if (i->value >= 0)
            {
                expr = std::make_unique<ULongLiteral>(static_cast<unsigned long>(i->value));
                return true;
            }
            return false;
        }
        if (auto *l = dynamic_cast<LongLiteral *>(expr.get()))
        {
            if (l->value >= 0)
            {
                expr = std::make_unique<ULongLiteral>(static_cast<unsigned long>(l->value));
                return true;
            }
            return false;
        }
        if (dynamic_cast<ULongLiteral *>(expr.get()))
            return true;
        break;
    }

    case BaseType::CHAR:
    {
        if (auto *i = dynamic_cast<IntegerLiteral *>(expr.get()))
        {
            if (i->value >= std::numeric_limits<char>::min() &&
                i->value <= std::numeric_limits<char>::max())
            {
                expr = std::make_unique<CharLiteral>(
                    static_cast<char>(i->value), Type(BaseType::CHAR));
                return true;
            }
            return false;
        }
        break;
    }

    case BaseType::BOOL:
    {
        if (auto *i = dynamic_cast<IntegerLiteral *>(expr.get()))
        {
            if (i->value == 0 || i->value == 1)
            {
                expr = std::make_unique<BoolLiteral>(i->value != 0);
                return true;
            }
            return false;
        }
        break;
    }

    default:
        break;
    }

    return false;
}
