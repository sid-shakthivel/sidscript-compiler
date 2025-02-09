#include "../include/semanticAnalyser.h"

#include <iostream>

SemanticAnalyser::SemanticAnalyser(std::shared_ptr<GlobalSymbolTable> gst) : gst(gst) {}

void SemanticAnalyser::analyse(std::shared_ptr<ProgramNode> program)
{
    for (auto &decl : program->decls)
    {
        if (decl->type == NodeType::NODE_FUNCTION)
            analyse_func(dynamic_cast<FuncNode *>(decl.get()));
        else if (decl->type == NodeType::NODE_VAR_DECL)
            analyse_var_decl(dynamic_cast<VarDeclNode *>(decl.get()));
    }
}

void SemanticAnalyser::analyse_func(FuncNode *func)
{
    // Check for duplicate function names
    if (gst->functions.find(func->name) != gst->functions.end())
        throw std::runtime_error("Semantic Error: Duplicate function name: " + func->name);

    // Find and create new function symbol using params
    std::vector<Type> arg_types;
    for (auto &param : func->params)
        arg_types.push_back(infer_type(dynamic_cast<VarDeclNode *>(param.get())->var.get()));

    std::unique_ptr<FuncSymbol> func_symbol = std::make_unique<FuncSymbol>(func->name, func->params.size(), arg_types, func->return_type);
    std::shared_ptr<SymbolTable> symbol_table = std::make_unique<SymbolTable>();
    gst->functions[func->name] = std::make_tuple(std::move(func_symbol), symbol_table);

    current_func_name = func->name;

    symbol_table->enter_scope();

    for (auto &param : func->params)
        symbol_table->declare_var(dynamic_cast<VarDeclNode *>(param.get())->var->name);

    for (auto &element : func->elements)
        analyse_node(element.get());

    symbol_table->exit_scope();

    current_func_name = "";
}

void SemanticAnalyser::analyse_node(ASTNode *node)
{
    if (node->type == NodeType::NODE_VAR_DECL)
        analyse_var_decl((VarDeclNode *)node);
    else if (node->type == NodeType::NODE_VAR_ASSIGN)
        analyser_var_assign((VarAssignNode *)node);
    else if (node->type == NodeType::NODE_IF)
        analyse_if_stmt((IfNode *)node);
    else if (node->type == NodeType::NODE_RETURN)
        analyse_rtn((RtnNode *)node);
    else if (node->type == NodeType::NODE_BINARY)
        analyse_binary((BinaryNode *)node);
    else if (node->type == NodeType::NODE_UNARY)
        analyse_unary((UnaryNode *)node);
    else if (node->type == NodeType::NODE_VAR)
        analyse_var((VarNode *)node);
    else if (node->type == NodeType::NODE_WHILE)
        analyse_while_stmt((WhileNode *)node);
    else if (node->type == NodeType::NODE_FOR)
        analyse_for_stmt((ForNode *)node);
    else if (node->type == NodeType::NODE_BREAK || node->type == NodeType::NODE_CONTINUE)
        analyse_loop_control(node);
    else if (node->type == NodeType::NODE_FUNC_CALL)
        analyse_func_call((FuncCallNode *)node);
    else if (node->type == NodeType::NODE_CAST)
        analyse_cast((CastNode *)node);
}

void SemanticAnalyser::analyse_cast(CastNode *node)
{
    node->src_type = infer_type((ASTNode *)(node->expr.get()));
}

void SemanticAnalyser::analyse_var_decl(VarDeclNode *node)
{
    // Global variables are only allowed to have constant values ie 5 rather then expressions
    if (current_func_name == "")
        if (node->value != nullptr)
            if (node->value->type != NodeType::NODE_NUMBER)
                throw std::runtime_error("Semantic Error: Global variable " + node->var->name + " must have constant value");

    if (node->value != nullptr)
    {
        analyse_node(node->value.get());

        Type var_type = node->var->type;
        Type value_type = infer_type(node->value.get());

        Type common_type = get_common_type(var_type, value_type);

        if (common_type != var_type)
            throw std::runtime_error("Semantic Error: Conflicting types " + get_type_str(var_type) + " and " + get_type_str(value_type) + " in declaration of " + node->var->name);
    }

    gst->declare_var(current_func_name, node->var.get());
    gst->get_symbol(current_func_name, node->var->name)->set_type(node->var->type);
}

void SemanticAnalyser::analyser_var_assign(VarAssignNode *node)
{
    node->var->name = gst->check_var_defined(current_func_name, node->var->name);
    analyse_node(node->value.get());
    Type var_type = gst->get_symbol(current_func_name, node->var->name)->type;
    Type value_type = infer_type(node->value.get());

    Type common_type = get_common_type(var_type, value_type);

    if (common_type != var_type)
        throw std::runtime_error("Semantic Error: Conflicting types " + get_type_str(var_type) + " and " + get_type_str(value_type) + " in assignment of " + node->var->name);
}

void SemanticAnalyser::analyse_rtn(RtnNode *node)
{
    if (node->value)
    {
        analyse_node(node->value.get());

        FuncSymbol *func = gst->get_func_symbol(current_func_name);
        Type return_type = infer_type(node->value.get());

        Type common_type = get_common_type(func->return_type, return_type);

        if (common_type != func->return_type)
            throw std::runtime_error("Semantic Error: Conflicting types in return of " + current_func_name);
    }
}

