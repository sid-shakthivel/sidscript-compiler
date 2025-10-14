#include <iostream>
#include <sstream>

#include "../include/globalSymbolTable.h"

GlobalSymbolTable::GlobalSymbolTable() {}

void GlobalSymbolTable::create_new_func(const std::string &func_name, std::unique_ptr<FuncSymbol> symbol, std::shared_ptr<SymbolTable> st)
{
	// Check if a function with the same name already exists
	auto it = functions.find(func_name);
	if (it != functions.end())
		throw std::runtime_error("Semantic Error: Function '" + func_name + "' already exists");

	functions[func_name] = std::make_tuple(std::move(symbol), st, current_module);
}

void GlobalSymbolTable::enter_func_scope(const std::string &func_name)
{
	auto it = functions.find(func_name);
	if (it == functions.end())
		throw std::runtime_error("Semantic Error: Function '" + func_name + "' is not declared");
	current_func = func_name;

	enter_scope();
}

void GlobalSymbolTable::leave_func_scope() { current_func = ""; }

bool GlobalSymbolTable::is_global_scope() const { return current_func.empty(); }

std::string GlobalSymbolTable::get_current_func() const { return current_func; }

FuncSymbol *GlobalSymbolTable::get_func_symbol(const std::string &func_name)
{
	auto it = functions.find(func_name);
	if (it == functions.end())
		return nullptr;

	/*
		If the function is not just within the current module, check if it is imported
		This is done by checking all of the imports within the import table for the current module
	*/
	if (std::get<2>(it->second) != current_module)
	{
		bool function_imported = false;

		auto it = import_table.find(current_module);
		if (it != import_table.end())
		{
			for (const auto &[imported_module, symbols] : it->second)
			{
				if (std::find(symbols.begin(), symbols.end(), func_name) != symbols.end())
				{
					function_imported = true;
					break;
				}
			}
		}

		if (!function_imported)
			throw std::runtime_error("Semantic Error: Function '" + func_name + "' is not imported in module " + current_module);
	}

	return std::get<0>(it->second).get();
}

SymbolTable *GlobalSymbolTable::get_func_st(const std::string &func_name)
{
	auto it = functions.find(func_name);
	if (it == functions.end())
		return nullptr;
	return std::get<1>(it->second).get();
}

void GlobalSymbolTable::enter_scope()
{
	auto it = functions.find(current_func);
	if (it != functions.end())
		std::get<1>(it->second)->enter_scope();
}

void GlobalSymbolTable::exit_scope()
{
	auto it = functions.find(current_func);
	if (it != functions.end())
		std::get<1>(it->second)->exit_scope();
}

void GlobalSymbolTable::declare_var(VarNode *node)
{
	return is_global_scope() ? handle_global_var_decl(node) : handle_local_var_decl(node);
}

void GlobalSymbolTable::handle_global_var_decl(VarNode *node)
{
	/*
		If func_name is empty, we are declaring a global variable
		These can be static or not
		(hence just check against other global variables)
	*/

	if (current_func.empty())
	{
		auto it = global_variables.find(node->name);

		if (it != global_variables.end())
		{
			Symbol *existing_symbol = std::get<0>(it->second).get();

			// Check for linkage conflicts
			if (existing_symbol->linkage == Linkage::Internal && contains_specifier(node->specifiers, Specifier::EXTERN))
				throw std::runtime_error("Semantic Error: Variable '" + node->name + "' declared as 'extern' conflicts with a static declaration");

			if (existing_symbol->linkage == Linkage::External && contains_specifier(node->specifiers, Specifier::STATIC))
				throw std::runtime_error("Semantic Error: Variable '" + node->name + "' declared as 'static' conflicts with an extern declaration");

			return; // Redeclarations with compatible linkage are fine.
		}

		// Create a new global symbol
		std::unique_ptr<Symbol> symbol = std::make_unique<Symbol>(node->name, 0, node->type, node->specifiers);
		symbol->set_storage_duration(StorageDuration::Static);
		symbol->set_linkage(contains_specifier(node->specifiers, Specifier::STATIC) ? Linkage::Internal : Linkage::External);
		symbol->is_global = true;

		global_variables[node->name] = std::make_tuple(std::move(symbol), current_module);
		return;
	}
}

