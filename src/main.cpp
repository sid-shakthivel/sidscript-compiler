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

    std::shared_ptr<ProgramNode> program = parser.parse();
    program->print();

    std::shared_ptr<GlobalSymbolTable> gst = std::make_shared<GlobalSymbolTable>();

    SemanticAnalyser semanticAnalyser(gst);
    semanticAnalyser.analyse(program);

    gst->print();

    TacGenerator tacGenerator(gst);
    std::vector<TACInstruction> tacInstructions = tacGenerator.generate_tac(program);
    tacGenerator.print_all_tac();

    Assembler assembler(gst);
    assembler.assemble(tacInstructions, "test.s");

    return 0;
}