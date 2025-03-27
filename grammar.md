### Current Grammar

<program> ::= { <func_decl> | <var_decl> }
<block> ::= "{" { <element> } "}"

<element> ::= <stmt> | <decl>

<decl> ::= <var_decl> | <var_assign> | <func_decl> | <struct_decl>
<var_decl> ::= <specifier>? { <type_specifier> }+ <identifier> [ "=" <expr> ] ";"
<func_decl> ::= <specifier>? "fn" <identifier> "(" <param-list> ")" "->" (<type_specifier> | "void") <block>
<struct_decl ::= "struct" <identifier> [ "{" { <member-declaration> }+ "}" ] ";"
<member_decl> ::= { <type-specifier> }+ <declarator> ";"

<var_assign> ::= <identifer> "=" <expr> ";"

<stmt> ::= <rtn_stmt> | <if_stmt> | <while_stmt> | <for_stmt> | <loop_control>

<rtn_stmt> ::= "return" <expr> ";"
<if_stmt> ::= "if" "(" <expr> ")" <block> [ "else" <block> ]

<while_stmt> ::= "while" "(" <expr> ")" <block>
<for_stmt> ::= "for" "(" <for_init> <expr> ";" <expr> ")" <block>
<for_init> ::= <var_decl> | <var_assign>
<loop_control> ::= ("continue" | "break") ";"

<param_list> ::= "" | <type_specifier> <identifier> { "," <type_specifier> <identifier> }
<argument_list> ::= <exp> { "," <exp> }

<expr> ::= <factor> | <expr> <binopr> <expr> | <factor> <postopr>
<factor> ::= <literal>
| <identifier> [ "[" <expr> "]" ]\*
| "(" { <type_specifier> }+ ")" <factor>
| <unopr> <factor>
| "(" <expr> ")"
| <identifier> "(" [ <argument-list> ] ")"

<type_specifier> ::= { "\*" } "int" | "long" | "unsigned" | "signed" | "double" | "char" [ "[" <expr> "]" ] | "struct" <identifier> | "bool"
<specifier> ::= "static" | "extern"

<binopr> ::= ::= "-" | "+" | "\*" | "/" | "%" | "&&" | "||"
| "==" | "!=" | "<" | "<=" | ">" | ">="
<unopr> ::= "-" | "~" | "++" | "--" | "&" | "\*"
<postopr> ::= "++" | "--" | "." <identifier> | "->" <identifier>

<literal> ::= <int> | <long> | <uint> | <ulong> | <double> | <char> | <string> | <bool>

<identifier> ::= ? An identifier token ?
<int> ::= ? A int (4 bytes) token ?
<long> ::= ? An long (8 bytes) token ?
<uint> ::= ? An unsigned int (4 bytes) token ?
<ulong> ::= ? An unsigned long (8 bytes) token ?
<double> ::= ? A floating-point constant token ?
<char> ::= "'" ? any single character ? "'"
<bool> ::= "true" | "false"
<string> ::= '"' ? any sequence of characters ? '"'

A declaration introduces/defines entities (variables, functions, types)
A statement is an action which is executed (assignment, if, while, for, return)

A signed int/long is the same as regular long/int

Note that <var_assign> is a rule in itself to only allow a variable to be assigned once hence operations like "a = b = c" are not allowed
There are no `long int` rather just `long` for simplification