void GlobalSymbolTable::handle_local_var_decl(VarNode *node)
{
	/*
		If a var is not global, that means it must have block scope
		Hence we will check whether the storage duration and linkage match or not
	*/
	StorageDuration sd = contains_specifier(node->specifiers, Specifier::STATIC) ? StorageDuration::Static : StorageDuration::Automatic;

	auto it = global_variables.find(node->name);
	if (it != global_variables.end())
	{
		Symbol *existing_symbol = std::get<0>(it->second).get();

		if (sd == StorageDuration::Static)
			throw std::runtime_error("Semantic Error: Block-scoped static variable '" + node->name + "' conflicts with a global static variable");
	}

	// Check in function against local variables

	auto it2 = functions.find(current_func);
	if (it2 == functions.end())
		throw std::runtime_error("Semantic Error: Function '" + current_func + "' is not declared");

	/*
		In a function, the same variable name can be used inside different scopes ie

		void func()
		{
			int a;
			int b = 5;
			if (b = 5) {
				int a = 9;
			}
		}

		Hence they need unqiue names to easily identify them
	*/

	auto [has_name_changed, new_name] = std::get<1>(it2->second)->declare_var(node->name, node->type, node->specifiers);

	if (has_name_changed)
		node->name = new_name;
}

void GlobalSymbolTable::declare_temp_var(const std::string &name, const Type &type)
{
	auto it = functions.find(current_func);
	if (it == functions.end())
		throw std::runtime_error("Semantic Error: Function '" + current_func + "' is not declared");
	std::get<1>(it->second)->declare_temp_var(name, type);
}

void GlobalSymbolTable::declare_const_var(const std::string &name, const Type &type)
{
	auto it = functions.find(current_func);
	if (it == functions.end())
		throw std::runtime_error("Semantic Error: Function '" + current_func + "' is not declared");

	std::get<1>(it->second)->declare_const_var(name, type);
}

void GlobalSymbolTable::declare_str_var(const std::string &name, const Type &type)
{
	auto it = functions.find(current_func);
	if (it == functions.end())
		throw std::runtime_error("Semantic Error: Function '" + current_func + "' is not declared");

	std::get<1>(it->second)->declare_str_var(name, type);
}

std::string GlobalSymbolTable::check_var_defined(const std::string &name)
{
	auto it = functions.find(current_func);

	/*
		If we are not within a function, that means we're global
		This means we need to check against other global variables ONLY
	*/
	if (it == functions.end())
	{
		auto it = global_variables.find(name);
		if (it == global_variables.end())
			throw std::runtime_error("Semantic Error: Variable '" + name + "' is not declared");

		std::string module_of_global_var = std::get<1>(it->second);
		if (module_of_global_var != current_module)
		{
			auto it = import_table.find(current_module);

			if (it == import_table.end())
				throw std::runtime_error("Semantic Error: No imports for " + current_module + " and variable '" + name + "' is not found within the module " + current_module);

			auto it2 = it->second.find(module_of_global_var);

			if (it2 == it->second.end())
				throw std::runtime_error("Semantic Error: Variable '" + name + "' has not been imported from " + module_of_global_var);
		}

		return name;
	}

	// Check against local variables

	auto [var_exists, new_name] = std::get<1>(it->second)->check_var_defined(name);

	if (!var_exists)
	{
		auto it = global_variables.find(name);
		if (it == global_variables.end())
			throw std::runtime_error("Semantic Error: Variable '" + name + "' is not declared");

		/*
			If the variable is declared in a different module:
				- Check that it exists within the imports of the current module
		*/
		std::string module_of_global_var = std::get<1>(it->second);
		if (module_of_global_var != current_module)
		{
			auto it = import_table.find(current_module);

			if (it == import_table.end())
				throw std::runtime_error("Semantic Error: No imports for " + current_module + " and variable '" + name + "' is not found within the module " + current_module);

			auto it2 = it->second.find(module_of_global_var);

			if (it2 == it->second.end())
				throw std::runtime_error("Semantic Error: Variable '" + name + "' has not been imported from " + module_of_global_var);
		}

		return name;
	}

	return new_name;
}

