#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <map>

enum class BaseType
{
    INT,
    LONG,
    UINT,
    ULONG,
    DOUBLE,
    VOID,
    STRUCT,
    CHAR,
    BOOL,
};

class Type
{
private:
    BaseType base_type;
    int ptr_level = 0;
    std::vector<int> array_sizes;
    std::optional<std::string> struct_name;
    std::map<std::string, std::pair<Type, int>> struct_fields;

public:
    Type() : base_type(BaseType::VOID), ptr_level(0) {}
    Type(BaseType base);
    Type(BaseType base, int ptr_level);
    Type(std::string struct_name, int ptr_level);

    Type &add_array_dimension(int size);

    bool is_pointer() const;
    bool is_array() const;
    bool is_struct() const;
    BaseType get_base_type() const;
    bool has_base_type(BaseType other) const;
    int get_ptr_depth() const;

    bool is_signed() const;

    std::string to_string() const;
    void print();

    size_t get_size() const;
    size_t get_array_size() const;
    bool is_size_8() const;

    std::string get_struct_name() const;

    // Type compatibility checks
    bool can_assign_from(const Type &other) const;
    bool can_convert_to(const Type &other) const;

    // Equality operators
    bool operator==(const Type &other) const;
    bool operator!=(const Type &other) const;

    bool is_integral() const;

    void add_field(const std::string &name, const Type &type);
    int get_field_offset(const std::string &field_name) const;
    std::string get_field_name(int index) const;
};