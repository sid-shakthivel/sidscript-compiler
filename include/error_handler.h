#pragma once

#include <string>
#include <stdexcept>
#include <sstream>

class CompilerError : public std::runtime_error
{
public:
    CompilerError(const std::string &message, size_t line, size_t column)
        : std::runtime_error(formatMessage(message, line, column)),
          line_(line), column_(column) {}

    size_t getLine() const { return line_; }
    size_t getColumn() const { return column_; }

private:
    size_t line_;
    size_t column_;

    static std::string formatMessage(const std::string &message, size_t line, size_t column)
    {
        std::stringstream ss;
        ss << "Error at line " << line << ", column " << column << ": " << message;
        return ss.str();
    }
};

class LexerError : public CompilerError
{
public:
    LexerError(const std::string &message, size_t line, size_t column)
        : CompilerError("Lexer: " + message, line, column) {}
};

class ParserError : public CompilerError
{
public:
    ParserError(const std::string &message, size_t line, size_t column)
        : CompilerError("Parser: " + message, line, column) {}
};
