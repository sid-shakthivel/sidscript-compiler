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

Parser::Parser(Lexer *l) : lexer(l), current_token(TOKEN_EOF, "", 1, 0)
{
    advance();
}

bool Parser::match(TokenType type)
{
    return current_token.type == type;
}

bool Parser::match(std::vector<TokenType> &tokens)
{
    return std::find(tokens.begin(), tokens.end(), current_token.type) != tokens.end();
}

void Parser::advance()
{
    current_token = lexer->get_next_token();
}

void Parser::retreat(int iterations)
{
    current_token = lexer->rewind(iterations);
}

void Parser::error(const std::string &message)
{
    throw std::runtime_error("Parser Error: " + message + " but found " + current_token.text + " on line " + std::to_string(current_token.line));
}

void Parser::expect(TokenType token_type)
{
    if (current_token.type != token_type)
        error("Expected " + token_to_string(token_type));
}

void Parser::expect(std::vector<TokenType> &tokens)
{
    if (!match(tokens))
        error("Expected (multiple) " + token_to_string(tokens[0]));
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
        else if (match(TOKEN_STRUCT))
            program->decls.emplace_back(parse_struct_decl());
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

std::unique_ptr<ASTNode> Parser::parse_struct_decl()
{
    expect_and_advance(TOKEN_STRUCT);

    std::string struct_name = current_token.text;
    expect_and_advance(TOKEN_IDENTIFIER);

    advance();

    std::vector<std::unique_ptr<ASTNode>> members;

    while (!match(TOKEN_RBRACE))
    {
        members.emplace_back(parse_var_decl());
        advance();
    }

    expect_and_advance(TOKEN_RBRACE);

    expect(TOKEN_SEMICOLON);

    return std::make_unique<StructDeclNode>(struct_name, std::move(members));
}

std::unique_ptr<FuncNode> Parser::parse_func_decl(TokenType specifier)
{
    expect_and_advance(TOKEN_FN);

    expect(TOKEN_IDENTIFIER);

    std::unique_ptr<FuncNode> func = std::make_unique<FuncNode>(current_token.text, get_specifier(specifier));

    advance();

    expect_and_advance(TOKEN_LPAREN);

    parse_param_list(func);

    expect_and_advance(TOKEN_RPAREN);

    expect_and_advance(TOKEN_ARROW);

    func->return_type = parse_type();

    std::vector<std::unique_ptr<ASTNode>> elements = parse_block();

    for (auto &element : elements)
        func->elements.push_back(std::move(element));

    return func;
}

void Parser::parse_param_list(std::unique_ptr<FuncNode> &func)
{
    if (match(TOKEN_RPAREN))
        return;

    while (!match(TOKEN_RPAREN))
    {
        Type param_type = parse_type();

        expect(TOKEN_IDENTIFIER);
        std::string param_name = current_token.text;
        advance();

        std::unique_ptr<VarNode> var = std::make_unique<VarNode>(param_name, param_type);
        func->params.emplace_back(std::make_unique<VarDeclNode>(std::move(var), nullptr));

        if (!match(TOKEN_RPAREN))
            expect_and_advance(TOKEN_COMMA);
    }
}

std::vector<std::unique_ptr<ASTNode>> Parser::parse_block()
{
    expect_and_advance(TOKEN_LBRACE);

    std::vector<std::unique_ptr<ASTNode>> elements = {};

    while (current_token.type != TOKEN_RBRACE)
    {
        if (match(TOKEN_RTN))
            elements.emplace_back(parse_rtn());
        else if (match(TOKEN_IDENTIFIER))
        {
            /*
            This could be one of three things
                - Assigning a variable ie a = 5, a += 5;
                - Assigning an array ie arr[3] = 5, arr[2]++
                - An expression ie func(), i++
                - Declaring a struct variable ie Foo foo;
            */

            // advance();

            // if (match(TOKEN_IDENTIFIER))
            // {
            //     std::cout << "here are identifier is " << current_token.text << std::endl;
            //     retreat();
            //     std::cout << "after retreat are identifier is " << current_token.text << std::endl;
            //     elements.emplace_back(parse_var_decl());
            //     advance();
            //     continue;
            // }

            // retreat();

            int retreat_num = 0;
            bool is_assignment = false;

            while (!match(TOKEN_SEMICOLON) && !match(TOKEN_EOF))
            {
                advance();
                retreat_num += 1;
                if (match(assign_tokens))
                {
                    is_assignment = true;
                    break;
                }
            }

            if (is_assignment)
            {
                retreat(retreat_num);
                elements.emplace_back(parse_var_assign());
            }
            else
            {
                retreat(retreat_num);
                elements.emplace_back(parse_expr());
                expect(TOKEN_SEMICOLON);
            }
        }
        else if (match(addressable_types))
            elements.emplace_back(parse_var_decl());
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

    std::vector<std::unique_ptr<ASTNode>>
        then_elements = parse_block();
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
    else
        error("Expected continue or break statement");
}

std::unique_ptr<VarDeclNode> Parser::parse_var_decl(TokenType specifier)
{
    Type var_type = parse_type();

    expect(TOKEN_IDENTIFIER);
    std::string var_name = current_token.text;
    advance();

    while (match(TOKEN_LSBRACE))
    {
        advance();
        std::unique_ptr<ASTNode> size_expr = parse_expr();
        if (auto num = dynamic_cast<IntegerLiteral *>(size_expr.get()))
            var_type.add_array_dimension(num->value);
        else
            error("Array size must be a constant integer");
        expect_and_advance(TOKEN_RSBRACE);
    }

    std::unique_ptr<VarNode> var = std::make_unique<VarNode>(var_name, var_type, get_specifier(specifier));

    std::vector<TokenType> excepted_tokens = std::vector<TokenType>{TOKEN_ASSIGN, TOKEN_SEMICOLON};
    expect(excepted_tokens);

    if (match(TOKEN_ASSIGN))
    {
        advance();
        std::unique_ptr<ASTNode> init_expr;

        if (match(TOKEN_LBRACE) && var_type.is_array())
            init_expr = parse_array_initialiser(var_type);
        else
            init_expr = parse_expr();

        expect(TOKEN_SEMICOLON);
        return std::make_unique<VarDeclNode>(std::move(var), std::move(init_expr));
    }

    return std::make_unique<VarDeclNode>(std::move(var), nullptr);
}

Type Parser::parse_type()
{
    std::vector<TokenType> types;
    int i = 0;

    while (match(addressable_types))
    {
        if (i > 0)
            if (current_token.type == TOKEN_IDENTIFIER)
            {
                retreat();
                break;
            }
        types.emplace_back(current_token.type);
        advance();
        i++;
    }

    return determine_type(types);
}

Type Parser::determine_type(std::vector<TokenType> &types)
{
    BaseType base_type = BaseType::INT;
    bool is_unsigned = false;
    bool is_struct = false;
    int ptr_level = 0;

    for (const auto &type : types)
    {
        if (type == TOKEN_UNSIGNED)
            is_unsigned = true;
        else if (type == TOKEN_STAR)
            ptr_level++;
        else if (type == TOKEN_STRUCT)
            is_struct = true;
    }

    for (const auto &type : types)
    {
        switch (type)
        {
        case TOKEN_INT:
            base_type = is_unsigned ? BaseType::UINT : BaseType::INT;
            break;
        case TOKEN_LONG:
            base_type = is_unsigned ? BaseType::ULONG : BaseType::LONG;
            break;
        case TOKEN_DOUBLE:
            if (is_unsigned)
                error("Double cannot be unsigned");
            base_type = BaseType::DOUBLE;
            break;
        case TOKEN_CHAR_TEXT:
            base_type = BaseType::CHAR;
            break;
        case TOKEN_IDENTIFIER:
            std::cout << "here ig\n";
            return Type(current_token.text, ptr_level);
        default:
            continue;
        }
    }

    return Type(base_type, ptr_level);
}

std::unique_ptr<VarAssignNode> Parser::parse_var_assign()
{
    std::unique_ptr<ASTNode> target = parse_var_or_array_access();
    TokenType assign_type = current_token.type;

    expect_and_advance(assign_tokens);

    std::unique_ptr<ASTNode> expr = parse_expr();

    if (assign_type == TOKEN_ASSIGN)
    {
        return std::make_unique<VarAssignNode>(std::move(target), std::move(expr));
    }
    else
    {
        auto bin_op_type = get_bin_op_type(assign_type);

        std::unique_ptr<ASTNode> target_clone;
        if (auto var_node = dynamic_cast<VarNode *>(target.get()))
            target_clone = std::make_unique<VarNode>(*var_node);
        else if (auto array_access = dynamic_cast<ArrayAccessNode *>(target.get()))
            target_clone = std::make_unique<ArrayAccessNode>(*array_access);

        auto right = std::make_unique<BinaryNode>(bin_op_type, std::move(target_clone), std::move(expr));
        return std::make_unique<VarAssignNode>(std::move(target), std::move(right));
    }
}

std::unique_ptr<ASTNode> Parser::parse_expr(int min_presedence)
{
    std::unique_ptr<ASTNode> left = parse_factor();

    advance();

    if (match(TOKEN_INCREMENT) || match(TOKEN_DECREMENT))
    {
        if (dynamic_cast<VarNode *>(left.get()) == nullptr)
            error("Postfix operator requires a variable");

        left = std::make_unique<PostfixNode>(current_token.type, std::move(left));

        advance();
    }

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
        return parse_number_literal();
    }
    else if (match(TOKEN_FPN))
    {
        try
        {
            return std::make_unique<DoubleLiteral>(std::stod(current_token.text));
        }
        catch (const std::exception &e)
        {
            error("Number out of range");
        }
    }
    else if (match(TOKEN_CHAR))
    {
        return std::make_unique<CharLiteral>(current_token.text[0], Type(BaseType::CHAR));
    }
    else if (match(TOKEN_STRING))
    {
        Type string_type(BaseType::CHAR);
        string_type.add_array_dimension(current_token.text.length());
        return std::make_unique<StringLiteral>(current_token.text, string_type);
    }
    else if (match(un_op_tokens))
    {
        return parse_unary_operation();
    }
    else if (match(TOKEN_LPAREN))
    {
        /*
            This could be one of two things:
            - Casting something ie (int)a, (double*)ptr;
            - Just an expression wrapped in brackets ie (1 + 2) * 3;
        */
        advance();

        if (match(addressable_types))
        {
            retreat();
            return parse_cast();
        }

        auto expr = parse_expr();

        return expr;
    }
    else if (match(TOKEN_IDENTIFIER))
    {
        std::string identifier = current_token.text;
        std::unique_ptr<ASTNode> potential_var = parse_var_or_array_access();

        if (match(TOKEN_LPAREN))
        {
            std::unique_ptr<FuncCallNode> func_call = std::make_unique<FuncCallNode>(identifier);
            parse_args_list(func_call);
            expect(TOKEN_RPAREN);
            return func_call;
        }

        retreat();
        return potential_var;
    }
    else
        error("Expected expression");
}

std::unique_ptr<ASTNode> Parser::parse_number_literal()
{
    std::string num_text = current_token.text;
    try
    {
        // Check for suffixes first
        bool is_unsigned = (num_text.find('u') != std::string::npos || num_text.find('U') != std::string::npos);
        bool is_long = (num_text.find('l') != std::string::npos || num_text.find('L') != std::string::npos);

        // Remove suffixes for conversion
        num_text.erase(std::remove_if(num_text.begin(), num_text.end(),
                                      [](char c)
                                      { return c == 'u' || c == 'U' || c == 'l' || c == 'L'; }),
                       num_text.end());

        if (is_unsigned && is_long)
            return std::make_unique<ULongLiteral>(std::stoull(num_text));
        else if (is_unsigned)
            return std::make_unique<UIntegerLiteral>(std::stoul(num_text));
        else if (is_long)
            return std::make_unique<LongLiteral>(std::stoll(num_text));

        // Attempt to fit in smallest type possible
        try
        {
            return std::make_unique<IntegerLiteral>(std::stoi(num_text));
        }
        catch (const std::out_of_range &)
        {
            return std::make_unique<LongLiteral>(std::stoll(num_text));
        }
    }
    catch (const std::exception &)
    {
        error("Number out of range or invalid format");
    }

    return nullptr; // Unreachable but silences compiler warnings
}

std::unique_ptr<ASTNode> Parser::parse_unary_operation()
{
    auto op = current_token.type;
    advance();

    auto expr = parse_factor();

    if (op == TOKEN_AMPERSAND)
        return std::make_unique<AddrOfNode>(std::move(expr));
    else if (op == TOKEN_STAR)
        return std::make_unique<DerefNode>(std::move(expr));
    else
        return std::make_unique<UnaryNode>(get_unary_op_type(op), std::move(expr));
}

std::unique_ptr<ArrayLiteral> Parser::parse_array_initialiser(const Type &array_type)
{
    expect_and_advance(TOKEN_LBRACE);

    auto init_node = std::make_unique<ArrayLiteral>(array_type);

    if (!match(TOKEN_RBRACE))
    {
        // Parse first element
        init_node->add_element(parse_expr());

        // Parse remaining elements
        while (match(TOKEN_COMMA))
        {
            advance();
            if (match(TOKEN_RBRACE))
                break; // Allow trailing comma
            init_node->add_element(parse_expr());
        }
    }

    expect_and_advance(TOKEN_RBRACE);
    return init_node;
}

void Parser::parse_args_list(std::unique_ptr<FuncCallNode> &func_call)
{
    advance();

    if (match(TOKEN_RPAREN))
        return;

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

std::unique_ptr<ASTNode> Parser::parse_var_or_array_access(Specifier specifier)
{
    expect(TOKEN_IDENTIFIER);
    std::string var_name = current_token.text;
    advance();

    if (!match(TOKEN_LSBRACE))
        return std::make_unique<VarNode>(current_token.text);

    auto var = std::make_unique<VarNode>(var_name);
    advance();
    auto index = parse_expr();
    expect_and_advance(TOKEN_RSBRACE);

    return std::make_unique<ArrayAccessNode>(std::move(var), std::move(index));
}

std::unique_ptr<ASTNode> Parser::parse_cast()
{
    expect_and_advance(TOKEN_LPAREN);

    Type type = parse_type();

    expect_and_advance(TOKEN_RPAREN);

    std::unique_ptr<ASTNode> factor = parse_factor();

    if (!factor)
        error("Expected expression after cast");

    return std::make_unique<CastNode>(std::move(factor), type);
}