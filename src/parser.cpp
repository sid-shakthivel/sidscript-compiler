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

Parser::Parser(Lexer &l, std::string source_file) : lexer(l), current_token(TOKEN_EOF, "", 1, 0), source_file(source_file)
{
    advance();
}

bool Parser::match(const TokenType &type)
{
    return current_token.type == type;
}

bool Parser::match(const std::vector<TokenType> &tokens)
{
    return std::find(tokens.begin(), tokens.end(), current_token.type) != tokens.end();
}

void Parser::advance()
{
    current_token = lexer.get_next_token();
}

void Parser::retreat(int iterations)
{
    current_token = lexer.rewind(iterations);
}

void Parser::error(const std::string &message)
{
    throw std::runtime_error("Parser Error: " + message + " but found '" + current_token.text + "' on line " + std::to_string(current_token.line) + " in module '" + source_file + "'");
}

void Parser::expect(const TokenType &token_type)
{
    if (current_token.type != token_type)
        error("Expected " + lexer.token_to_string(token_type));
}

void Parser::expect(const std::vector<TokenType> &tokens)
{
    if (!match(tokens))
        error("Expected (multiple) " + lexer.token_to_string(tokens[0]));
}

void Parser::expect_and_advance(const TokenType &token_type)
{
    expect(token_type);
    advance();
}

void Parser::expect_and_advance(const std::vector<TokenType> &tokens)
{
    expect(tokens);
    advance();
}

void Parser::advance_and_expect(const TokenType &token_type)
{
    advance();
    expect(token_type);
}

std::shared_ptr<ProgramNode> Parser::parse()
{
    std::shared_ptr<ProgramNode> program = std::make_shared<ProgramNode>();

    while (current_token.type != TOKEN_EOF)
    {
        if (match(TOKEN_FN))
            program->decls.emplace_back(parse_func_decl(std::nullopt));
        else if (match(TOKEN_STRUCT))
        {
            /*
                When the struct keyword is found, it could be one of two things:
                - A struct declaration i.e. struct Test { int a; int b; };
                - A global struct varaible declaration i.e. struct Test *t;
                Hence we skip forward to determine whether the third token is an identifier or a left brace to determine which of the two it is
            */
            advance();
            advance();

            bool is_struct_var_decl = false;

            if (match(TOKEN_IDENTIFIER))
                is_struct_var_decl = true;

            retreat(2);

            if (is_struct_var_decl)
                program->decls.emplace_back(parse_var_decl({}));
            else
                program->decls.emplace_back(parse_struct_decl());
        }
        else if (match(addressable_types))
            program->decls.emplace_back(parse_var_decl(std::nullopt));
        else if (match(specifier_tokens))
        {
            std::vector<TokenType> specifiers;

            while (match(specifier_tokens))
            {
                specifiers.emplace_back(current_token.type);
                advance();
            }

            if (match(TOKEN_FN))
                program->decls.emplace_back(parse_func_decl(specifiers));
            else if (match(addressable_types))
                program->decls.emplace_back(parse_var_decl(specifiers));
        }
        else if (match(TOKEN_IMPORT))
            program->decls.emplace_back(parse_import());

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
        members.emplace_back(parse_var_decl(std::nullopt));
        advance();
    }

    expect_and_advance(TOKEN_RBRACE);

    expect(TOKEN_SEMICOLON);

    return std::make_unique<StructDeclNode>(
        struct_name,
        std::move(members),
        SourceLocation{current_token.line, current_token.index});
}

