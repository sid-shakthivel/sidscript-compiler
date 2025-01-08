### Final Grammar

```
<program> ::= (<statement>)*
<statement> ::= <var_decl> | <var_assign> | <if_stmt> | <while_stmt> | <for_stmt>

<var_decl> ::= <basic_type> <id> "=" <expr> ";"
<var_assign> ::= <id> "=" <expr> ";"
<rtn_stmt> ::= "return" <expr> ";"

<if_stmt> ::= "if" "(" <conditional> ")" "{" <statement>+ "}" | "if" "(" <conditional> ")" "{" <statement>+ "}" "else" "{" <statement>+ "}"
<conditional> ::= <expr> ("==" | "!=" | ">" | ">=" | "<" | "<=") <expr>

<while_stmt> ::= "while" "(" <conditional> ")" "{" <statement>+ "}">
<for_stmt> ::= "for" "(" (<var_assign> | <var_decl>) ";" <conditional> ";" <var_assign> ")" "{" <statement>+ "}">

<func_decl> ::= "fn" <id> "(" <param_list>? ")" "-"">" <basic_type>)? "{" <statement>+ "}"
<param_list> ::= <param> ("," <param>)*
<param> ::= <basic_type> <id>
<arg_list> ::= <expr> ("," <expr>)*
<func_call> ::= <id> "(" <arg_list>? ")" ";"

<expr> ::= (<term> ("+" | "-") <term>)+ | <term>
<term> ::= ( <factor> ("*" | "/") <factor>)+ | <factor>
<factor> ::= <un_opr> <factor> | (int | float | bool | <func_call>)
<un_opr> ::= "-" | "~" | "!"
<basic_type> ::= "int" | "float" | "bool"
```

### Current ASDL

program = Program(function_definition)
func_decl = Function(identifier name, statement body)
stmt = Return(exp)
expr = Literal(int)

### Current Grammar

for the function need to add back rust-y "-> int|void"

<program> ::= <func_decl>
<func_decl> ::= "fn" <identifier> "(" ")" "{" <stmt> "}"
<stmt> ::= "return" <expr> ";"
<expr> ::= <int>
<identifier> ::= ? An identifier token ?
<int> ::= ? A constant token ?