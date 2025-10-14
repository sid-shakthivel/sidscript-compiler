#include "../include/type.h"

Type::Type(BaseType base) : base_type(base) {}

Type::Type(BaseType base, int ptr_level) : base_type(base), ptr_level(ptr_level) {}

Type::Type(std::string given_struct_name, int ptr_level) : base_type(BaseType::STRUCT), struct_name(std::move(given_struct_name)), ptr_level(ptr_level) {}

Type &Type::add_array_dimension(int size)
{
    array_sizes.push_back(size);
    return *this;
}

bool Type::is_pointer() const { return ptr_level > 0; }
bool Type::is_array() const { return array_sizes.size() > 0; }
bool Type::is_struct() const { return base_type == BaseType::STRUCT; }
bool Type::is_void() const { return base_type == BaseType::VOID; }

BaseType Type::get_base_type() const { return base_type; }

// Return size in bytes
size_t Type::get_size() const
{
    size_t base_size;

    if (is_pointer())
        return 8;

    switch (base_type)
    {
    case BaseType::BOOL:
        base_size = 1;
        break;
    case BaseType::CHAR:
        base_size = 1;
        break;
    case BaseType::INT:
    case BaseType::UINT:
        base_size = 4;
        break;
    case BaseType::LONG:
    case BaseType::ULONG:
    case BaseType::DOUBLE:
        base_size = 8;
        break;
    case BaseType::VOID:
        base_size = 0;
        break;
    case BaseType::STRUCT:
        base_size = 0;
        break;
    case BaseType::NULL_TYPE:
        base_size = 0;
        break;
    }

    if (is_array())
    {
        size_t total_size = base_size;
        for (int size : array_sizes)
            total_size *= size;
        return total_size;
    }

    if (is_struct())
    {
        base_size = 0;
        for (const auto &field : struct_fields)
            base_size += field.second.first.get_size();
    }

    return base_size;
}

size_t Type::get_array_length() const
{
    if (!is_array())
        return 0;

    size_t total_size = 1;

    for (int size : array_sizes)
        total_size *= size;

    return total_size;
}

size_t Type::get_base_size() const
{
    size_t base_size;

    if (is_pointer())
        return 8;

    switch (base_type)
    {
    case BaseType::BOOL:
        base_size = 1;
        break;
    case BaseType::CHAR:
        base_size = 1;
        break;
    case BaseType::INT:
    case BaseType::UINT:
        base_size = 4;
        break;
    case BaseType::LONG:
    case BaseType::ULONG:
    case BaseType::DOUBLE:
        base_size = 8;
        break;
    case BaseType::VOID:
        base_size = 0;
        break;
    case BaseType::STRUCT:
        base_size = 0;
        break;
    }

    return base_size;
}

bool Type::is_size_8() const
{
    return get_size() == 8;
}

int Type::get_ptr_depth() const
{
    return ptr_level;
}

void Type::set_ptr_depth(int ptr_depth)
{
    ptr_level = ptr_depth;
}

std::string Type::get_struct_name() const
{
    if (!struct_name.has_value())
        throw std::runtime_error("Type Error: Attempting to get struct name from type " + to_string());
    return struct_name.value();
}

std::string Type::to_string() const
{
    std::string result;

    // Add base type
    switch (base_type)
    {
    case BaseType::INT:
        result = "int";
        break;
    case BaseType::LONG:
        result = "long";
        break;
    case BaseType::UINT:
        result = "unsigned int";
        break;
    case BaseType::ULONG:
        result = "unsigned long";
        break;
    case BaseType::DOUBLE:
        result = "double";
        break;
    case BaseType::VOID:
        result = "void";
        break;
    case BaseType::CHAR:
        result = "char";
        break;
    case BaseType::BOOL:
        result = "bool";
        break;
    case BaseType::STRUCT:
        result = "struct " + (struct_name.has_value() ? struct_name.value() : "unknown");
        break;
    case BaseType::NULL_TYPE:
        result = "null";
        break;
    }

    for (int i = 0; i < ptr_level; i++)
        result += "*";

    for (int size : array_sizes)
        result += "[" + std::to_string(size) + "]";

    return result;
}

