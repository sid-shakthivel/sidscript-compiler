#include <string>
#include <iostream>
#include <unordered_set>

#include "../include/lexer.h"

Token::Token(TokenType t, const std::string &txt, size_t l, size_t i)
    : type(t), text(txt), line(l), index(i) {}

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
    {"char", TOKEN_CHAR_TEXT},
    {"struct", TOKEN_STRUCT},
    {".", TOKEN_DOT},
    {"true", TOKEN_TRUE},
    {"false", TOKEN_FALSE},
    {"bool", TOKEN_BOOL},
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
            throw std::runtime_error("Lexer Error: Invalid number format on line " + std::to_string(line));
    }

    // Validate the number format
    if (temp_number == ".")
        throw std::runtime_error("Lexer Error: Invalid number format on line " + std::to_string(line));

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

Token Lexer::process_char()
{
    size_t init_index = index;
    char value;

    if (source[++index] == '\\')
    {
        index += 1;
        switch (source[index])
        {
        case 'n':
            value = '\n';
            break;
        case 't':
            value = '\t';
            break;
        case '\\':
            value = '\\';
            break;
        case '\'':
            value = '\'';
            break;
        case '"':
            value = '"';
            break;
        default:
            throw std::runtime_error("Lexer Error: Invalid escape sequence on line " + std::to_string(line));
        }
    }
    else
    {
        value = source[index];
    }

    if (source[++index] != '\'')
        throw std::runtime_error("Lexer Error: Unterminanted char on line " + std::to_string(line));

    index += 1;
    state_stack.push({init_index, line});
    return Token(TOKEN_CHAR, std::string(1, value), line, init_index);
}

Token Lexer::process_string()
{
    size_t init_index = index;
    std::string str;

    index += 1;

    while (source[index] != '"' && index <= source.length())
    {
        if (source[index] == '\\')
        {
            index += 1;
            switch (source[index])
            {
            case 'n':
                str += '\n';
                break;
            case 't':
                str += '\t';
                break;
            case '\\':
                str += '\\';
                break;
            case '"':
                str += '"';
                break;
            default:
                str += source[index];
            }
        }
        else
        {
            str += source[index];
        }
        index += 1;
    }

    if (source[index] != '"')
        throw std::runtime_error("Lexer Error: Unterminated string on line " + std::to_string(line));

    index += 1;
    state_stack.push({init_index, line});
    return Token(TOKEN_STRING, str, line, init_index);
}

Token Lexer::get_next_token()
{
    if (index >= source.length())
        return Token(TOKEN_EOF, "", line, index);

    char c = source[index];

    if (c == '\n')
        line++;

    if (c == '\'')
        return process_char();
    else if (c == '"')
        return process_string();
    else if (isdigit(c) || c == '.')
    {
        size_t init_index = index;

        if (c == '.' && !isdigit(source[index + 1]))
        {
            index += 1;
            state_stack.push({init_index, line});
            return Token(TOKEN_DOT, ".", line, init_index);
        }

        std::string num = process_number();
        if (num.find('.') != std::string::npos ||
            num.find('e') != std::string::npos ||
            num.find('E') != std::string::npos)
        {
            state_stack.push({init_index, line});
            return Token(TOKEN_FPN, num, line, init_index);
        }
        state_stack.push({init_index, line});
        return Token(TOKEN_NUMBER, num, line, init_index);
    }
    else if (isalpha(c))
    {
        size_t init_index = index;
        std::string temp_identifier = process_identifier();

        auto it = string_to_token.find(temp_identifier);
        TokenType token_type = (it != string_to_token.end()) ? it->second : TOKEN_IDENTIFIER;

        state_stack.push({init_index, line});
        return Token(token_type, temp_identifier, line, init_index);
    }
    else if (!isspace(c))
    {
        size_t init_index = index;
        std::string temp_symbol = process_symbol();

        auto it = string_to_token.find(temp_symbol);
        TokenType token_type = (it != string_to_token.end()) ? it->second : TOKEN_UNKNOWN_SYMBOL;

        if (token_type == TOKEN_UNKNOWN_SYMBOL)
            std::cout << "Lexer Error: Unknown symbol\n";

        state_stack.push({init_index, line});
        return Token(token_type, temp_symbol, line, init_index);
    }

    index++;
    return get_next_token();
}

Token Lexer::rewind(int iterations)
{
    if (state_stack.size() > 1)
    {
        for (int i = 0; i < iterations; i++)
            state_stack.pop();

        LexerState state = state_stack.top();
        index = state.index;
        line = state.line;
        return get_next_token();
    }

    state_stack.push({index, line});
    return Token(TOKEN_EOF, "", line, index);
}

void Lexer::print_all_tokens()
{
    Token next_token = get_next_token();
    while (next_token.type != TOKEN_EOF)
    {
        // std::cout << next_token.text << " " << next_token.index << " " << source[next_token.index] << std::endl;
        std::cout << next_token.text << std::endl;
        next_token = get_next_token();
    }
}

void Lexer::print_stack()
{
    std::stack<LexerState> temp_stack = state_stack;
    std::cout << "Stack size is " << temp_stack.size() << std::endl;
    std::cout << "Stack contents (top to bottom):\n";
    while (!temp_stack.empty())
    {
        LexerState state = temp_stack.top();
        std::cout << "Index: " << state.index << ", Line: " << state.line << ", Char: " << source[state.index] << "\n";
        temp_stack.pop();
    }
}