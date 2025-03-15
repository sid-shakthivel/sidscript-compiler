#include "../include/semanticAnalyser.h"

#include <iostream>

SemanticAnalyser::SemanticAnalyser(std::shared_ptr<GlobalSymbolTable> gst) : gst(gst)
{
    // Initialise analysers
    handlers[NodeType::NODE_FUNCTION] = [this](ASTNode *node)
    { analyse_func(node); };
    handlers[NodeType::NODE_VAR_DECL] = [this](ASTNode *node)
    { analyse_var_decl(node); };
    handlers[NodeType::NODE_VAR_ASSIGN] = [this](ASTNode *node)
    { analyse_var_assign(node); };
    handlers[NodeType::NODE_RETURN] = [this](ASTNode *node)
    { analyse_rtn(node); };
    handlers[NodeType::NODE_IF] = [this](ASTNode *node)
    { analyse_if_stmt(node); };
    handlers[NodeType::NODE_WHILE] = [this](ASTNode *node)
    { analyse_while_stmt(node); };
    handlers[NodeType::NODE_FOR] = [this](ASTNode *node)
    { analyse_for_stmt(node); };
    handlers[NodeType::NODE_CONTINUE] = [this](ASTNode *node)
    { analyse_loop_control(node); };
    handlers[NodeType::NODE_BREAK] = [this](ASTNode *node)
    { analyse_loop_control(node); };
    handlers[NodeType::NODE_FUNC_CALL] = [this](ASTNode *node)
    { analyse_func_call(node); };
    handlers[NodeType::NODE_CAST] = [this](ASTNode *node)
    { analyse_cast(node); };
    handlers[NodeType::NODE_ADDR_OF] = [this](ASTNode *node)
    { analyse_addr_of(node); };
    handlers[NodeType::NODE_BINARY] = [this](ASTNode *node)
    { analyse_binary(node); };
    handlers[NodeType::NODE_UNARY] = [this](ASTNode *node)
    { analyse_unary(node); };
    handlers[NodeType::NODE_VAR] = [this](ASTNode *node)
    { analyse_var(node); };
    handlers[NodeType::NODE_ADDR_OF] = [this](ASTNode *node)
    { analyse_addr_of(node); };
    handlers[NodeType::NODE_DEREF] = [this](ASTNode *node)
    { analyse_deref(node); };
    handlers[NodeType::NODE_POSTFIX] = [this](ASTNode *node)
    {
        PostfixNode *postfix = dynamic_cast<PostfixNode *>(node);
        analyse_node(postfix->value.get());
    };
}

void SemanticAnalyser::analyse(std::shared_ptr<ProgramNode> program)
{
    for (auto &decl : program->decls)
    {
        if (decl->node_type == NodeType::NODE_FUNCTION)
            analyse_func(decl.get());
        else if (decl->node_type == NodeType::NODE_VAR_DECL)
            analyse_var_decl(decl.get());
    }
}

void SemanticAnalyser::analyse_func(ASTNode *node)
{
    FuncNode *func_node = (FuncNode *)node;

    // Find and create new function symbol using params
    std::vector<Type> arg_types;
    for (auto &param : func_node->params)
        arg_types.push_back(infer_type(dynamic_cast<VarDeclNode *>(param.get())->var.get()));

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

    gst->leave_func_scope();
}

void SemanticAnalyser::analyse_node(ASTNode *node)
{
    auto handler = handlers.find(node->node_type);
    if (handler != handlers.end())
        handler->second(node);
    else
        error("Unknown node type in semantic analysis");
}

void SemanticAnalyser::analyse_var_decl(ASTNode *node)
{
    VarDeclNode *var_decl_node = (VarDeclNode *)node;

    // Global variables are only allowed to have constant values ie 5 rather then expressions
    if (gst->is_global_scope())
        if (var_decl_node->value != nullptr)
            if (var_decl_node->value->node_type == NodeType::NODE_BINARY || var_decl_node->value->node_type == NodeType::NODE_UNARY)
                error("Global variable " + var_decl_node->var->name + " must have constant value");

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
        else if (var_type.is_array())
        {
            if (var_decl_node->value->node_type != NodeType::NODE_ARRAY_INIT)
                error("Array initialisation of " + var_decl_node->var->name + " requires array literal");

            ArrayLiteral *array_init = dynamic_cast<ArrayLiteral *>(var_decl_node->value.get());

            if (array_init->values.size() > var_type.get_size())
                error("Too many elements in array initialisation of " + var_decl_node->var->name);

            for (auto &element : array_init->values)
            {
                if (!infer_type(element.get()).has_base_type(var_type.get_base_type()))
                    error("Type in array initialisation of " + var_decl_node->var->name + " doesn't match");
            }
        }

        validate_type_assignment(var_type, value_type, var_decl_node->var->name);
    }

    gst->declare_var(var_decl_node->var.get());
    // gst->get_symbol(var_decl_node->var->name)->set_type(var_decl_node->var->type);
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

        validate_type_assignment(var_type, value_type, var->name);
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
}