std::unique_ptr<FuncNode> Parser::parse_func_decl(const std::optional<std::vector<TokenType>> specifiers)
{
    expect_and_advance(TOKEN_FN);

    expect(TOKEN_IDENTIFIER);

    std::vector<Specifier> specs;
    if (specifiers.has_value())
        specs = parse_tokens_to_specifiers(*specifiers);
    else
        specs = {};

    std::unique_ptr<FuncNode> func = std::make_unique<FuncNode>(current_token.text, specs);

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
        std::unique_ptr<VarNode> var = parse_var_declarator(std::nullopt);

        func->params.emplace_back(std::make_unique<VarDeclNode>(std::move(var), nullptr, SourceLocation{current_token.line, current_token.index}));

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
            */

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

            retreat(retreat_num);

            if (is_assignment)
                elements.emplace_back(parse_var_assign());
            else
            {
                elements.emplace_back(parse_expr());
                expect(TOKEN_SEMICOLON);
            }
        }
        else if (match(TOKEN_STAR))
            elements.emplace_back(parse_var_assign());
        else if (match(addressable_types))
            elements.emplace_back(parse_var_decl(std::nullopt));
        else if (match(TOKEN_IF))
            elements.emplace_back(parse_if_stmt());
        else if (match(TOKEN_WHILE))
            elements.emplace_back(parse_while_stmt());
        else if (match(TOKEN_FOR))
            elements.emplace_back(parse_for_stmt());
        else if (match(TOKEN_CONTINUE) || match(TOKEN_BREAK))
            elements.emplace_back(parse_loop_control());
        else if (match(specifier_tokens))
        {
            std::vector<TokenType> specifiers;

            while (match(specifier_tokens))
            {
                specifiers.emplace_back(current_token.type);
                advance();
            }

            elements.emplace_back(parse_var_decl(specifiers));
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

    if (match(TOKEN_SEMICOLON))
        return std::make_unique<RtnNode>(nullptr, SourceLocation{current_token.line, current_token.index});

    std::unique_ptr<ASTNode> expr = parse_expr();

    expect(TOKEN_SEMICOLON);

    return std::make_unique<RtnNode>(std::move(expr), SourceLocation{current_token.line, current_token.index});
}

std::unique_ptr<IfNode> Parser::parse_if_stmt()
{
    advance();
    expect_and_advance(TOKEN_LPAREN);

    std::unique_ptr<ASTNode> expr = parse_expr();

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

    return std::make_unique<IfNode>(std::move(expr), std::move(then_elements), std::move(else_elements), SourceLocation{current_token.line, current_token.index});
}

std::unique_ptr<WhileNode> Parser::parse_while_stmt()
{
    advance();

    expect_and_advance(TOKEN_LPAREN);

    std::unique_ptr<ASTNode> expr = parse_expr();

    expect_and_advance(TOKEN_RPAREN);

    std::vector<std::unique_ptr<ASTNode>> elements = parse_block();

    return std::make_unique<WhileNode>(std::move(expr), std::move(elements), SourceLocation{current_token.line, current_token.index});
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

    return std::make_unique<ForNode>(std::move(init), std::move(condition), std::move(post), std::move(elements), SourceLocation{current_token.line, current_token.index});
}

std::unique_ptr<ASTNode> Parser::parse_for_init()
{
    if (match(addressable_types))
        return parse_var_decl(std::nullopt);
    else if (match(TOKEN_IDENTIFIER))
        return parse_var_assign();
    else
        error("Parser Error: Expected valid for init");
}

std::unique_ptr<ASTNode> Parser::parse_loop_control()
{
    TokenType token_type = current_token.type;

    advance_and_expect(TOKEN_SEMICOLON);

    return std::make_unique<LoopControl>(token_type, "", SourceLocation{current_token.line, current_token.index});
}

