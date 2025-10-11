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

    for (int i = 1; i < argc; ++i)
    {
        Module module(argv[i], gst);
        module.compile();
    }

    return 0;
}
