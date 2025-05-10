#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <string>
#include <vector>

class Lexer {
public:
    Lexer(const std::string& input, bool silent = false);
    std::vector<Token> tokenize();

private:
    std::string input;
    size_t pos;
    int line;
    int line_pos;
    bool silent_mode;
};

#endif // LEXER_H