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

Parser::Parser(Lexer *l) : lexer(l), current_token(TOKEN_EOF, "")
{
    advance();
}

ProgramNode *Parser::parse()
{
    FuncNode *main_func = parse_func();
    ProgramNode *program = new ProgramNode(main_func);

    return program;
}

bool Parser::match(TokenType type)
{
    return current_token.type == type;
}

bool Parser::match(std::vector<TokenType> &end_tokens)
{
    for (auto token : end_tokens)
    {
        if (current_token.type == token)
            return true;
    }
}

void Parser::advance()
{
    current_token = lexer->get_next_token();
}

void Parser::error(const std::string &message)
{
    throw std::runtime_error("Parser Error: " + message + " but found " + current_token.text);
}

void Parser::expect(TokenType token_type)
{
    if (!(current_token.type == token_type))
    {
        std::string expected_token = token_to_string(token_type);
        error("Expected " + expected_token);
    }
}

FuncNode *Parser::parse_func()
{
    expect(TOKEN_FN);

    advance();

    expect(TOKEN_IDENTIFIER);

    std::string func_name = current_token.text;

    advance();

    expect(TOKEN_LPAREN);

    advance();

    expect(TOKEN_RPAREN);

    advance();

    expect(TOKEN_LBRACE);

    advance();

    std::vector<ASTNode *> stmts = parse_stmts();

    advance();

    expect(TOKEN_RBRACE);

    return new FuncNode(func_name, stmts);
}

std::vector<ASTNode *> Parser::parse_stmts()
{
    std::vector<ASTNode *> stmts;

    if (match(TOKEN_RTN))
    {
        stmts.emplace_back(parse_rtn());
    }

    return stmts;
}

RtnNode *Parser::parse_rtn()
{
    advance();

    ASTNode *expr = parse_expr(0);

    expect(TOKEN_SEMICOLON);

    return new RtnNode(expr);
}

ASTNode *Parser::parse_expr(int min_presedence = 0)
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
    else if (match(TOKEN_TILDA) || match(TOKEN_MINUS))
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

        advance();

        expect(TOKEN_RPAREN);

        return expr;
    }
}

int Parser::get_precedence(TokenType op)
{
    return precedence_map.at(get_bin_op_type(op));
}

// // <var_decl> ::= <basic_type> <id> "=" <expr> ";"
// VarDecl *Parser::parse_var_decl()
// {
//     TokenType var_type = current_token.type;
//     advance();

//     expect(TOKEN_IDENTIFIER);

//     std::string var_identifier = current_token.text;

//     advance();

//     expect(TOKEN_ASSIGN);

//     advance();

//     ASTNode *expr = parse_expr({TOKEN_SEMICOLON});

//     return new VarDecl(var_identifier, expr, var_type);
// }

// // <var_assign> ::= <id> "=" <expr> ";"
// VarAssign *Parser::parse_var_assign()
// {
//     std::string var_identifier = current_token.text;

//     advance();

//     expect(TOKEN_ASSIGN);

//     advance();

//     ASTNode *expr = parse_expr({TOKEN_SEMICOLON});

//     return new VarAssign(var_identifier, expr);
// }

// IfStmt *Parser::parse_if_stmt()
// {
//     advance();

//     expect(TOKEN_LPAREN);

//     advance();

//     advance();

//     expect(TOKEN_LBRACE);

//     // Parse statements

//     expect(TOKEN_RBRACE);

//     return nullptr;
// }

// Condition *Parser::parse_condition()
// {
//     ASTNode *left = parse_expr({TOKEN_EQUALS, TOKEN_NOT_EQUALS, TOKEN_LT, TOKEN_GT, TOKEN_LE, TOKEN_GE});

//     TokenType op = current_token.type;

//     advance();

//     ASTNode *right = parse_expr({TOKEN_EQUALS, TOKEN_NOT_EQUALS, TOKEN_LT, TOKEN_GT, TOKEN_LE, TOKEN_GE});

//     return new Condition(op, left, right);
// }

// // <expr> ::= <expr> ("+" | "-") <term> | <term>
// ASTNode *Parser::parse_expr(std::vector<TokenType> end_tokens)
// {
//     // Expect a proper term
//     ASTNode *left_term = parse_term(end_tokens);

//     while (!match(end_tokens))
//     {
//         TokenType op = current_token.type;

//         advance();

//         ASTNode *right_term = parse_term(end_tokens);

//         left_term = new BinaryExpression(left_term, right_term, op);
//     }

//     return left_term;
// }

// ASTNode *Parser::parse_term(std::vector<TokenType> &end_tokens)
// {
//     ASTNode *left_factor = parse_factor();

//     advance();

//     while (!match(end_tokens))
//     {
//         TokenType op = current_token.type;

//         advance();

//         ASTNode *right_factor = parse_term(end_tokens);

//         left_factor = new BinaryExpression(left_factor, right_factor, op);
//     }

//     return left_factor;
// }

// FactorNode *Parser::parse_factor()
// {
//     if (match(TOKEN_MINUS) || match(TOKEN_EXCLAMATION) || match(TOKEN_TILDA))
//     {
//         TokenType op = current_token.type;

//         advance();

//         auto literal = parse_factor()->value;
//         return new UnaryExpression(op, literal);
//     }

//     // Return AST node
//     if (match(TOKEN_INT))
//     {
//         int value = std::stoi(current_token.text);
//         return new FactorNode(new IntegerLiteral(value));
//     }
//     else if (match(TOKEN_FLOAT))
//     {
//         float value = std::stof(current_token.text);
//         return new FactorNode(new FloatLiteral(value));
//     }
//     else if (match(TOKEN_BOOL))
//     {
//         bool value = current_token.text == "true";
//         return new FactorNode(new BoolLiteral(value));
//     }

//     error("Unknown factor");
// }
