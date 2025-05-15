#ifndef ERROR_H
#define ERROR_H

#include <string>

class Error {
public:
    Error(const std::string& type, const std::string& symbol, int line, int pos);
    std::string message() const;

private:
    std::string type;   // Тип ошибки: lexical, syntax, runtime
    std::string symbol; // Ошибочный символ или токен
    int line;           // Номер строки
    int pos;            // Позиция
};

#endif