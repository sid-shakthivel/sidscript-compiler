#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <memory>

#include "../include/module.h"
#include "../include/globalSymbolTable.h"

int main()
{
    std::shared_ptr<GlobalSymbolTable> gst = std::make_shared<GlobalSymbolTable>();

    Module module("../tests/test.ss", gst);
    module.compile();

    return 0;
}
