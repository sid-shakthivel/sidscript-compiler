### Grammar

```
<program> ::= (<statement>)*
<statement> ::= <var_decl> | <var_assign> | <if_stmt> | <while_stmt> | <for_stmt>

<var_decl> ::= <basic_type> <id> "=" <expr> ";"
<var_assign> ::= <id> "=" <expr> ";"

<if_stmt> ::= "if" "(" <conditional> ")" "{" <statement>+ "}" | "if" "(" <conditional> ")" "{" <statement>+ "}" "else" "{" <statement>+ "}"
<conditional> ::= <expr> ("==" | "!=" | ">" | ">=" | "<" | "<=") <expr>

<while_stmt> ::= "while" "(" <conditional> ")" "{" <statement>+ "}">
<for_stmt> ::= "for" "(" (<var_assign> | <var_decl>) ";" <conditional> ";" <var_assign> ")" "{" <statement>+ "}">

<func_decl> ::= "fn" <id> "(" <param_list>? ")" ("->" <basic_type>)? "{" <statement>+ "}"
<param_list> ::= <param> ("," <param>)*
<param> ::= <basic_type> <id>
<func_call> ::= <id> "(" <arg_list>? ")" 

<expr> ::= <expr> ("+" | "-") <term> | <term>
<term> ::= <term> ("*" | "/") <factor> | <factor>
<factor> ::= <un_opr> <factor> | (<int> | <float> | <bool> | <func_call>)
<un_opr> ::= "-" | "~" | "!"
<basic_type> ::= "int" | "float" | "bool"
```