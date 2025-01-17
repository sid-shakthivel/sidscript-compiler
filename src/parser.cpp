#include <vector>
#include <iostream>
#include <optional>

#include "../include/parser.h"
#include "../include/lexer.h"
#include "../include/ast.h"

std::unordered_map<BinOpType, int> precedence_map = {
    {OR, 5},
    {AND, 10},
    {EQUAL, 20},
    {NOT_EQUAL, 20},
    {LESS_THAN, 25},
    {GREATER_THAN, 25},
    {LESS_OR_EQUAL, 25},
    {GREATER_OR_EQUAL, 25},
    {ADD, 35},
    {SUB, 35},
    {MUL, 40},
    {DIV, 40},
    {MOD, 40},
};

std::vector<TokenType> bin_op_tokens = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_PERCENT, TOKEN_EQUALS, TOKEN_NOT_EQUALS, TOKEN_LT, TOKEN_GT, TOKEN_LE, TOKEN_GE, TOKEN_AND, TOKEN_OR};

Parser::Parser(Lexer *l) : lexer(l), current_token(TOKEN_EOF, "", 1)
{
    advance();
}

bool Parser::match(TokenType type)
{
    return current_token.type == type;
}

bool Parser::match(std::vector<TokenType> &tokens)
{
    for (auto token : tokens)
        if (current_token.type == token)
            return true;
}

void Parser::advance()
{
    current_token = lexer->get_next_token();
}

void Parser::retreat()
{
    lexer->rewind();
}

void Parser::error(const std::string &message)
{
    throw std::runtime_error("Parser Error: " + message + " but found " + current_token.text + " on line " + std::to_string(current_token.line));
}

void Parser::expect(TokenType token_type)
{
    if (!(current_token.type == token_type))
    {
        std::string expected_token = token_to_string(token_type);
        error("Expected " + expected_token);
    }
}

void Parser::expect(std::vector<TokenType> &tokens)
{
    for (auto token : tokens)
        if (current_token.type != token)
            return;

    // Fix this to include what it actually should be
    error("Expected " + token_to_string(tokens[0]));
}

void Parser::expect_and_advance(TokenType token_type)
{
    expect(token_type);
    advance();
}

ProgramNode *Parser::parse()
{
    ProgramNode *program = new ProgramNode();

    while (current_token.type != TOKEN_EOF)
    {
        FuncNode *func = parse_func_decl();
        program->functions.emplace_back(func);

        advance();
    }

    return program;
}

FuncNode *Parser::parse_func_decl()
{
    expect_and_advance(TOKEN_FN);

    expect(TOKEN_IDENTIFIER);

    std::string func_name = current_token.text;

    FuncNode *func = new FuncNode(func_name);

    advance();

    expect_and_advance(TOKEN_LPAREN);

    parse_param_list(func);

    expect_and_advance(TOKEN_RPAREN);

    expect_and_advance(TOKEN_ARROW);

    std::vector<TokenType> accepted_tokens = {TOKEN_INT_TEXT, TOKEN_VOID};
    expect(accepted_tokens);
    advance();

    func->return_type = get_type(current_token.type);

    std::vector<ASTNode *> elements = parse_block();

    func->elements = elements;

    return func;
}

void Parser::parse_param_list(FuncNode *func)
{
    if (match(TOKEN_RPAREN))
        return;

    expect_and_advance(TOKEN_INT_TEXT);

    expect(TOKEN_IDENTIFIER);

    VarNode *var = new VarNode(current_token.text, Type::INT);
    func->params.emplace_back(new VarDeclNode(var, nullptr));

    advance();

    if (match(TOKEN_RPAREN))
        return;

    while (current_token.type != TOKEN_RPAREN)
    {
        expect_and_advance(TOKEN_COMMA);

        expect_and_advance(TOKEN_INT_TEXT);

        expect(TOKEN_IDENTIFIER);

        var = new VarNode(current_token.text, Type::INT);
        func->params.emplace_back(new VarDeclNode(var, nullptr));

        advance();
    }

    return;
}

std::vector<ASTNode *> Parser::parse_block()
{
    expect_and_advance(TOKEN_LBRACE);

    std::vector<ASTNode *> elements;

    while (current_token.type != TOKEN_RBRACE)
    {
        if (match(TOKEN_RTN))
            elements.emplace_back(parse_rtn());
        else if (match(TOKEN_INT_TEXT))
            elements.emplace_back(parse_var_decl());
        else if (match(TOKEN_IDENTIFIER))
            elements.emplace_back(parse_var_assign());
        else if (match(TOKEN_IF))
            elements.emplace_back(parse_if_stmt());
        else if (match(TOKEN_WHILE))
            elements.emplace_back(parse_while_stmt());
        else if (match(TOKEN_FOR))
            elements.emplace_back(parse_for_stmt());
        else if (match(TOKEN_CONTINUE) || match(TOKEN_BREAK))
            elements.emplace_back(parse_loop_control());
        else
            error("Expected an element");

        advance();
    }

    expect(TOKEN_RBRACE);

    return elements;
}