void SemanticAnalyser::analyse_if_stmt(IfNode *node)
{
    analyse_node(node->condition.get());

    gst->enter_scope(current_func_name);
    for (auto &stmt : node->then_elements)
        analyse_node(stmt.get());
    gst->exit_scope(current_func_name);

    if (!node->else_elements.empty())
    {
        gst->enter_scope(current_func_name);
        for (auto &stmt : node->else_elements)
            analyse_node(stmt.get());
        gst->exit_scope(current_func_name);
    }
}

void SemanticAnalyser::analyse_while_stmt(WhileNode *node)
{
    analyse_node(node->condition.get());

    std::string label = gen_new_loop_label();
    node->label = label;

    enter_loop_scope(label);
    gst->enter_scope(current_func_name);

    for (auto &stmt : node->elements)
        analyse_node(stmt.get());

    gst->exit_scope(current_func_name);
    exit_loop_scope();
}

void SemanticAnalyser::analyse_for_stmt(ForNode *node)
{
    std::string label = gen_new_loop_label();
    node->label = label;

    enter_loop_scope(label);
    gst->enter_scope(current_func_name);

    analyse_node(node->init.get());
    analyse_node(node->condition.get());
    analyse_node(node->post.get());

    for (auto &stmt : node->elements)
        analyse_node(stmt.get());

    gst->exit_scope(current_func_name);
    exit_loop_scope();
}

void SemanticAnalyser::analyse_binary(BinaryNode *node)
{
    analyse_node(node->left.get());
    analyse_node(node->right.get());
}

void SemanticAnalyser::analyse_unary(UnaryNode *node)
{
    analyse_node(node->value.get());
}

void SemanticAnalyser::analyse_var(VarNode *node)
{
    node->name = gst->check_var_defined(current_func_name, node->name);
}

std::string SemanticAnalyser::gen_new_loop_label()
{
    return ".Lloop_" + std::to_string(loop_label_counter++);
}

void SemanticAnalyser::enter_loop_scope(std::string label)
{
    loop_scopes.emplace(label);
}

void SemanticAnalyser::exit_loop_scope()
{
    if (loop_scopes.empty())
        throw std::runtime_error("Semantic Error: No scope to exit");

    loop_scopes.pop();
}

void SemanticAnalyser::analyse_loop_control(ASTNode *node)
{
    if (node->type == NodeType::NODE_CONTINUE)
        ((ContinueNode *)node)->label = loop_scopes.top();
    else if (node->type == NodeType::NODE_BREAK)
        ((BreakNode *)node)->label = loop_scopes.top();
}

void SemanticAnalyser::analyse_func_call(FuncCallNode *node)
{
    FuncSymbol *func = gst->get_func_symbol(node->name);

    if (func->arg_count != node->args.size())
        throw std::runtime_error("Semantic Error: Function '" + node->name + "' has " + std::to_string(func->arg_count) + " arguments, but " + std::to_string(node->args.size()) + " were provided");

    for (int i = 0; i < node->args.size(); i++)
        analyse_node(node->args[i].get());

    for (int i = 0; i < node->args.size(); i++)
    {
        Type arg_type = infer_type(node->args[i].get());
        Type param_type = func->arg_types[i];

        Type common_type = get_common_type(arg_type, param_type);

        if (common_type != arg_type)
            throw std::runtime_error("Semantic Error: Conflicting types in function call of '" + node->name + "'");
    }
}

Type SemanticAnalyser::infer_type(ASTNode *node)
{
    switch (node->type)
    {
    case NodeType::NODE_NUMBER:
        return ((NumericLiteral *)node)->value_type;
    case NodeType::NODE_VAR:
        return gst->get_symbol(current_func_name, ((VarNode *)node)->name)->type;
    case NodeType::NODE_FUNC_CALL:
    {
        FuncSymbol *func = gst->get_func_symbol(((FuncCallNode *)node)->name);
        return func->return_type;
    }
    case NodeType::NODE_BINARY:
    {
        BinaryNode *bin_node = dynamic_cast<BinaryNode *>(node);
        Type left = infer_type(bin_node->left.get());
        Type right = infer_type(bin_node->right.get());

        Type common_type = get_common_type(left, right);

        if (left != common_type)
        {
            auto cast_node = std::make_unique<CastNode>(std::move(bin_node->left), common_type, left);
            bin_node->left = std::move(cast_node);
        }
        if (right != common_type)
        {
            auto cast_node = std::make_unique<CastNode>(std::move(bin_node->right), common_type, right);
            bin_node->right = std::move(cast_node);
        }

        bin_node->type = common_type;

        return common_type;
    }
    case NodeType::NODE_UNARY:
    {
        UnaryNode *un_node = dynamic_cast<UnaryNode *>(node);
        Type type = infer_type(un_node->value.get());
        un_node->type = type;
        return type;
    }
    case NodeType::NODE_CAST:
    {
        return ((CastNode *)node)->target_type;
    }
    default:
        throw std::runtime_error("Semantic Error: Cannot infer type of node of type ");
    }
}

int SemanticAnalyser::get_type_size(Type &type)
{
    if (type == Type::INT || type == Type::UINT)
        return 4;
    else if (type == Type::LONG || type == Type::ULONG)
        return 8;
}

bool SemanticAnalyser::is_signed(Type &type)
{
    return type == Type::INT || type == Type::LONG;
}

Type SemanticAnalyser::get_common_type(Type type1, Type type2)
{
    // If the types are the same, pick either one
    if (type1 == type2)
        return type1;

    // If they’re the same size, choose the signed type
    if (get_type_size(type1) == get_type_size(type2))
        return is_signed(type1) ? type1 : type2;

    // If they’re not the same size, choose the bigger one
    return (get_type_size(type1) > get_type_size(type2)) ? type1 : type2;
}