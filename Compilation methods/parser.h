#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "symbol_table.h"
#include "ops.h"
#include <vector>
#include <string>
#include <stack>
#include <map>
#include <set>

struct Rule {
    std::string lhs;
    std::vector<std::string> rhs;
    int id;
};

class Parser {
private:
    SymbolTable& sym_table;
    std::vector<Token> tokens_list;
    std::vector<OPS> ops_list;
    std::stack<std::string> parse_stack;
    std::stack<size_t> label_stack;

    std::vector<Rule> grammar_rules;
    std::map<std::string, std::map<std::string, int>> ll_parse_table;
    std::set<std::string> declared_arrays_set;

    std::string id_for_actions;
    std::string number_for_actions;
    std::string id_for_lhs;
    std::string stored_comparison_operator;
    std::string saved_array_id;
    bool is_array_access;
    int current_initializer_count;
    size_t current_token_idx;
    bool silent_mode_active;

    void initialize_grammar_and_table();
    void execute_action(const std::string& action_symbol);
    std::string get_current_input_terminal_string(const Token& token);
    bool match_and_advance(const std::string& expected_terminal_in_rule);
    void add_ops_instruction(const std::string& operation, const std::string& operand = "");
    void push_label_ops_stack(size_t p);
    size_t pop_label_ops_stack();
    void set_jump_target(size_t ops_label_pos, size_t ops_target_address);
    bool is_variable_declared(const std::string& name) const;

public:
    Parser(SymbolTable& sym_table);
    void set_silent_mode(bool mode);
    std::vector<OPS> parse(const std::vector<Token>& tokens);
};

#endif  // PARSER_H