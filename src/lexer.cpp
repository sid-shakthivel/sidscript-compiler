#include <string>
#include <iostream>
#include <unordered_set>

#include "../include/lexer.h"

std::unordered_map<std::string, TokenType> string_to_token = {
    {"int", TOKEN_INT},
    {"void", TOKEN_VOID},
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
    {"&&", TOKEN_AND},
    {"||", TOKEN_OR},
    {"++", TOKEN_INCREMENT},
    {"--", TOKEN_DECREMENT},
    {"true", TOKEN_BOOL},
    {"false", TOKEN_BOOL},
    {"return", TOKEN_RTN},
    {"?", TOKEN_QUESTION_MARK},
    {":", TOKEN_COLON},
    {"->", TOKEN_ARROW},
    {"continue", TOKEN_CONTINUE},
    {",", TOKEN_COMMA},
    {"break", TOKEN_BREAK},
    {"long", TOKEN_LONG},
    {"static", TOKEN_STATIC},
    {"extern", TOKEN_EXTERN},
    {"unsigned", TOKEN_UNSIGNED},
    {"signed", TOKEN_SIGNED},
    {"double", TOKEN_DOUBLE},
    {"&", TOKEN_AMPERSAND},
    {"+=", TOKEN_PLUS_EQUALS},
    {"-=", TOKEN_MINUS_EQUALS},
    {"*=", TOKEN_STAR_EQUALS},
    {"/=", TOKEN_SLASH_EQUALS},
    {"%=", TOKEN_MODULUS_EQUALS},
    {"[", TOKEN_LSBRACE},
    {"]", TOKEN_RSBRACE},
};

std::string token_to_string(TokenType token_type)
{
    for (const auto &pair : string_to_token)
        if (pair.second == token_type)
            return pair.first;
    return "";
}

Lexer::Lexer(const std::string &src) : source(src), index(0) {}

std::string Lexer::process_number()
{
    std::string temp_number;
    bool has_decimal = false;
    bool has_exponent = false;

    // Process integer part or leading decimal
    while (index < source.length() && (std::isdigit(source[index]) || source[index] == '.'))
    {
        if (source[index] == '.')
        {
            if (has_decimal)
                break; // Second decimal point - stop
            has_decimal = true;
        }
        temp_number += source[index++];
    }

    // Process exponent part if it exists
    if (index < source.length() && (source[index] == 'e' || source[index] == 'E'))
    {
        temp_number += source[index++];
        has_exponent = true;

        // Handle optional sign in exponent
        if (index < source.length() && (source[index] == '+' || source[index] == '-'))
            temp_number += source[index++];

        // Process exponent digits
        bool has_exp_digits = false;
        while (index < source.length() && std::isdigit(source[index]))
        {
            temp_number += source[index++];
            has_exp_digits = true;
        }

        // If no digits after E, it's invalid
        if (!has_exp_digits)
            throw std::runtime_error("Lexer Error: Invalid number format on line " + line);
    }

    // Validate the number format
    if (temp_number == ".")
        throw std::runtime_error("Lexer Error: Invalid number format on line " + line);

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
    static const std::unordered_set<std::string> multi_char_symbols = {
        "&&", "||", "==", "!=", ">=", "<=", "++", "--", "->", "+=", "-=", "*=", "/=", "%="};

    std::string symbol(1, source[index++]);

    if (index < source.length())
    {
        std::string potential_symbol = symbol + source[index];
        if (multi_char_symbols.find(potential_symbol) != multi_char_symbols.end())
        {
            symbol = potential_symbol;
            index++;
        }
    }

    return symbol;
}

Token Lexer::get_next_token()
{
    if (index >= source.length())
        return Token(TOKEN_EOF, "", line);

    shadow_index = index;
    char c = source[index];

    if (c == '\n')
        line++;

    if (isdigit(c) || c == '.')
    {
        std::string num = process_number();
        if (num.find('.') != std::string::npos ||
            num.find('e') != std::string::npos ||
            num.find('E') != std::string::npos)
        {
            return Token(TOKEN_FPN, num, line);
        }
        return Token(TOKEN_NUMBER, num, line);
    }
    else if (isalpha(c))
    {
        std::string temp_identifier = process_identifier();

        auto it = string_to_token.find(temp_identifier);
        TokenType token_type = (it != string_to_token.end()) ? it->second : TOKEN_IDENTIFIER;

        return Token(token_type, temp_identifier, line);
    }
    else if (!isspace(c))
    {
        std::string temp_symbol = process_symbol();

        auto it = string_to_token.find(temp_symbol);
        TokenType token_type = (it != string_to_token.end()) ? it->second : TOKEN_UNKNOWN_SYMBOL;

        if (token_type == TOKEN_UNKNOWN_SYMBOL)
            std::cout << "Lexer Error: Unknown symbol\n";

        return Token(token_type, temp_symbol, line);
    }

    index++;
    return get_next_token();
}

void Lexer::rewind()
{
    index = shadow_index;
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