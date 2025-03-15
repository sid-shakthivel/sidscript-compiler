#include <iostream>

#include "../include/globalSymbolTable.h"

GlobalSymbolTable::GlobalSymbolTable() {}

void GlobalSymbolTable::create_new_func(const std::string &func_name, std::unique_ptr<FuncSymbol> symbol, std::shared_ptr<SymbolTable> st)
{
	// Check if a function with the same name already exists
	auto it = functions.find(func_name);
	if (it != functions.end())
		throw std::runtime_error("Semantic Error: Function '" + func_name + "' already exists");

	functions[func_name] = std::make_tuple(std::move(symbol), st);
}

void GlobalSymbolTable::enter_func_scope(const std::string &func_name)
{
	auto it = functions.find(func_name);
	if (it == functions.end())
		throw std::runtime_error("Semantic Error: Function '" + func_name + "' is not declared");
	current_func = func_name;
}

void GlobalSymbolTable::leave_func_scope() { current_func = ""; }

bool GlobalSymbolTable::is_global_scope() const { return current_func.empty(); }

std::string GlobalSymbolTable::get_current_func() const { return current_func; }

FuncSymbol *GlobalSymbolTable::get_func_symbol(const std::string &func_name)
{
	auto it = functions.find(func_name);
	if (it == functions.end())
		return nullptr;
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
			Symbol *existing_symbol = it->second.get();

			// Check for linkage conflicts
			if (existing_symbol->linkage == Linkage::Internal && node->specifier == Specifier::EXTERN)
				throw std::runtime_error("Semantic Error: Variable '" + node->name + "' declared as 'extern' conflicts with a static declaration");

			if (existing_symbol->linkage == Linkage::External && node->specifier == Specifier::STATIC)
				throw std::runtime_error("Semantic Error: Variable '" + node->name + "' declared as 'static' conflicts with an extern declaration");

			return; // Redeclarations with compatible linkage are fine.
		}

		// Create a new global symbol
		std::unique_ptr<Symbol> symbol = std::make_unique<Symbol>(node->name, 0, node->type);
		symbol->set_storage_duration(StorageDuration::Static);
		symbol->set_linkage(node->specifier == Specifier::STATIC ? Linkage::Internal : Linkage::External);

		global_variables[node->name] = std::move(symbol);
		return;
	}
}

void GlobalSymbolTable::handle_local_var_decl(VarNode *node)
{
	/*
		If a var is not global, that means it must have block scope
		Hence we will check whether the storage duration and linkage match or not
	*/
	StorageDuration sd = node->specifier == Specifier::STATIC ? StorageDuration::Static : StorageDuration::Automatic;

	auto it = global_variables.find(node->name);
	if (it != global_variables.end())
	{
		Symbol *existing_symbol = it->second.get();

		if (sd == StorageDuration::Static)
			throw std::runtime_error("Semantic Error: Block-scoped static variable '" + node->name + "' conflicts with a global static variable");
	}

	// Check in function against local variables

	auto it2 = functions.find(current_func);
	if (it2 == functions.end())
		throw std::runtime_error("Semantic Error: Function '" + current_func + "' is not declared");

	auto [has_name_changed, new_name] = std::get<1>(it2->second)->declare_var(node->name, node->type, node->specifier == Specifier::STATIC);

	if (has_name_changed)
		node->name = new_name;
}

void GlobalSymbolTable::declare_temp_var(const std::string &name, Type type)
{
	auto it = functions.find(current_func);
	if (it == functions.end())
		throw std::runtime_error("Semantic Error: Function '" + current_func + "' is not declared");
	std::get<1>(it->second)->declare_temp_var(name, type);
}

void GlobalSymbolTable::declare_const_var(const std::string &name, Type type)
{
	auto it = functions.find(current_func);
	if (it == functions.end())
		throw std::runtime_error("Semantic Error: Function '" + current_func + "' is not declared");

	std::get<1>(it->second)->declare_const_var(name, type);
}

void GlobalSymbolTable::declare_str_var(const std::string &name, Type type)
{
	auto it = functions.find(current_func);
	if (it == functions.end())
		throw std::runtime_error("Semantic Error: Function '" + current_func + "' is not declared");

	std::get<1>(it->second)->declare_str_var(name, type);
}

std::string GlobalSymbolTable::check_var_defined(const std::string &name)
{
	auto it = functions.find(current_func);

	// Check against global variables
	if (it == functions.end())
	{
		auto it = global_variables.find(name);
		if (it == global_variables.end())
			throw std::runtime_error("Semantic Error: Variable '" + name + "' is not declared");

		return name;
	}

	// Check against local variables

	auto [var_exists, new_name] = std::get<1>(it->second)->check_var_defined(name);

	if (!var_exists)
	{
		auto it = global_variables.find(name);
		if (it == global_variables.end())
			throw std::runtime_error("Semantic Error: Variable '" + name + "' is not declared");

		return name;
	}

	return new_name;
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
		return it2->second.get();

	return nullptr;
}

void GlobalSymbolTable::print()
{
	for (auto it = global_variables.begin(); it != global_variables.end(); ++it)
		std::cout << it->first << std::endl;

	for (auto it = functions.begin(); it != functions.end(); ++it)
	{
		std::cout << "Variables for *" << it->first << "* are: " << std::endl;
		std::get<1>(it->second)->print();
	}
}