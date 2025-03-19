#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <memory>

#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/assembler.h"
#include "../include/ast.h"
#include "../include/semanticAnalyser.h"
#include "../include/tacGenerator.h"

int main()
{
    std::string file_line;
    std::ifstream code_file("../tests/test.ss");

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
    // lexer.print_stack();

    Parser parser(&lexer);

    std::shared_ptr<ProgramNode> program = parser.parse();

    program->print(0);

    std::shared_ptr<GlobalSymbolTable> gst = std::make_shared<GlobalSymbolTable>();

    SemanticAnalyser semanticAnalyser(gst);
    semanticAnalyser.analyse(program);

    // TacGenerator tacGenerator(gst);
    // tacGenerator.generate_tac(program);
    // tacGenerator.print_all_tac();

    // gst->print();

    // auto &instructions = tacGenerator.get_instructions();

    // Assembler assembler(gst, "test.s");
    // assembler.assemble(instructions);

    return 0;
}