// Parses name, type, array dims, etc for declaration
std::unique_ptr<VarNode> Parser::parse_var_declarator(const std::optional<std::vector<TokenType>> specifiers)
{
    Type var_type = parse_type();

    expect(TOKEN_IDENTIFIER);
    std::string var_name = current_token.text;
    advance();

    while (match(TOKEN_LSBRACE))
    {
        advance();

        /*
            Check for empty array declaration ie int arr[]; which is allowed for function parameters
        */
        if (match(TOKEN_RSBRACE))
        {
            var_type.add_array_dimension(-1);
            expect_and_advance(TOKEN_RSBRACE);
            break;
        }

        std::unique_ptr<ASTNode> size_expr = parse_expr();
        if (auto num = dynamic_cast<IntegerLiteral *>(size_expr.get()))
            var_type.add_array_dimension(num->value);
        else
            error("Array size must be a constant integer");
        expect_and_advance(TOKEN_RSBRACE);
    }

    std::vector<Specifier> specs;
    if (specifiers.has_value())
        specs = parse_tokens_to_specifiers(*specifiers);
    else
        specs = {};

    return std::make_unique<VarNode>(var_name, var_type, specs, SourceLocation{current_token.line, current_token.index});
}

// Uses method above to capture type of variable
// Specifier is static/extern
std::unique_ptr<VarDeclNode> Parser::parse_var_decl(const std::optional<std::vector<TokenType>> specifiers)
{
    std::unique_ptr<VarNode> var = parse_var_declarator(specifiers);
    Type var_type = var->type;
    std::vector<TokenType> excepted_tokens = std::vector<TokenType>{TOKEN_ASSIGN, TOKEN_SEMICOLON};
    expect(excepted_tokens);

    if (match(TOKEN_ASSIGN))
    {
        advance();
        std::unique_ptr<ASTNode> init_expr;

        if ((match(TOKEN_LBRACE) && var_type.is_array()) || (match(TOKEN_LBRACE) && var_type.is_struct()))
            init_expr = parse_aggregate_literal(var_type);
        else
            init_expr = parse_expr();

        expect(TOKEN_SEMICOLON);
        return std::make_unique<VarDeclNode>(std::move(var), std::move(init_expr), SourceLocation{current_token.line, current_token.index});
    }

    return std::make_unique<VarDeclNode>(std::move(var), nullptr, SourceLocation{current_token.line, current_token.index});
}

Type Parser::parse_type()
{
    std::vector<TokenType> types;

    while (match(addressable_types))
    {
        types.emplace_back(current_token.type);
        advance();
    }

    return determine_type(types);
}

Type Parser::determine_type(const std::vector<TokenType> &types)
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

    if (is_struct)
    {
        std::string struct_name = current_token.text;
        advance();

        /*
            Check for any pointer symbols following the struct name
            This is done here for cases like
            Struct Test *example
        */
        while (match(TOKEN_STAR))
        {
            ptr_level++;
            advance();
        }

        return Type(struct_name, ptr_level);
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
            return Type(current_token.text, ptr_level);
        case TOKEN_BOOL:
            base_type = BaseType::BOOL;
            break;
        case TOKEN_VOID:
            base_type = BaseType::VOID;
            break;
        default:
            continue;
        }
    }

    return Type(base_type, ptr_level);
}

