#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "error.h" 
#include <string>
#include <vector>
#include <map>
#include <set> // Добавлено

enum class LexState {
    S, A, B, C, D, F, E, M
};

enum class CharCategory {
    LETTER, DIGIT, SLASH, STAR, PLUS, MINUS, TILDE, LT, GT, EQ_SYMBOL, // Переименовано из EQ_START для ясности
    PIPE, AMP, EXCL, SEMICOLON, LPAREN, RPAREN, LBRACKET, RBRACKET,
    LBRACE, RBRACE, COMMA, SPACE, NEWLINE, OTHER, END_OF_FILE
};

struct TransitionResult {
    LexState next_state;
    int action_code;
};

class Lexer {
public:
    Lexer(const std::string& input);
    std::vector<Token> tokenize();
    void set_silent_mode(bool mode); // Добавлено

private:
    CharCategory get_char_category(char c) const;
    void init_transition_table();
    std::string get_token_type_from_action(int action_code, const std::string& lexeme);


    std::string input;
    size_t pos;
    int line;
    int line_pos;
    bool silent_mode_active; // Добавлено

    std::map<std::pair<LexState, CharCategory>, TransitionResult> transition_table;
    std::set<std::string> keywords;
};

#endif