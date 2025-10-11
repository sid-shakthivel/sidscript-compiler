#pragma once

#include <string>

#include "../include/globalSymbolTable.h"

class Module
{
public:
    std::string name;
    std::string filepath;

    Module(const std::string &path, std::shared_ptr<GlobalSymbolTable> gst);
    void compile();

private:
    std::string file_contents;
    std::shared_ptr<GlobalSymbolTable> gst;

    void check_file();
};
