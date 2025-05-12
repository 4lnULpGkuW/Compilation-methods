#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <string>
#include <vector>

class Lexer {
public:
    Lexer(const std::string& input);
    std::vector<Token> tokenize();

private:
    std::string input;
    size_t pos;
    int line;
    int line_pos;
};

#endif // LEXER_H