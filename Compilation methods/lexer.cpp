#include "lexer.h"
#include "error.h"
#include <cctype>
#include <sstream>

Lexer::Lexer(const std::string& input) : input(input), pos(0), line(1), line_pos(1) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (pos < input.size()) {
        char c = input[pos];

        // Пропускаем пробелы и табуляцию
        if (std::isspace(c)) {
            if (c == '\n') {
                line++;
                line_pos = 1;
            }
            else {
                line_pos++;
            }
            pos++;
            continue;
        }

        // Обработка комментариев
        if (c == '/' && pos + 1 < input.size() && input[pos + 1] == '/') {
            pos += 2;
            line_pos += 2;
            // Пропускаем все символы до конца строки
            while (pos < input.size() && input[pos] != '\n') {
                pos++;
                line_pos++;
            }
            continue;
        }

        // Ключевые слова и идентификаторы
        if (std::isalpha(c) || c == '_') {
            std::string word;
            int start_pos = line_pos;
            while (pos < input.size() && (std::isalnum(input[pos]) || input[pos] == '_')) {
                word += input[pos];
                pos++;
                line_pos++;
            }

            // Проверка ключевых слов
            if (word == "int" || word == "if" || word == "else" ||
                word == "while" || word == "read" || word == "print") {
                tokens.emplace_back("KEYWORD", word, line, start_pos);
            }
            else {
                tokens.emplace_back("ID", word, line, start_pos);
            }
            continue;
        }

        // Числа
        if (std::isdigit(c)) {
            std::string number;
            int start_pos = line_pos;
            while (pos < input.size() && std::isdigit(input[pos])) {
                number += input[pos];
                pos++;
                line_pos++;
            }
            tokens.emplace_back("NUMBER", number, line, start_pos);
            continue;
        }

        // Символы
        if (c == '+' || c == '-' || c == '*' || c == '/' ||
            c == '=' || c == '>' || c == '<' || c == '|' ||
            c == '&' || c == '!' || c == '~' || c == '(' ||
            c == ')' || c == '{' || c == '}' || c == '[' ||
            c == ']' || c == ';' || c == ',') {
            std::string symbol(1, c);
            tokens.emplace_back("SYMBOL", symbol, line, line_pos);
            pos++;
            line_pos++;
            // Проверка на ==
            if (c == '=' && pos < input.size() && input[pos] == '=') {
                tokens.back().value = "==";
                pos++;
                line_pos++;
            }
            continue;
        }

        // Неизвестный символ
        std::stringstream ss;
        ss << "invalid symbol '" << c << "'";
        throw Error("Lexical error", ss.str(), line, line_pos);
    }

    tokens.emplace_back("EOF", "", line, line_pos);
    return tokens;
}