#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <memory>

#include "../include/module.h"
#include "../include/globalSymbolTable.h"

int main(int argc, char *argv[])
{
    std::shared_ptr<GlobalSymbolTable> gst = std::make_shared<GlobalSymbolTable>();

    std::unordered_map<std::string, int> modules;

    for (int i = 1; i < argc; ++i)
    {
        if (modules.find(argv[i]) != modules.end())
        {
            std::cerr << "Compiler Error: Duplicate module name: " << argv[i] << std::endl;
            return 1;
        }

        Module module(argv[i], gst);
        module.compile();
    }

    // gst->print();
    gst->check_imports();

    return 0;
}
