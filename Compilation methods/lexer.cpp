#include "lexer.h"
#include "error.h"
#include <cctype>
#include <sstream>
#include <iostream> 

Lexer::Lexer(const std::string& input)
    : input(input), pos(0), line(1), line_pos(1), silent_mode_active(false) {
    keywords = { "int", "if", "else", "while", "read", "print" };
    init_transition_table();
}

void Lexer::set_silent_mode(bool mode) {
    silent_mode_active = mode;
}

CharCategory Lexer::get_char_category(char c) const {
    if (std::isalpha(c) || c == '_') return CharCategory::LETTER;
    if (std::isdigit(c)) return CharCategory::DIGIT;
    switch (c) {
    case '/': return CharCategory::SLASH;
    case '*': return CharCategory::STAR;
    case '+': return CharCategory::PLUS;
    case '-': return CharCategory::MINUS;
    case '~': return CharCategory::TILDE;
    case '<': return CharCategory::LT;
    case '>': return CharCategory::GT;
    case '=': return CharCategory::EQ_SYMBOL;
    case '|': return CharCategory::PIPE;
    case '&': return CharCategory::AMP;
    case '!': return CharCategory::EXCL;
    case ';': return CharCategory::SEMICOLON;
    case '(': return CharCategory::LPAREN;
    case ')': return CharCategory::RPAREN;
    case '[': return CharCategory::LBRACKET;
    case ']': return CharCategory::RBRACKET;
    case '{': return CharCategory::LBRACE;
    case '}': return CharCategory::RBRACE;
    case ',': return CharCategory::COMMA;
    case ' ': case '\t': case '\r': return CharCategory::SPACE;
    case '\n': return CharCategory::NEWLINE;
    case '\0': return CharCategory::END_OF_FILE;
    default: return CharCategory::OTHER;
    }
}

std::string Lexer::get_token_type_from_action(int action_code, const std::string& lexeme) {
    switch (action_code) {
    case 0: case 26: case 27: // ID related actions
        return keywords.count(lexeme) ? "KEYWORD" : "ID";
    case 1: case 28: case 29: // NUMBER related actions
        return "NUMBER";
    case 2: case 4: case 5: case 6: case 7: case 8: case 9:
    case 10: case 11: case 12: case 13: case 14: case 15: case 16:
    case 17: case 18: case 19: case 20: case 21:
        return "SYMBOL"; // All single char symbols + ==
    case 25: return "EOF";
    default:
        return "UNKNOWN";
    }
}

