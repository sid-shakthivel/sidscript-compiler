### Current ASDL

need to recognise tilda, negation, decrement

program = Program(function_definition)
func_decl = Function(identifier name, element\* body)
element = Decl(decl) | Stmt(stmt)
decl = VarDecl(identifier name, expr? value) | VarAssign(identifier, expr)
stmt = Return(exp) | If(expr condition, element then, element? else)
expr = Literal(int) | Unary(un_opr, expr) | Binary(bin_apr, exp, exp) | Var(identifier)
un_opr = Complement | Negate | Not
bin_opr = Add | Subtract | Multiply | Divide | Remainder | And | Or
| Equal | NotEqual | LessThan | LessOrEqual
| GreaterThan | GreaterOrEqual

### Current Grammar

<program> ::= <func>
<func> ::= "fn" <identifier> "(" ")" "->" "int" <block>
<block> ::= "{" { <element> } "}"
<element> ::= <stmt> | <decl>
<decl> ::= <var_decl> | <var_assign>
<var_decl> ::= "int" <identifier> [ "=" <expr> ] ";"
<var_assign> ::= <identifer> "=" <expr> ";"
<stmt> ::= <rtn_stmt> | <if_stmt>
<rtn_stmt> ::= "return" <expr> ";"
<if_stmt> ::= "if" "(" <expr> ")" <block> [ "else" <block> ]
<expr> ::= <factor> | <expr> <binopr> <expr>
<factor> ::= <int> | <identifier> | <unopr> <factor> | "(" <expr> ")"
<binopr> ::= ::= "-" | "+" | "\*" | "/" | "%" | "&&" | "||"
| "==" | "!=" | "<" | "<=" | ">" | ">="
<unopr> ::= "-" | "~" | "!"
<identifier> ::= ? An identifier token ?
<int> ::= ? A constant token ?

A declaration introduces/defines entities (variables, functions, types)
A statement is an action which is executed (assignment, if, while, for, return)

TACInstruction t0("GT", "x", "y", "t0"); // t0 = x > y
TACInstruction t1("IF", "t0", "", "L1"); // IF t0 GOTO L1
TACInstruction t2("GOTO", "", "", "L2"); // GOTO L2
TACInstruction t3("LABEL", "L1"); // L1:
TACInstruction t4("ADD", "x", "y", "t1"); // t1 = x + y
TACInstruction t5("ASSIGN", "t1", "", "z"); // z = t1
TACInstruction t6("LABEL", "L2"); // L2:
