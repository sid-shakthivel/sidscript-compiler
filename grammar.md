### Current Grammar

<program> ::= { <func_decl> }
<block> ::= "{" { <element> } "}"

<element> ::= <stmt> | <decl>

<decl> ::= <var_decl> | <var_assign> | <func_decl>
<var_decl> ::= "int" <identifier> [ "=" <expr> ] ";"
<func_decl> ::= "fn" <identifier> "(" <param-list> ")" "->" ("int" | "void") <block>

<var_assign> ::= <identifer> "=" <expr> ";"

<stmt> ::= <rtn_stmt> | <if_stmt> | <while_stmt> | <for_stmt> | <loop_control>

<rtn_stmt> ::= "return" <expr> ";"
<if_stmt> ::= "if" "(" <expr> ")" <block> [ "else" <block> ]

<while_stmt> ::= "while" "(" <expr> ")" <block>
<for_stmt> ::= "for" "(" <for_init> <expr> ";" <expr> ")" <block>
<for_init> ::= <var_decl> | <var_assign>
<loop_control> ::= ("continue" | "break") ";"

<param_list> ::= "" | "int" <identifier> { "," "int" <identifier> }
<argument_list> ::= <exp> { "," <exp> }

<expr> ::= <factor> | <expr> <binopr> <expr>
<factor> ::= <int> | <identifier> | <unopr> <factor> | "(" <expr> ")" | <identifier> "(" [ <argument-list> ] ")"

<binopr> ::= ::= "-" | "+" | "\*" | "/" | "%" | "&&" | "||"
| "==" | "!=" | "<" | "<=" | ">" | ">="
<unopr> ::= "-" | "~" | "++" | "--"

<identifier> ::= ? An identifier token ?
<int> ::= ? A constant token ?

A declaration introduces/defines entities (variables, functions, types)
A statement is an action which is executed (assignment, if, while, for, return)

Note that <var_assign> is a rule in itself to only allow a variable to be assigned once hence operations like "a = b = c" are not allowed