RtnNode *Parser::parse_rtn()
{
    advance();

    ASTNode *expr = parse_expr();

    expect(TOKEN_SEMICOLON);

    return new RtnNode(expr);
}

IfNode *Parser::parse_if_stmt()
{
    advance();

    expect_and_advance(TOKEN_LPAREN);

    ASTNode *expr = parse_expr();

    expect_and_advance(TOKEN_RPAREN);

    std::vector<ASTNode *> then_elements = parse_block();
    std::vector<ASTNode *> else_elements;

    advance();

    if (match(TOKEN_ELSE))
    {
        advance();
        else_elements = parse_block();
    }
    else
        retreat();

    return new IfNode((BinaryNode *)expr, then_elements, else_elements);
}

WhileNode *Parser::parse_while_stmt()
{
    advance();

    expect_and_advance(TOKEN_LPAREN);

    ASTNode *expr = parse_expr();

    expect_and_advance(TOKEN_RPAREN);

    std::vector<ASTNode *> elements = parse_block();

    return new WhileNode((BinaryNode *)expr, elements);
}

ForNode *Parser::parse_for_stmt()
{
    advance();

    expect_and_advance(TOKEN_LPAREN);

    ASTNode *init = parse_for_init();

    advance();

    BinaryNode *condition = (BinaryNode *)parse_expr();

    expect_and_advance(TOKEN_SEMICOLON);

    ASTNode *post = parse_expr();

    expect_and_advance(TOKEN_RPAREN);

    std::vector<ASTNode *> elements = parse_block();

    return new ForNode(init, condition, post, elements);
}

ASTNode *Parser::parse_for_init()
{
    if (match(TOKEN_INT_TEXT))
        return parse_var_decl();
    else if (match(TOKEN_IDENTIFIER))
        return parse_var_assign();
    else
        error("Parser Error: Expected valid for init");
}

ASTNode *Parser::parse_loop_control()
{
    TokenType token_type = current_token.type;

    advance();

    expect(TOKEN_SEMICOLON);

    if (token_type == TOKEN_CONTINUE)
        return new ContinueNode("");
    else if (token_type == TOKEN_BREAK)
        return new BreakNode("");
}

VarDeclNode *Parser::parse_var_decl()
{
    advance();

    expect(TOKEN_IDENTIFIER);

    std::string var_identifier = current_token.text;
    VarNode *var = new VarNode(var_identifier, Type::INT);

    advance();

    std::vector<TokenType> excepted_tokens = std::vector<TokenType>{TOKEN_ASSIGN, TOKEN_SEMICOLON};
    expect(excepted_tokens);

    if (match(TOKEN_ASSIGN))
    {
        advance();

        ASTNode *expr = parse_expr();

        expect(TOKEN_SEMICOLON);

        return new VarDeclNode(var, expr);
    }

    return new VarDeclNode(var, nullptr);
}

VarAssignNode *Parser::parse_var_assign()
{
    VarNode *var = (VarNode *)parse_factor();

    advance();

    expect_and_advance(TOKEN_ASSIGN);

    ASTNode *expr = parse_expr();

    return new VarAssignNode(var, expr);
}

ASTNode *Parser::parse_expr(int min_presedence)
{
    ASTNode *left = parse_factor();

    advance();

    while (match(bin_op_tokens) && get_precedence(current_token.type) >= min_presedence)
    {
        TokenType op = current_token.type;

        advance();

        ASTNode *right = parse_expr(get_precedence(op) + 1);
        left = new BinaryNode(get_bin_op_type(op), left, right);
    }

    return left;
}

ASTNode *Parser::parse_factor()
{
    if (match(TOKEN_INT))
    {
        int number = std::stoi(current_token.text);

        return new IntegerLiteral(number);
    }
    else if (match(TOKEN_TILDA) || match(TOKEN_MINUS) || match(TOKEN_INCREMENT) || match(TOKEN_DECREMENT))
    {
        auto op = current_token.type;

        advance();

        ASTNode *expr = parse_factor();

        return new UnaryNode(get_unary_op_type(op), expr);
    }
    else if (match(TOKEN_LPAREN))
    {
        advance();

        ASTNode *expr = parse_expr();

        return expr;
    }
    else if (match(TOKEN_IDENTIFIER))
    {
        std::string var_identifier = current_token.text;

        advance();

        if (match(TOKEN_LPAREN))
        {
            FuncCallNode *func_call = new FuncCallNode(var_identifier);
            parse_args_list(func_call);
            expect(TOKEN_RPAREN);
            return func_call;
        }
        else
        {
            retreat();
            return new VarNode(var_identifier, Type::INT);
        }
    }
    else
        error("Expected expression");
}

void Parser::parse_args_list(FuncCallNode *func_call)
{
    advance();

    ASTNode *expr = parse_expr();

    func_call->args.push_back(expr);

    while (match(TOKEN_COMMA))
    {
        advance();
        expr = parse_expr();
        func_call->args.push_back(expr);
    }
}

int Parser::get_precedence(TokenType op)
{
    return precedence_map.at(get_bin_op_type(op));
}