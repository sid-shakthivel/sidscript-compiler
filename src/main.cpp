#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/assembler.h"
#include "../include/ast.h"

int main()
{
    std::string file_line;
    std::ifstream code_file("../example_code/test.ss");

    if (!code_file)
    {
        std::cerr << "Error opening file!" << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << code_file.rdbuf();
    std::string file_contents = buffer.str();

    code_file.close();

    Lexer lexer(file_contents);

    // Token next_token = lexer.get_next_token();

    // while (next_token.type != TOKEN_EOF)
    // {
    //     std::cout << next_token.text << "\n";
    //     next_token = lexer.get_next_token();
    // }

    Parser parser(&lexer);

    ProgramNode *program = parser.parse();

    Assembler assembler;

    assembler.assemble(program, "test.s");

    return 0;
}