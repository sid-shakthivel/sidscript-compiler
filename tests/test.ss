struct Test {
    int a;
    double b;
    char c[3];
};

fn main() -> int
{
    struct Test example2 = {3, 2.0, {'a', 'b', 'c'}};

    return example2;
}

std::string Assembler::format_memory_ref(Symbol *sym, int offset)
{
	if (!sym)
		return "";

	if (sym->has_static_sd() || sym->is_literal8)
		return "_" + sym->name + "(%rip)";
	else
		return std::to_string(sym->stack_offset + offset) + "(%rbp)";
}