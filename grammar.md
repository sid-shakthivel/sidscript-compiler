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

need to recognise tilda, negation, decrement

program = Program(function_definition)
func_decl = Function(identifier name, element\* body)
element = Decl(decl) | Stmt(stmt)
decl = VarDecl(identifier name, expr? value)
stmt = Return(exp)
expr = Literal(int) | Unary(un_opr, expr) | Binary(bin_apr, exp, exp) | Var(identifier) | VarAssign(expr, expr)
un_opr = Complement | Negate | Not
bin_opr = Add | Subtract | Multiply | Divide | Remainder | And | Or
| Equal | NotEqual | LessThan | LessOrEqual
| GreaterThan | GreaterOrEqual

### Current Grammar

<program> ::= <func>
<func> ::= "fn" <identifier> "(" ")" "{" { <element> } "}"
<element> ::= <stmt> | <decl>
<decl> ::= "int" <identifier> [ "=" <expr> ] ";"
<stmt> ::= "return" <expr> ";"
<expr> ::= <factor> | <expr> <binopr> <expr>
<factor> ::= <int> | <identifier> | <unopr> <factor> | "(" <expr> ")"
<binopr> ::= ::= "-" | "+" | "\*" | "/" | "%" | "&&" | "||"
| "==" | "!=" | "<" | "<=" | ">" | ">="
<unopr> ::= "-" | "~" | "!"
<identifier> ::= ? An identifier token ?
<int> ::= ? A constant token ?

A declaration introduces/defines entities (variables, functions, types)
A statement is an action which is executed (assignment, if, while, for, return)
