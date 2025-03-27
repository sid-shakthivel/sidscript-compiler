#pragma once

#include <unordered_map>
#include <string>
#include <stack>

enum TokenType
{
    TOKEN_EOF,
    TOKEN_INT,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_RTN,
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
    TOKEN_PERCENT,
    TOKEN_ASSIGN,
    TOKEN_EQUALS,
    TOKEN_NOT_EQUALS,
    TOKEN_EXCLAMATION,
    TOKEN_TILDA,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LE,
    TOKEN_GE,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_BOOL,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_INCREMENT,
    TOKEN_DECREMENT,
    TOKEN_ARROW,
    TOKEN_QUESTION_MARK,
    TOKEN_COLON,
    TOKEN_CONTINUE,
    TOKEN_BREAK,
    TOKEN_COMMA,
    TOKEN_VOID,
    TOKEN_STATIC,
    TOKEN_EXTERN,
    TOKEN_LONG,
    TOKEN_UNSIGNED,
    TOKEN_SIGNED,
    TOKEN_DOUBLE,
    TOKEN_FPN,
    TOKEN_AMPERSAND,
    TOKEN_PLUS_EQUALS,
    TOKEN_MINUS_EQUALS,
    TOKEN_STAR_EQUALS,
    TOKEN_SLASH_EQUALS,
    TOKEN_MODULUS_EQUALS,
    TOKEN_UNKNOWN_SYMBOL,
    TOKEN_LSBRACE, // Left Square Brace
    TOKEN_RSBRACE,
    TOKEN_CHAR,
    TOKEN_STRING,
    TOKEN_CHAR_TEXT,
    TOKEN_STRUCT,
    TOKEN_DOT,
    TOKEN_TRUE,
    TOKEN_FALSE,
};

extern std::unordered_map<std::string, TokenType> string_to_token;
extern std::string token_to_string(TokenType token_type);

struct Token
{
public:
    TokenType type;
    std::string text;
    size_t line;
    size_t index;

    Token(TokenType t, const std::string &txt, size_t l, size_t i);
};

class Lexer
{
public:
    Lexer(const std::string &source);
    Token get_next_token();
    Token rewind(int iterations = 1);
    void print_all_tokens();
    void print_stack();

private:
    std::string source;
    size_t index;
    size_t line = 1;

    struct LexerState
    {
        size_t index;
        size_t line;
    };

    std::stack<LexerState> state_stack;

    std::string process_number();
    std::string process_identifier();
    std::string process_symbol();
    Token process_char();
    Token process_string();
};