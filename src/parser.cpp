#include <vector>
#include <iostream>
#include <optional>

#include "../include/parser.h"
#include "../include/lexer.h"
#include "../include/ast.h"

std::unordered_map<BinOpType, int> precedence_map = {
    {BinOpType::OR, 5},
    {BinOpType::AND, 10},
    {BinOpType::EQUAL, 20},
    {BinOpType::NOT_EQUAL, 20},
    {BinOpType::LESS_THAN, 25},
    {BinOpType::GREATER_THAN, 25},
    {BinOpType::LESS_OR_EQUAL, 25},
    {BinOpType::GREATER_OR_EQUAL, 25},
    {BinOpType::ADD, 35},
    {BinOpType::SUB, 35},
    {BinOpType::MUL, 40},
    {BinOpType::DIV, 40},
    {BinOpType::MOD, 40},
};

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

void Parser::expect_and_advance(std::vector<TokenType> &tokens)
{
    expect(tokens);
    advance();
}

std::shared_ptr<ProgramNode> Parser::parse()
{
    std::shared_ptr<ProgramNode> program = std::make_shared<ProgramNode>();

    while (current_token.type != TOKEN_EOF)
    {
        if (match(TOKEN_FN))
            program->decls.emplace_back(parse_func_decl());
        else if (match(addressable_types))
            program->decls.emplace_back(parse_var_decl());
        else if (match(TOKEN_STATIC) || match(TOKEN_EXTERN))
        {
            TokenType qualifier = current_token.type;
            advance();

            if (match(TOKEN_FN))
                program->decls.emplace_back(parse_func_decl(qualifier));
            else if (match(addressable_types))
                program->decls.emplace_back(parse_var_decl(qualifier));
        }

        advance();
    }

    return program;
}

std::unique_ptr<FuncNode> Parser::parse_func_decl(TokenType specifier)
{
    expect_and_advance(TOKEN_FN);

    expect(TOKEN_IDENTIFIER);

    std::string func_name = current_token.text;

    std::unique_ptr<FuncNode> func = std::make_unique<FuncNode>(func_name, get_specifier(specifier));

    advance();

    expect_and_advance(TOKEN_LPAREN);

    parse_param_list(func);

    expect_and_advance(TOKEN_RPAREN);

    expect_and_advance(TOKEN_ARROW);

    std::vector<TokenType> types;

    while (match(addressable_types))
    {
        types.emplace_back(current_token.type);

        advance();
    }

    func->return_type = determine_type(types);

    std::vector<std::unique_ptr<ASTNode>> elements = parse_block();

    for (auto &element : elements)
        func->elements.push_back(std::move(element));

    return func;
}

void Parser::parse_param_list(std::unique_ptr<FuncNode> &func)
{
    if (match(TOKEN_RPAREN))
        return;

    expect_and_advance(addressable_types);

    expect(TOKEN_IDENTIFIER);

    std::unique_ptr<VarNode> var = std::make_unique<VarNode>(current_token.text, Type::INT);
    func->params.emplace_back(std::make_unique<VarDeclNode>(std::move(var), nullptr));

    advance();

    if (match(TOKEN_RPAREN))
        return;

    while (current_token.type != TOKEN_RPAREN)
    {
        expect_and_advance(TOKEN_COMMA);

        expect_and_advance(addressable_types);

        expect(TOKEN_IDENTIFIER);

        std::unique_ptr<VarNode> var = std::make_unique<VarNode>(current_token.text, Type::INT);
        func->params.emplace_back(std::make_unique<VarDeclNode>(std::move(var), nullptr));

        advance();
    }

    return;
}

std::vector<std::unique_ptr<ASTNode>> Parser::parse_block()
{
    expect_and_advance(TOKEN_LBRACE);

    std::vector<std::unique_ptr<ASTNode>> elements = {};

    while (current_token.type != TOKEN_RBRACE)
    {
        if (match(TOKEN_RTN))
            elements.emplace_back(parse_rtn());
        else if (match(addressable_types))
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
        else if (match(TOKEN_STATIC) || match(TOKEN_EXTERN))
        {
            TokenType specifier = current_token.type;
            advance();
            elements.emplace_back(parse_var_decl(specifier));
        }
        else
            error("Expected an element");

        advance();
    }

    expect(TOKEN_RBRACE);

    return elements;
}

std::unique_ptr<RtnNode> Parser::parse_rtn()
{
    advance();

    std::unique_ptr<ASTNode> expr = parse_expr();

    expect(TOKEN_SEMICOLON);

    return std::make_unique<RtnNode>(std::move(expr));
}

