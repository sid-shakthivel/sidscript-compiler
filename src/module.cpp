#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "../include/module.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/assembler.h"
#include "../include/ast.h"
#include "../include/semanticAnalyser.h"
#include "../include/tacGenerator.h"

Module::Module(const std::string &path, std::shared_ptr<GlobalSymbolTable> gst) : gst(gst)
{
    auto get_filename = [](const std::string &filepath) -> std::string
    {
        size_t last_slash = filepath.find_last_of("/\\");
        size_t last_dot = filepath.find_last_of('.');

        size_t start = (last_slash == std::string::npos) ? 0 : last_slash + 1;
        size_t end = (last_dot == std::string::npos) ? filepath.length() : last_dot;

        return filepath.substr(start, end - start);
    };

    filepath = path;
    name = get_filename(filepath);

    check_file();
}

void Module::check_file()
{
    std::ifstream code_file(filepath, std::ios::in);

    if (!code_file)
        throw std::runtime_error("File Error: Error opening file: " + filepath);

    std::stringstream buffer;
    buffer << code_file.rdbuf();

    file_contents = buffer.str();

    code_file.close();

    if (file_contents.empty())
        throw std::runtime_error("File Error: File is empty: " + filepath);
}

void Module::compile()
{
    gst->current_module = name;

    Lexer lexer(file_contents);

    Parser parser(lexer, name);

    std::shared_ptr<ProgramNode> program = parser.parse();

    std::shared_ptr<SemanticAnalyser> sem_analyser = std::make_shared<SemanticAnalyser>(gst, name);
    sem_analyser->analyse(program);

    // program->print();

    TacGenerator tacGenerator(gst, sem_analyser);

    tacGenerator.generate_all_tac(program);

    auto &instructions = tacGenerator.get_instructions();

    // tacGenerator.print_all_tac();

    Assembler assembler(gst, name + ".s");
    assembler.assemble(instructions);
}