void SemanticAnalyser::analyse_rtn(ASTNode *node)
{
    RtnNode *rtn_node = (RtnNode *)node;

    if (rtn_node->value == nullptr)
        error("Return statement in function '" + gst->get_current_func() + "' must have a value");

    analyse_node(rtn_node->value.get());

    FuncSymbol *func = gst->get_func_symbol(gst->get_current_func());
    Type return_type = infer_type(rtn_node->value.get());

    validate_type_assignment(func->return_type, return_type, func->name);
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
}

void SemanticAnalyser::analyse_unary(ASTNode *node)
{
    UnaryNode *unary_node = (UnaryNode *)node;
    analyse_node(unary_node->value.get());
}

void SemanticAnalyser::analyse_var(ASTNode *node)
{
    VarNode *var_node = (VarNode *)node;
    var_node->name = gst->check_var_defined(var_node->name);
}

std::string SemanticAnalyser::gen_new_loop_label()
{
    return ".Lloop_" + std::to_string(loop_label_counter++);
}

void SemanticAnalyser::analyse_loop_control(ASTNode *node)
{
    if (node->node_type == NodeType::NODE_CONTINUE)
        ((ContinueNode *)node)->label = loop_scopes.top();
    else if (node->node_type == NodeType::NODE_BREAK)
        ((BreakNode *)node)->label = loop_scopes.top();
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
        auto arg = fc_node->args[i].get();

        analyse_node(arg);

        Type arg_type = infer_type(arg);
        Type param_type = func->arg_types[i];

        validate_type_assignment(param_type, arg_type, "in call to '" + fc_node->name + "'");
    }
}

void SemanticAnalyser::analyse_addr_of(ASTNode *node)
{
    AddrOfNode *addr_of = (AddrOfNode *)node;
    analyse_node(addr_of->expr.get());

    if (addr_of->expr->node_type != NodeType::NODE_VAR)
        error("Can only take address of variables");

    Type expr_type = infer_type(addr_of->expr.get());
    addr_of->type = Type(expr_type.get_base_type(), expr_type.get_ptr_depth() + 1);
}

void SemanticAnalyser::analyse_deref(ASTNode *node)
{
    DerefNode *deref = (DerefNode *)node;
    analyse_node(deref->expr.get());
    Type expr_type = infer_type(deref->expr.get());

    if (!expr_type.is_pointer())
        error("Cannot dereference non-pointer type");

    deref->type = Type(expr_type.get_base_type(), expr_type.get_ptr_depth() - 1);
}

/*
    These loop scope methods have one distinct purpose:
    - When there are nested loops, it's difficult to determine which loop
      loop controls (break/continue) should use hence the loop_scopes is a stack of all loops
      so the most inner loop is used for loop controls
*/

void SemanticAnalyser::enter_loop_scope(std::string label)
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

void SemanticAnalyser::validate_type_assignment(const Type &target_type, const Type &source_type,
                                                const std::string &context)
{
    if (!target_type.can_assign_from(source_type))
        error("Cannot assign " + source_type.to_string() +
              " to " + target_type.to_string() + " in " + context);
}

void SemanticAnalyser::error(const std::string &message)
{
    throw std::runtime_error("Semantic Error: " + message);
}

Type SemanticAnalyser::infer_type(ASTNode *node)
{
    switch (node->node_type)
    {
    case NodeType::NODE_NUMBER:
        return ((NumericLiteral *)node)->value_type;
    case NodeType::NODE_VAR:
        return gst->get_symbol(((VarNode *)node)->name)->type;
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

        un_node->type = type;
        return type;
    }
    case NodeType::NODE_CAST:
    {
        return ((CastNode *)node)->target_type;
    }
    case NodeType::NODE_POSTFIX:
    {
        PostfixNode *post_node = dynamic_cast<PostfixNode *>(node);

        Type type = infer_type(post_node->value.get());

        post_node->type = type;
        return type;
    }
    case NodeType::NODE_DEREF:
    {
        return ((DerefNode *)node)->type;
    }
    case NodeType::NODE_ADDR_OF:
    {
        return ((AddrOfNode *)node)->type;
    }
    case NodeType::NODE_ARRAY_INIT:
    {
        return ((ArrayLiteral *)node)->arr_type;
    }
    case NodeType::NODE_ARRAY_ACCESS:
    {
        ArrayAccessNode *array_access_node = (ArrayAccessNode *)node;
        Symbol *array_symbol = gst->get_symbol(array_access_node->array->name);

        // Check if the index is a constant
        if (auto index_literal = dynamic_cast<IntegerLiteral *>(array_access_node->index.get()))
        {
            if (index_literal->value < 0 || index_literal->value >= array_symbol->type.get_size())
            {
                error("Array index " +
                      std::to_string(index_literal->value) +
                      " out of bounds for array '" + array_access_node->array->name +
                      "' of size " + std::to_string(array_symbol->type.get_size()));
            }
        }

        array_access_node->type = array_symbol->type;
        array_access_node->array->type = array_symbol->type;

        return array_symbol->type.get_base_type();
    }
    default:
        error("Cannot infer type of node of type ");
    }
}