std::unique_ptr<IfNode> Parser::parse_if_stmt()
{
    advance();
    expect_and_advance(TOKEN_LPAREN);

    std::unique_ptr<ASTNode> expr = parse_expr();
    std::unique_ptr<BinaryNode> binary_expr = std::unique_ptr<BinaryNode>(dynamic_cast<BinaryNode *>(expr.release()));

    expect_and_advance(TOKEN_RPAREN);

    std::vector<std::unique_ptr<ASTNode>> then_elements = parse_block();
    std::vector<std::unique_ptr<ASTNode>> else_elements;

    advance();

    if (match(TOKEN_ELSE))
    {
        advance();
        else_elements = parse_block();
    }
    else
        retreat();

    return std::make_unique<IfNode>(std::move(binary_expr), std::move(then_elements), std::move(else_elements));
}

std::unique_ptr<WhileNode> Parser::parse_while_stmt()
{
    advance();

    expect_and_advance(TOKEN_LPAREN);

    std::unique_ptr<ASTNode> expr = parse_expr();
    std::unique_ptr<BinaryNode> bin_expr = std::unique_ptr<BinaryNode>(dynamic_cast<BinaryNode *>(expr.release()));

    expect_and_advance(TOKEN_RPAREN);

    std::vector<std::unique_ptr<ASTNode>> elements = parse_block();

    return std::make_unique<WhileNode>(std::move(bin_expr), std::move(elements));
}

std::unique_ptr<ForNode> Parser::parse_for_stmt()
{
    advance();

    expect_and_advance(TOKEN_LPAREN);

    std::unique_ptr<ASTNode> init = parse_for_init();

    advance();

    std::unique_ptr<ASTNode> expr = parse_expr();
    std::unique_ptr<BinaryNode> condition = std::unique_ptr<BinaryNode>(dynamic_cast<BinaryNode *>(expr.release()));

    expect_and_advance(TOKEN_SEMICOLON);

    std::unique_ptr<ASTNode> post = parse_expr();

    expect_and_advance(TOKEN_RPAREN);

    std::vector<std::unique_ptr<ASTNode>> elements = parse_block();

    return std::make_unique<ForNode>(std::move(init), std::move(condition), std::move(post), std::move(elements));
}

std::unique_ptr<ASTNode> Parser::parse_for_init()
{
    if (match(addressable_types))
        return parse_var_decl();
    else if (match(TOKEN_IDENTIFIER))
        return parse_var_assign();
    else
        error("Parser Error: Expected valid for init");
}

std::unique_ptr<ASTNode> Parser::parse_loop_control()
{
    TokenType token_type = current_token.type;

    advance();

    expect(TOKEN_SEMICOLON);

    if (token_type == TOKEN_CONTINUE)
        return std::make_unique<ContinueNode>("");
    else if (token_type == TOKEN_BREAK)
        return std::make_unique<BreakNode>("");
}

std::unique_ptr<VarDeclNode> Parser::parse_var_decl(TokenType specifier)
{
    std::vector<TokenType> types;

    while (match(addressable_types))
    {
        types.emplace_back(current_token.type);

        advance();
    }

    curr_decl_type = determine_type(types);

    expect(TOKEN_IDENTIFIER);

    std::unique_ptr<VarNode> var = std::make_unique<VarNode>(current_token.text, curr_decl_type, get_specifier(specifier));

    advance();

    std::vector<TokenType> excepted_tokens = std::vector<TokenType>{TOKEN_ASSIGN, TOKEN_SEMICOLON};
    expect(excepted_tokens);

    if (match(TOKEN_ASSIGN))
    {
        advance();

        std::unique_ptr<ASTNode> expr = parse_expr();

        expect(TOKEN_SEMICOLON);

        curr_decl_type = Type::INT;

        return std::make_unique<VarDeclNode>(std::move(var), std::move(expr));
    }

    curr_decl_type = Type::INT;

    return std::make_unique<VarDeclNode>(std::move(var), nullptr);
}

Type Parser::determine_type(std::vector<TokenType> &types)
{
    if (types.size() == 1 && types[0] == TOKEN_INT)
        return Type::INT;
    else if (types.size() == 1 && types[0] == TOKEN_LONG)
        return Type::LONG;
    else if (types.size() == 2 && types[0] == TOKEN_SIGNED && types[1] == TOKEN_INT)
        return Type::INT;
    else if (types.size() == 2 && types[0] == TOKEN_UNSIGNED && types[1] == TOKEN_INT)
        return Type::UINT;
    else if (types.size() == 2 && types[0] == TOKEN_SIGNED && types[1] == TOKEN_LONG)
        return Type::LONG;
    else if (types.size() == 2 && types[0] == TOKEN_UNSIGNED && types[1] == TOKEN_LONG)
        return Type::ULONG;
    else if (types.size() == 1 && types[0] == TOKEN_DOUBLE)
        return Type::DOUBLE;
}