void Lexer::init_transition_table() {
    using CC = CharCategory;
    using LS = LexState;

    auto add = [&](LS from, CC cat, LS to, int action) {
        transition_table[{from, cat}] = { to, action };
    };

    // S - START state
    add(LS::S, CC::LETTER, LS::A, 0);  // A/0
    add(LS::S, CC::DIGIT, LS::B, 1);  // B/1
    add(LS::S, CC::SLASH, LS::D, -1); // D (no action yet)
    add(LS::S, CC::STAR, LS::E, 5);  // E/5
    add(LS::S, CC::PLUS, LS::E, 6);  // E/6
    add(LS::S, CC::MINUS, LS::E, 7);  // E/7
    add(LS::S, CC::TILDE, LS::E, 8);  // E/8
    add(LS::S, CC::LT, LS::E, 9);  // E/9
    add(LS::S, CC::GT, LS::E, 10); // E/10
    add(LS::S, CC::EQ_SYMBOL, LS::C, -1); // C (no action yet)
    add(LS::S, CC::PIPE, LS::E, 19); // E/19
    add(LS::S, CC::AMP, LS::E, 20); // E/20
    add(LS::S, CC::EXCL, LS::E, 21); // E/21
    add(LS::S, CC::SEMICOLON, LS::E, 12); // E/12
    add(LS::S, CC::LPAREN, LS::E, 13); // E/13
    add(LS::S, CC::RPAREN, LS::E, 14); // E/14
    add(LS::S, CC::LBRACKET, LS::E, 15); // E/15
    add(LS::S, CC::RBRACKET, LS::E, 16); // E/16
    add(LS::S, CC::LBRACE, LS::E, 17); // E/17
    add(LS::S, CC::RBRACE, LS::E, 18); // E/18
    add(LS::S, CC::COMMA, LS::E, 19); // E/19 (fixed for comma)
    add(LS::S, CC::SPACE, LS::S, 23); // S/23 (Skip space)
    add(LS::S, CC::NEWLINE, LS::S, 22); // S/22 (Skip newline, count line)
    add(LS::S, CC::OTHER, LS::M, 24); // M/24 (Error)
    add(LS::S, CC::END_OF_FILE, LS::E, 25); // E/25 (EOF)

    // A - IDentifier state
    add(LS::A, CC::LETTER, LS::A, 26); // A/26
    add(LS::A, CC::DIGIT, LS::A, 26); // A/26
    for (CC cat : {CC::SLASH, CC::STAR, CC::PLUS, CC::MINUS, CC::TILDE, CC::LT, CC::GT, CC::EQ_SYMBOL, CC::PIPE, CC::AMP, CC::EXCL, CC::SEMICOLON, CC::LPAREN, CC::RPAREN, CC::LBRACKET, CC::RBRACKET, CC::LBRACE, CC::RBRACE, CC::COMMA, CC::SPACE, CC::NEWLINE, CC::END_OF_FILE}) {
        add(LS::A, cat, LS::E, 27); // E/27
    }
    add(LS::A, CC::OTHER, LS::M, 24); // M/24

    // B - NUMber state
    add(LS::B, CC::DIGIT, LS::B, 28); // B/28
    add(LS::B, CC::LETTER, LS::M, 24); // M/24
    for (CC cat : {CC::SLASH, CC::STAR, CC::PLUS, CC::MINUS, CC::TILDE, CC::LT, CC::GT, CC::EQ_SYMBOL, CC::PIPE, CC::AMP, CC::EXCL, CC::SEMICOLON, CC::LPAREN, CC::RPAREN, CC::LBRACKET, CC::RBRACKET, CC::LBRACE, CC::RBRACE, CC::COMMA, CC::SPACE, CC::NEWLINE, CC::END_OF_FILE}) {
        add(LS::B, cat, LS::E, 29); // E/29
    }
    add(LS::B, CC::OTHER, LS::M, 24); // M/24

    // C - EQ_START state (after first '=')
    add(LS::C, CC::EQ_SYMBOL, LS::E, 11); // E/11 (for ==)
    for (CC cat : {CC::LETTER, CC::DIGIT, CC::SLASH, CC::STAR, CC::PLUS, CC::MINUS, CC::TILDE, CC::LT, CC::GT, CC::PIPE, CC::AMP, CC::EXCL, CC::SEMICOLON, CC::LPAREN, CC::RPAREN, CC::LBRACKET, CC::RBRACKET, CC::LBRACE, CC::RBRACE, CC::COMMA, CC::SPACE, CC::NEWLINE, CC::OTHER, CC::END_OF_FILE}) {
        add(LS::C, cat, LS::E, 2);  // E/2 (for single =)
    }

    // D - SLASH state (after first '/')
    add(LS::D, CC::SLASH, LS::F, -1); // F (no action, start comment)
    for (CC cat : {CC::LETTER, CC::DIGIT, CC::STAR, CC::PLUS, CC::MINUS, CC::TILDE, CC::LT, CC::GT, CC::EQ_SYMBOL, CC::PIPE, CC::AMP, CC::EXCL, CC::SEMICOLON, CC::LPAREN, CC::RPAREN, CC::LBRACKET, CC::RBRACKET, CC::LBRACE, CC::RBRACE, CC::COMMA, CC::SPACE, CC::NEWLINE, CC::OTHER, CC::END_OF_FILE}) {
        add(LS::D, cat, LS::E, 4);  // E/4 (for / operator)
    }

    // F - COMMENT state
    for (CC cat : {CC::LETTER, CC::DIGIT, CC::SLASH, CC::STAR, CC::PLUS, CC::MINUS, CC::TILDE, CC::LT, CC::GT, CC::EQ_SYMBOL, CC::PIPE, CC::AMP, CC::EXCL, CC::SEMICOLON, CC::LPAREN, CC::RPAREN, CC::LBRACKET, CC::RBRACKET, CC::LBRACE, CC::RBRACE, CC::COMMA, CC::SPACE, CC::OTHER}) {
        add(LS::F, cat, LS::F, 30); // F/30
    }
    add(LS::F, CC::NEWLINE, LS::S, 22); // S/22 (End comment, return to S)
    add(LS::F, CC::END_OF_FILE, LS::E, 25); // E/25 (End comment at EOF)
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> token_list;
    LexState current_lex_state = LexState::S;
    std::string current_lexeme_value = "";
    int lexeme_start_line_num = line;
    int lexeme_start_char_pos = line_pos;

    while (pos <= input.length()) {
        char current_char = (pos < input.length()) ? input[pos] : '\0'; // EOF char
        CharCategory char_cat = get_char_category(current_char);

        TransitionResult transition_res = { LexState::M, 24 };
        auto transition_map_iter = transition_table.find({ current_lex_state, char_cat });
        if (transition_map_iter != transition_table.end()) {
            transition_res = transition_map_iter->second;
        }

        LexState next_lex_state = transition_res.next_state;
        int action_code = transition_res.action_code;

        // Skip whitespace/newline from S
        if (current_lex_state == LexState::S && (action_code == 22 || action_code == 23)) {
            if (action_code == 22) { // Newline
                line++;
                line_pos = 1;
            }
            else { // Space
                line_pos++;
            }
            pos++;
            current_lex_state = LexState::S;
            current_lexeme_value = "";
            lexeme_start_line_num = line;
            lexeme_start_char_pos = line_pos;
            continue;
        }

        // Start of comment
        if (current_lex_state == LexState::D && next_lex_state == LexState::F) {
            pos++; // Consume the second '/'
            line_pos++;
            current_lex_state = LexState::F;
            current_lexeme_value = "";
            lexeme_start_line_num = line;
            lexeme_start_char_pos = line_pos;
            continue;
        }

        // Inside a comment
        if (current_lex_state == LexState::F) {
            if (action_code == 22 || action_code == 25) { // End of comment (newline or EOF)
                if (action_code == 22) {
                    line++;
                    line_pos = 1;
                }
                pos++;
                current_lex_state = LexState::S;
                current_lexeme_value = "";
                lexeme_start_line_num = line;
                lexeme_start_char_pos = line_pos;
                if (current_char == '\0') break;
                continue;
            }
            else {
                pos++;
                line_pos++;
                continue;
            }
        }

        // Token completion
        if (next_lex_state == LexState::E) {
            std::string token_val_to_add = current_lexeme_value;
            std::string token_type_str;
            int final_action_code = action_code;
            bool is_single_char_token = (current_lex_state == LexState::S);

            // Handle single-character tokens or EOF
            if (is_single_char_token) {
                token_val_to_add = std::string(1, current_char);
            }

            // Special handling for '=' and '==' or '/'
            if (current_lex_state == LexState::C) {
                if (char_cat == CharCategory::EQ_SYMBOL) {
                    token_val_to_add = "==";
                    final_action_code = 11; // Action for ==
                    pos++; // Consume second '='
                    line_pos++;
                }
                else {
                    token_val_to_add = "=";
                    final_action_code = 2; // Action for single =
                }
            }
            else if (current_lex_state == LexState::D && char_cat != CharCategory::SLASH) {
                token_val_to_add = "/";
                final_action_code = 4;
            }

            token_type_str = get_token_type_from_action(final_action_code, token_val_to_add);

            // Add token if valid
            if (token_type_str != "UNKNOWN" && !token_val_to_add.empty()) {
                token_list.emplace_back(token_type_str, token_val_to_add, lexeme_start_line_num, lexeme_start_char_pos);
            }

            // Handle EOF
            if (final_action_code == 25) {
                if (!token_list.empty() && token_list.back().type == "EOF") { /* Already added */ }
                else {
                    token_list.emplace_back("EOF", "", lexeme_start_line_num, lexeme_start_char_pos);
                }
                break;
            }

            // Reset state
            current_lexeme_value = "";
            current_lex_state = LexState::S;

            // Update position
            if (is_single_char_token || final_action_code == 25) {
                pos++;
                if (char_cat == CharCategory::NEWLINE) {
                    line++;
                    line_pos = 1;
                }
                else if (current_char != '\0') {
                    line_pos++;
                }
            }

            // Update lexeme start for next token
            lexeme_start_line_num = line;
            lexeme_start_char_pos = line_pos;
        }
        else if (next_lex_state == LexState::M) {
            std::stringstream ss_err;
            ss_err << "char '" << current_char << "'";
            if (!current_lexeme_value.empty()) ss_err << " after '" << current_lexeme_value << "'";
            throw Error("Lexical error", ss_err.str(), lexeme_start_line_num, lexeme_start_char_pos);
        }
        else { // Continue accumulating
            if (current_lex_state == LexState::S) {
                lexeme_start_line_num = line;
                lexeme_start_char_pos = line_pos;
            }
            current_lexeme_value += current_char;
            current_lex_state = next_lex_state;
            pos++;
            if (char_cat == CharCategory::NEWLINE) {
                line++;
                line_pos = 1;
            }
            else {
                line_pos++;
            }
        }
        if (current_char == '\0') break;
    }
    return token_list;
}