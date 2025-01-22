#include "../include/semanticAnalyser.h"

#include <iostream>

SemanticAnalyser::SemanticAnalyser(GlobalSymbolTable *gst) : gst(gst) {}

void SemanticAnalyser::analyse(ProgramNode *program)
{
    for (auto decl : program->decls)
    {
        if (decl->type == NodeType::NODE_FUNCTION)
            analyse_func((FuncNode *)decl);
        else if (decl->type == NodeType::NODE_VAR_DECL)
            analyse_var_decl((VarDeclNode *)decl);
    }
}

void SemanticAnalyser::analyse_func(FuncNode *func)
{
    // Check for duplicate function names
    if (gst->functions.find(func->name) != gst->functions.end())
        throw std::runtime_error("Semantic Error: Duplicate function name: " + func->name);

    // Find and create new function symbol using params
    std::vector<Type> arg_types;
    for (auto param : func->params)
        arg_types.push_back(infer_type(((VarDeclNode *)param)->var));

    FuncSymbol *func_symbol = new FuncSymbol(func->name, func->params.size(), arg_types, func->return_type);
    SymbolTable *symbol_table = new SymbolTable();

    gst->functions[func->name] = std::make_tuple(func_symbol, symbol_table);

    current_st = symbol_table;

    symbol_table->enter_scope();

    for (auto param : func->params)
        symbol_table->declare_variable(((VarDeclNode *)param)->var->name);

    for (auto element : func->elements)
        analyse_node(element);

    symbol_table->exit_scope();

    current_st = nullptr;
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
}

void SemanticAnalyser::analyse_var_decl(VarDeclNode *node)
{
    current_st->declare_variable(node->var->name);
    if (node->value)
        analyse_node(node->value);
}

void SemanticAnalyser::analyser_var_assign(VarAssignNode *node)
{
    current_st->resolve_variable(node->var->name);
    analyse_node(node->value);
}

void SemanticAnalyser::analyse_rtn(RtnNode *node)
{
    if (node->value)
        analyse_node(node->value);
}

void SemanticAnalyser::analyse_if_stmt(IfNode *node)
{
    analyse_node(node->condition);

    current_st->enter_scope();
    for (auto stmt : node->then_elements)
        analyse_node(stmt);
    current_st->exit_scope();

    if (!node->else_elements.empty())
    {
        current_st->enter_scope();
        for (auto stmt : node->else_elements)
            analyse_node(stmt);
        current_st->exit_scope();
    }
}

void SemanticAnalyser::analyse_while_stmt(WhileNode *node)
{
    analyse_node(node->condition);

    std::string label = gen_new_loop_label();
    node->label = label;

    std::cout << label << std::endl;

    enter_loop_scope(label);
    current_st->enter_scope();

    for (auto stmt : node->elements)
        analyse_node(stmt);
    current_st->exit_scope();

    exit_loop_scope();
}

void SemanticAnalyser::analyse_for_stmt(ForNode *node)
{
    std::string label = gen_new_loop_label();
    node->label = label;

    enter_loop_scope(label);
    current_st->enter_scope();

    analyse_node(node->init);
    analyse_node(node->condition);
    analyse_node(node->post);

    for (auto stmt : node->elements)
        analyse_node(stmt);

    current_st->exit_scope();
    exit_loop_scope();
}

void SemanticAnalyser::analyse_binary(BinaryNode *node)
{
    analyse_node(node->left);
    analyse_node(node->right);
}

void SemanticAnalyser::analyse_unary(UnaryNode *node)
{
    analyse_node(node->value);
}

void SemanticAnalyser::analyse_var(VarNode *node)
{
    current_st->resolve_variable(node->name);
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
    FuncSymbol *func = gst->resolve_func(node->name);

    if (func->arg_count != node->args.size())
        throw std::runtime_error("Semantic Error: Function '" + node->name + "' has " + std::to_string(func->arg_count) + " arguments, but " + std::to_string(node->args.size()) + " were provided");

    for (int i = 0; i < node->args.size(); i++)
        analyse_node(node->args[i]);

    for (int i = 0; i < node->args.size(); i++)
    {
        Type arg_type = infer_type(node->args[i]);
        Type param_type = func->arg_types[i];

        if (arg_type != param_type)
            throw std::runtime_error("Semantic Error: Cannot infer type of node of type " + std::to_string(node->type));
    }
}

Type SemanticAnalyser::infer_type(ASTNode *node)
{
    switch (node->type)
    {
    case NodeType::NODE_INTEGER:
        return ((IntegerLiteral *)node)->type;
    case NodeType::NODE_VAR:
        return ((VarNode *)node)->type;
    case NodeType::NODE_FUNC_CALL:
    {
        FuncSymbol *func = gst->resolve_func(((FuncCallNode *)node)->name);
        return func->return_type;
    }
    case NodeType::NODE_BINARY:
    {
        Type left = infer_type(((BinaryNode *)node)->left);
        Type right = infer_type(((BinaryNode *)node)->right);

        if (left != right)
            throw std::runtime_error("Semantic Error: Cannot infer type of node of type " + std::to_string(node->type));

        return left;
    }
    case NodeType::NODE_UNARY:
    {
        return infer_type(((UnaryNode *)node)->value);
    }
    default:
        throw std::runtime_error("Semantic Error: Cannot infer type of node of type " + std::to_string(node->type));
    }
}