std::unique_ptr<VarAssignNode> Parser::parse_var_assign()
{
    std::unique_ptr<ASTNode> factor = parse_factor();
    std::unique_ptr<VarNode> var = std::unique_ptr<VarNode>(dynamic_cast<VarNode *>(factor.release()));

    advance();

    expect_and_advance(TOKEN_ASSIGN);

    std::unique_ptr<ASTNode> expr = parse_expr();

    return std::make_unique<VarAssignNode>(std::move(var), std::move(expr));
}

std::unique_ptr<ASTNode> Parser::parse_expr(int min_presedence)
{
    std::unique_ptr<ASTNode> left = parse_factor();

    advance();

    while (match(bin_op_tokens) && get_precedence(current_token.type) >= min_presedence)
    {
        TokenType op = current_token.type;

        advance();

        std::unique_ptr<ASTNode> right = parse_expr(get_precedence(op) + 1);

        left = std::make_unique<BinaryNode>(get_bin_op_type(op), std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<ASTNode> Parser::parse_factor()
{
    if (match(TOKEN_NUMBER))
    {
        try
        {
            if (curr_decl_type == Type::LONG)
            {
                long number = std::stol(current_token.text);
                return std::make_unique<LongLiteral>(number);
            }
            else if (curr_decl_type == Type::INT)
            {
                int number = std::stoi(current_token.text);
                return std::make_unique<IntegerLiteral>(number);
            }
            else if (curr_decl_type == Type::UINT)
            {
                unsigned int number = std::stoul(current_token.text);
                return std::make_unique<UIntegerLiteral>(number);
            }
            else if (curr_decl_type == Type::ULONG)
            {
                unsigned long number = std::stoul(current_token.text);
                return std::make_unique<ULongLiteral>(number);
            }
        }
        catch (const std::out_of_range &)
        {
            // If the number is out of int range, treat it as a long
            long number = std::stol(current_token.text);
            return std::make_unique<LongLiteral>(number);
        }

        error("Unknown number type");
    }
    else if (match(TOKEN_FPN))
    {
        try
        {
            double number = std::stod(current_token.text);
            return std::make_unique<DoubleLiteral>(number);
        }
        catch (const std::exception &e)
        {
            error("Number out of range");
        }
    }
    else if (match(TOKEN_TILDA) || match(TOKEN_MINUS) || match(TOKEN_INCREMENT) || match(TOKEN_DECREMENT))
    {
        auto op = current_token.type;

        advance();

        std::unique_ptr<ASTNode> expr = parse_factor();

        return std::make_unique<UnaryNode>(get_unary_op_type(op), std::move(expr));
    }
    else if (match(TOKEN_LPAREN))
    {
        advance();

        if (match(addressable_types))
        {
            std::vector<TokenType> types;

            while (match(addressable_types))
            {
                types.emplace_back(current_token.type);
                advance();
            }

            Type type = determine_type(types);

            expect_and_advance(TOKEN_RPAREN);

            std::unique_ptr<ASTNode> factor = parse_factor();

            return std::make_unique<CastNode>(std::move(factor), type);
        }

        auto expr = parse_expr();

        return expr;
    }
    else if (match(TOKEN_IDENTIFIER))
    {
        std::string var_identifier = current_token.text;

        advance();

        if (match(TOKEN_LPAREN))
        {
            std::unique_ptr<FuncCallNode> func_call = std::make_unique<FuncCallNode>(var_identifier);
            parse_args_list(func_call);
            expect(TOKEN_RPAREN);
            return func_call;
        }
        else
        {
            retreat();
            return std::make_unique<VarNode>(var_identifier, Type::INT);
        }
    }
    else
        error("Expected expression");
}

void Parser::parse_args_list(std::unique_ptr<FuncCallNode> &func_call)
{
    advance();

    std::unique_ptr<ASTNode> expr = parse_expr();

    func_call->args.push_back(std::move(expr));

    while (match(TOKEN_COMMA))
    {
        advance();
        expr = parse_expr();
        func_call->args.push_back(std::move(expr));
    }
}

int Parser::get_precedence(TokenType op)
{
    return precedence_map.at(get_bin_op_type(op));
}