bool GlobalSymbolTable::check_struct_defined(const std::string &name)
{
	auto it = struct_table.find(name);
	if (it == struct_table.end())
		return false;

	std::string module_of_struct = it->second.second;

	if (module_of_struct != current_module)
	{
		auto it = import_table.find(current_module);

		if (it == import_table.end())
			return false;

		auto it2 = it->second.find(module_of_struct);

		if (it2 == it->second.end())
			return false;
	}

	return true;
}

Symbol *GlobalSymbolTable::get_symbol(const std::string &name)
{
	auto it = functions.find(current_func);
	if (it != functions.end())
	{
		Symbol *symbol = std::get<1>(it->second)->get_symbol(name);
		if (symbol != nullptr)
			return symbol;
	}

	auto it2 = global_variables.find(name);
	if (it2 != global_variables.end())
		return std::get<0>(it2->second).get();

	return nullptr;
}

void GlobalSymbolTable::add_import(const std::string &imported_module_name, const std::vector<std::string> &imported_names)
{
	if (import_table[current_module].find(imported_module_name) != import_table[current_module].end())
		throw std::runtime_error("Semantic Error: Module '" + imported_module_name + "' is already imported");

	import_table[current_module][imported_module_name] = imported_names;
}

void GlobalSymbolTable::check_imports()
{
	/*
		For each module:
			- Retreive each symbol in each import list
			- Check that module exists properly somewhere

		Note that since local variables can't be exported, we only need to check:
			- global_variables
			- functions
	*/

	for (auto it = import_table.begin(); it != import_table.end(); ++it)
	{
		for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			for (const auto &symbol_name : it2->second)
			{
				// Check whether it's in functions
				FuncSymbol *func_symbol = get_func_symbol(symbol_name);

				if (func_symbol)
				{
					if (!func_symbol->is_public())
						throw std::runtime_error("Semantic Error: Function '" + symbol_name + "' must be marked as public to be imported");

					continue;
				}

				// Check whether it's in global_variables
				auto it3 = global_variables.find(symbol_name);
				if (it3 != global_variables.end())
				{
					Symbol *symbol = std::get<0>(it3->second).get();

					if (symbol)
					{
						if (!symbol->is_public())
							throw std::runtime_error("Semantic Error: Variable '" + symbol_name + "' must be marked as public to be imported");

						continue;
					}
				}

				/*
					If the symbol starts with 'struct' then we need to check the struct table (and check it's been declared in the right module)
					We first need to split the string by ' ' (whitespace)
					Then only if the size of the resulting vector is 2 and the first word is 'struct' do we check the struct table
					We need to check the import is from the correct file
				*/

				std::stringstream ss(symbol_name);
				std::vector<std::string> words;
				std::string word;

				while (std::getline(ss, word, ' '))
				{
					if (!word.empty())
						words.push_back(word);
				}

				if (words.size() == 2 && words[0] == "struct")
				{
					std::string struct_name = words[1];

					auto it4 = struct_table.find(struct_name);
					if (it4 != struct_table.end())
					{
						std::string module_of_struct = it4->second.second;
						if (module_of_struct != it2->first)
							throw std::runtime_error("Semantic Error: Struct '" + struct_name + "' is not declared in module " + it->first);
						continue;
					}
				}

				throw std::runtime_error("Semantic Error: Symbol '" + symbol_name + "' is not declared");
			}
		}
	}
}

void GlobalSymbolTable::print()
{
	for (auto it = global_variables.begin(); it != global_variables.end(); ++it)
		std::cout << it->first << std::endl;

	for (auto it = functions.begin(); it != functions.end(); ++it)
	{
		// FuncSymbol *func_symbol = std::get<0>(it->second).get();
		std::cout << "Variables for *" << it->first << "* are: " << std::endl;
		std::get<1>(it->second)->print();
	}
}
