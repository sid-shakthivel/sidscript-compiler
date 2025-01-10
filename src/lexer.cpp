#include <string>
#include <iostream>

#include "../include/lexer.h"

std::unordered_map<std::string, TokenType> string_to_token = {
    {"int", TOKEN_INT_TEXT},
    {"float", TOKEN_FLOAT_TEXT},
    {"bool", TOKEN_BOOL_TEXT},
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE},
    {"while", TOKEN_WHILE},
    {"for", TOKEN_FOR},
    {"fn", TOKEN_FN},
    {"(", TOKEN_LPAREN},
    {")", TOKEN_RPAREN},
    {"{", TOKEN_LBRACE},
    {"}", TOKEN_RBRACE},
    {";", TOKEN_SEMICOLON},
    {"+", TOKEN_PLUS},
    {"-", TOKEN_MINUS},
    {"*", TOKEN_STAR},
    {"/", TOKEN_SLASH},
    {"%", TOKEN_PERCENT},
    {"=", TOKEN_ASSIGN},
    {"==", TOKEN_EQUALS},
    {"!=", TOKEN_NOT_EQUALS},
    {"!", TOKEN_EXCLAMATION},
    {"~", TOKEN_TILDA},
    {"<", TOKEN_LT},
    {">", TOKEN_GT},
    {"<=", TOKEN_LT},
    {">=", TOKEN_GT},
    {"true", TOKEN_BOOL},
    {"false", TOKEN_BOOL},
    {"return", TOKEN_RTN},
};

std::string token_to_string(TokenType token_type)
{
    for (const auto &pair : string_to_token)
    {
        if (pair.second == token_type)
        {
            return pair.first;
        }
    }
    return "";
}

Lexer::Lexer(const std::string &src) : source(src), index(0) {}

std::string Lexer::process_number()
{
    std::string temp_number;

    while (index < source.length() && std::isdigit(source[index]))
        temp_number += source[index++];

    return temp_number;
}

std::string Lexer::process_identifier()
{
    std::string temp_identifier;

    while (index < source.length() && std::isalnum(source[index]))
        temp_identifier += source[index++];

    return temp_identifier;
}

std::string Lexer::process_symbol()
{
    std::string symbol(1, source[index++]);
    return symbol;
}

Token Lexer::get_next_token()
{
    if (index >= source.length())
    {
        return Token(TOKEN_EOF, "");
    }

    char c = source[index];

    if (isdigit(c))
    {
        return Token(TOKEN_INT, process_number());
    }
    else if (isalpha(c))
    {
        std::string temp_identifier = process_identifier();

        auto it = string_to_token.find(temp_identifier);
        TokenType token_type = (it != string_to_token.end()) ? it->second : TOKEN_IDENTIFIER;

        return Token(token_type, temp_identifier);
    }
    else if (!isspace(c))
    {
        std::string temp_symbol = process_symbol();

        auto it = string_to_token.find(temp_symbol);
        TokenType token_type = (it != string_to_token.end()) ? it->second : TOKEN_UNKNOWN_SYMBOL;

        return Token(token_type, temp_symbol);
    }

    index++;
    return get_next_token();
}

void Lexer::print_all_tokens()
{
    Token next_token = get_next_token();
    while (next_token.type != TOKEN_EOF)
    {
        std::cout << next_token.text << "\n";
        next_token = get_next_token();
    }
}