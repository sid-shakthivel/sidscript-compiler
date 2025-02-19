#pragma once

#include <vector>
#include <unordered_map>
#include <memory>

#include "ast.h"
#include "lexer.h"

class Parser
{
public:
    Parser(Lexer *l);
    std::shared_ptr<ProgramNode> parse();

private:
    Lexer *lexer;
    Token current_token;
    Token previous_token;

    Type curr_decl_type = Type(BaseType::INT);

    std::vector<TokenType> all_types = {TOKEN_VOID, TOKEN_INT, TOKEN_LONG, TOKEN_UNSIGNED, TOKEN_SIGNED, TOKEN_DOUBLE};
    std::vector<TokenType> addressable_types = {TOKEN_INT, TOKEN_LONG, TOKEN_UNSIGNED, TOKEN_SIGNED, TOKEN_DOUBLE, TOKEN_STAR};
    std::vector<TokenType> bin_op_tokens = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_PERCENT, TOKEN_EQUALS, TOKEN_NOT_EQUALS, TOKEN_LT, TOKEN_GT, TOKEN_LE, TOKEN_GE, TOKEN_AND, TOKEN_OR};
    std::vector<TokenType> un_op_tokens = {TOKEN_TILDA, TOKEN_MINUS, TOKEN_AMPERSAND, TOKEN_STAR, TOKEN_INCREMENT, TOKEN_DECREMENT};
    std::vector<TokenType> assign_tokens = {TOKEN_EQUALS, TOKEN_PLUS_EQUALS, TOKEN_MINUS_EQUALS, TOKEN_STAR_EQUALS, TOKEN_SLASH_EQUALS, TOKEN_MODULUS_EQUALS};

    bool match(TokenType type);
    bool match(std::vector<TokenType> &tokens);
    void advance();
    void retreat();
    void retreat_test();
    void expect(TokenType token_type);
    void expect_and_advance(TokenType token_type);
    void expect_and_advance(std::vector<TokenType> &tokens);
    void expect(std::vector<TokenType> &tokens);
    void error(const std::string &message);

    std::unique_ptr<FuncNode> parse_func_decl(TokenType specifier = TOKEN_EOF);
    std::vector<std::unique_ptr<ASTNode>> parse_block();
    void parse_param_list(std::unique_ptr<FuncNode> &func);
    std::unique_ptr<RtnNode> parse_rtn();
    std::unique_ptr<VarDeclNode> parse_var_decl(TokenType specifier = TOKEN_EOF);
    std::unique_ptr<VarAssignNode> parse_var_assign();
    std::unique_ptr<ASTNode> parse_factor();
    std::unique_ptr<ASTNode> parse_expr(int min_precedence = 0);
    std::unique_ptr<IfNode> parse_if_stmt();
    std::unique_ptr<WhileNode> parse_while_stmt();
    std::unique_ptr<ForNode> parse_for_stmt();
    std::unique_ptr<ASTNode> parse_for_init();
    std::unique_ptr<ASTNode> parse_loop_control();
    void parse_args_list(std::unique_ptr<FuncCallNode> &func_call);
    std::unique_ptr<ArrayLiteral> parse_array_initialiser();
    std::unique_ptr<ASTNode> parse_cast();

    int get_precedence(TokenType op);
    Type determine_type(std::vector<TokenType> &types);
};