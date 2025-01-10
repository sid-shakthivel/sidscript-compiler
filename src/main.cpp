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
    // lexer.print_all_tokens();

    Parser parser(&lexer);

    ProgramNode *program = parser.parse();
    // program->print(0);

    Assembler assembler;

    assembler.assemble(program, "test.s");

    return 0;
}