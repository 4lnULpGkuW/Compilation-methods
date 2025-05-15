#ifndef TOKEN_H
#define TOKEN_H

#include <string>

struct Token {
    std::string type;  // KEYWORD, ID, NUMBER, SYMBOL, EOF
    std::string value; // Значение токена (например, "int", "a", "42", "+")
    int line;          // Номер строки
    int pos;           // Позиция в строке
    Token(const std::string& t, const std::string& v, int l, int p)
        : type(t), value(v), line(l), pos(p) {}
};

#endif