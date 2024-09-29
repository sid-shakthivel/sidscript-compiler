#pragma once

#include <unordered_map>

enum TokenType
{
    TOKEN_EOF,
    TOKEN_INT_TEXT,
    TOKEN_FLOAT_TEXT,
    TOKEN_BOOL_TEXT,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_RETURN,
    TOKEN_FN,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_ASSIGN,
    TOKEN_EQUALS,
    TOKEN_NOT_EQUALS,
    TOKEN_EXCLAMATION,
    TOKEN_TILDA,
    TOKEN_LESS_THAN,
    TOKEN_GREATER_THAN,
    TOKEN_IDENTIFIER,
    TOKEN_INT,
    TOKEN_BOOL,
    TOKEN_FLOAT,
    TOKEN_UNKNOWN_SYMBOL,
};

extern std::unordered_map<std::string, TokenType> string_to_token;

struct Token
{
public:
    TokenType type;
    std::string text;

    Token(TokenType t, const std::string &txt) : type(t), text(txt) {}
};

class Lexer
{
public:
    Lexer(const std::string &source);
    Token get_next_token();

private:
    std::string source;
    size_t index;

    std::string process_number();
    std::string process_identifier();
    std::string process_symbol();
};