std::unique_ptr<VarAssignNode> Parser::parse_var_assign()
{
    if (match(TOKEN_STAR))
    {
        advance_and_expect(TOKEN_IDENTIFIER);

        std::unique_ptr<VarNode> var = std::make_unique<VarNode>(current_token.text, SourceLocation{current_token.line, current_token.index});
        std::unique_ptr<UnaryNode> deref = std::make_unique<UnaryNode>(get_unary_op_type(TOKEN_STAR), std::move(var), SourceLocation{current_token.line, current_token.index});

        advance_and_expect(TOKEN_ASSIGN);

        advance();

        std::unique_ptr<ASTNode> expr = parse_expr();
        expect(TOKEN_SEMICOLON);
        return std::make_unique<VarAssignNode>(std::move(deref), std::move(expr), SourceLocation{current_token.line, current_token.index});
    }

    std::unique_ptr<ASTNode> target = parse_lvalue();
    TokenType assign_type = current_token.type;

    expect_and_advance(assign_tokens);

    std::unique_ptr<ASTNode> expr = parse_expr();

    if (assign_type == TOKEN_ASSIGN)
    {
        return std::make_unique<VarAssignNode>(std::move(target), std::move(expr), SourceLocation{current_token.line, current_token.index});
    }
    else
    {
        auto bin_op_type = get_bin_op_type(assign_type);

        std::unique_ptr<ASTNode> target_clone;
        if (auto var_node = dynamic_cast<VarNode *>(target.get()))
            target_clone = std::make_unique<VarNode>(*var_node);
        else if (auto array_access = dynamic_cast<ArrayAccessNode *>(target.get()))
            target_clone = std::make_unique<ArrayAccessNode>(*array_access, SourceLocation{current_token.line, current_token.index});

        auto right = std::make_unique<BinaryNode>(bin_op_type, std::move(target_clone), std::move(expr), SourceLocation{current_token.line, current_token.index});
        return std::make_unique<VarAssignNode>(std::move(target), std::move(right), SourceLocation{current_token.line, current_token.index});
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

        left = std::make_unique<PostfixNode>(current_token.type, std::move(left), SourceLocation{current_token.line, current_token.index});

        advance();
    }

    while (match(bin_op_tokens) && get_precedence(current_token.type) >= min_presedence)
    {
        TokenType op = current_token.type;

        advance();

        std::unique_ptr<ASTNode> right = parse_expr(get_precedence(op) + 1);

        left = std::make_unique<BinaryNode>(get_bin_op_type(op), std::move(left), std::move(right), SourceLocation{current_token.line, current_token.index});
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
            return std::make_unique<DoubleLiteral>(std::stod(current_token.text), SourceLocation{current_token.line, current_token.index});
        }
        catch (const std::exception &e)
        {
            error("Number out of range");
        }
    }
    else if (match(TOKEN_TRUE) || match(TOKEN_FALSE))
    {
        return std::make_unique<BoolLiteral>(current_token.type == TOKEN_TRUE, SourceLocation{current_token.line, current_token.index});
    }
    else if (match(TOKEN_CHAR))
    {
        return std::make_unique<CharLiteral>(current_token.text[0], Type(BaseType::CHAR), SourceLocation{current_token.line, current_token.index});
    }
    else if (match(TOKEN_STRING))
    {
        Type string_type(BaseType::CHAR);
        string_type.add_array_dimension(current_token.text.length());
        return std::make_unique<StringLiteral>(current_token.text, string_type, SourceLocation{current_token.line, current_token.index});
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

        std::unique_ptr<ASTNode> potential_var = parse_lvalue();

        if (match(TOKEN_LPAREN))
        {
            std::unique_ptr<FuncCallNode> func_call = std::make_unique<FuncCallNode>(identifier, SourceLocation{current_token.line, current_token.index});
            parse_args_list(func_call);
            expect(TOKEN_RPAREN);
            return func_call;
        }

        retreat();
        return potential_var;
    }
    else if (match(TOKEN_LBRACE))
    {
        auto rtn = parse_aggregate_literal();
        retreat();
        return rtn;
    }
    else if (match(TOKEN_SIZEOF))
        return parse_sizeof();
    else if (match(TOKEN_NULL))
    {
        return std::make_unique<NullLiteral>(SourceLocation{current_token.line, current_token.index});
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
            return std::make_unique<ULongLiteral>(std::stoull(num_text), SourceLocation{current_token.line, current_token.index});
        else if (is_unsigned)
            return std::make_unique<UIntegerLiteral>(std::stoul(num_text), SourceLocation{current_token.line, current_token.index});
        else if (is_long)
            return std::make_unique<LongLiteral>(std::stoll(num_text), SourceLocation{current_token.line, current_token.index});

        // Attempt to fit in smallest type possible
        try
        {
            return std::make_unique<IntegerLiteral>(std::stoi(num_text), SourceLocation{current_token.line, current_token.index});
        }
        catch (const std::out_of_range &)
        {
            return std::make_unique<LongLiteral>(std::stoll(num_text), SourceLocation{current_token.line, current_token.index});
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

    return std::make_unique<UnaryNode>(get_unary_op_type(op), std::move(expr), SourceLocation{current_token.line, current_token.index});
}

std::unique_ptr<AggregateLiteral> Parser::parse_aggregate_literal(const Type &array_type)
{
    expect_and_advance(TOKEN_LBRACE);

    auto init_node = std::make_unique<AggregateLiteral>(array_type, SourceLocation{current_token.line, current_token.index});

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

int Parser::get_precedence(const TokenType &op)
{
    return precedence_map.at(get_bin_op_type(op));
}

std::unique_ptr<ASTNode> Parser::parse_lvalue(const Specifier &specifier)
{
    expect(TOKEN_IDENTIFIER);
    std::string var_name = current_token.text;
    std::unique_ptr<VarNode> var = std::make_unique<VarNode>(var_name, SourceLocation{current_token.line, current_token.index});
    advance();

    if (match(TOKEN_LSBRACE))
    {
        advance();
        auto index = parse_expr();
        expect_and_advance(TOKEN_RSBRACE);

        return std::make_unique<ArrayAccessNode>(std::move(var), std::move(index), SourceLocation{current_token.line, current_token.index});
    }
    else if (match(TOKEN_DOT) || match(TOKEN_ARROW))
    {
        TokenType op = current_token.type;
        advance();
        expect(TOKEN_IDENTIFIER);

        std::string field_name = current_token.text;

        std::unique_ptr<ASTNode> test = parse_factor();

        std::unique_ptr<PostfixNode> postfix = std::make_unique<PostfixNode>(op, std::move(test), SourceLocation{current_token.line, current_token.index});
        postfix->struct_name = var_name;
        postfix->field_name = field_name;

        advance();
        return postfix;
    }
    else
        return var;
}

std::unique_ptr<ASTNode> Parser::parse_cast()
{
    expect_and_advance(TOKEN_LPAREN);

    Type type = parse_type();

    expect_and_advance(TOKEN_RPAREN);

    std::unique_ptr<ASTNode> factor = parse_factor();

    if (!factor)
        error("Expected expression after cast");

    return std::make_unique<CastNode>(std::move(factor), type, Type(BaseType::VOID), SourceLocation{current_token.line, current_token.index});
}

std::unique_ptr<ASTNode> Parser::parse_sizeof()
{
    expect_and_advance(TOKEN_SIZEOF);
    expect_and_advance(TOKEN_LPAREN);

    std::unique_ptr<SizeOfNode> sizeof_node = nullptr;

    if (match(TOKEN_IDENTIFIER))
    {
        std::string var_name = current_token.text;
        advance();
        sizeof_node = std::make_unique<SizeOfNode>(std::make_unique<VarNode>(var_name, SourceLocation{current_token.line, current_token.index}), SourceLocation{current_token.line, current_token.index});
    }
    else
        sizeof_node = std::make_unique<SizeOfNode>(parse_type(), SourceLocation{current_token.line, current_token.index});

    expect(TOKEN_RPAREN);

    return sizeof_node;
}

std::unique_ptr<ASTNode> Parser::parse_import()
{
    expect_and_advance(TOKEN_IMPORT);
    expect_and_advance(TOKEN_LBRACE);

    std::vector<std::string> includes;

    while (!match(TOKEN_RBRACE))
    {
        if (match(TOKEN_STRUCT))
        {
            advance();

            expect(TOKEN_IDENTIFIER);

            includes.push_back("struct " + current_token.text);
        }
        else
            includes.push_back(current_token.text);

        advance();
    }

    expect_and_advance(TOKEN_RBRACE);

    expect_and_advance(TOKEN_FROM);

    std::string module_name = current_token.text;

    advance_and_expect(TOKEN_SEMICOLON);

    return std::make_unique<IncludeNode>(module_name, includes, SourceLocation{current_token.line, current_token.index});
}