void Type::print()
{
    std::cout << to_string() << std::endl;
}

bool Type::is_signed() const
{
    if (is_pointer() || is_array())
        return false;

    if (base_type == BaseType::INT || base_type == BaseType::LONG)
        return true;

    return false;
}

bool Type::operator==(const Type &other) const
{
    if (base_type != other.base_type)
        return false;

    if (ptr_level != other.ptr_level)
        return false;

    if (array_sizes != other.array_sizes)
        return false;

    if (base_type == BaseType::STRUCT)
    {
        if (!struct_name.has_value() || !other.struct_name.has_value())
            return false;
        return struct_name.value() == other.struct_name.value();
    }

    return true;
}

bool Type::operator!=(const Type &other) const
{
    return !(*this == other);
}

bool Type::can_assign_from(const Type &other) const
{
    if (*this == other)
        return true;

    // Allow for Array to pointer decay (for strings)
    if (is_pointer() && other.is_array())
    {
        return base_type == other.base_type;
    }

    if (is_pointer())
    {
        if (other.is_pointer())
        {
            // Allow void* to be assigned to any pointer
            if (other.base_type == BaseType::VOID)
                return true;
        }
        // Allow null (integer 0) to be assigned to pointers
        if (other.base_type == BaseType::INT && !other.is_pointer())
            return true;
        return false;
    }

    if (!is_pointer() && !other.is_pointer())
    {
        // Allow smaller integers to larger ones
        if (base_type == BaseType::LONG && other.base_type == BaseType::INT)
            return true;

        // Allow integers to double
        if (base_type == BaseType::DOUBLE &&
            (other.base_type == BaseType::INT || other.base_type == BaseType::LONG))
            return true;
    }
}

bool Type::can_convert_to(const Type &other) const
{
    if (can_assign_from(other))
        return true;

    if (!is_pointer() && !other.is_pointer() && !is_array() && !other.is_array())
    {
        if ((base_type != BaseType::VOID && base_type != BaseType::STRUCT) &&
            (other.base_type != BaseType::VOID && other.base_type != BaseType::STRUCT))
            return true;
    }

    return false;
}

bool Type::has_base_type(const BaseType &other) const
{
    return base_type == other;
}

bool Type::is_integral() const
{
    return has_base_type(BaseType::INT) ||
           has_base_type(BaseType::UINT) ||
           has_base_type(BaseType::LONG) ||
           has_base_type(BaseType::ULONG) ||
           has_base_type(BaseType::CHAR) ||
           has_base_type(BaseType::BOOL);
}

void Type::add_field(const std::string &name, const Type &type)
{
    int current_offset = 0;

    if (!struct_fields.empty())
    {
        const auto &[last_name, last_pair] = struct_fields.back();
        const Type &last_type = last_pair.first;
        int last_offset = last_pair.second;
        current_offset = last_offset + last_type.get_size();
    }

    // Align to 8 bytes max
    size_t alignment = std::min<size_t>(8, std::max<size_t>(1, type.get_size()));
    current_offset = (current_offset + alignment - 1) & ~(alignment - 1);

    struct_fields.push_back({name, {type, current_offset}});
}

int Type::get_field_offset(const std::string &field_name) const
{
    if (!is_struct())
        throw std::runtime_error("Type Error: Attempting to get field offset from non-struct type");

    for (const auto &field : struct_fields)
    {
        if (field.first == field_name)
            return field.second.second;
    }

    throw std::runtime_error("Type Error: Field '" + field_name + "' not found in struct");
}

std::string Type::get_field_name(int index) const
{
    if (!is_struct() || index < 0 || index >= static_cast<int>(struct_fields.size()))
        throw std::runtime_error("Type Error: Invalid field index " + std::to_string(index));

    return struct_fields[index].first;
}