#pragma once

#include <string>
#include <unordered_map>

enum class TokenType
{
    // End of file
    EOF_TOKEN,

    // Keywords
    INT,
    IF,
    ELSE,
    WHILE,
    FOR,
    RTN,
    FN,
    CONTINUE,
    BREAK,
    VOID,
    STATIC,
    EXTERN,
    LONG,
    UNSIGNED,
    SIGNED,
    DOUBLE,
    CHAR,

    // Delimiters
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LSBRACE,
    RSBRACE,
    SEMICOLON,
    COMMA,
    COLON,

    // Operators
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    ASSIGN,
    EQUALS,
    NOT_EQUALS,
    EXCLAMATION,
    TILDA,
    LESS_THAN,
    GREATER_THAN,
    LESS_OR_EQUAL,
    GREATER_OR_EQUAL,
    AND,
    OR,
    INCREMENT,
    DECREMENT,
    ARROW,
    QUESTION_MARK,
    AMPERSAND,

    // Compound assignments
    PLUS_EQUALS,
    MINUS_EQUALS,
    STAR_EQUALS,
    SLASH_EQUALS,
    MODULUS_EQUALS,

    // Literals and identifiers
    IDENTIFIER,
    NUMBER,
    BOOL,
    STRING,
    CHAR_TEXT,
    FPN,
    UNKNOWN_SYMBOL
};

// Token string representations - moved from lexer.h
extern std::unordered_map<std::string, TokenType> string_to_token;
std::string token_to_string(TokenType token_type);

struct Token
{
    TokenType type;
    std::string text;
    size_t line;

    Token(TokenType t, const std::string &txt, size_t l)
        : type(t), text(txt), line(l) {}
};
