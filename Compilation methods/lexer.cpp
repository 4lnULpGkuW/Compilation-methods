#include "lexer.h"
#include "error.h"
#include <cctype>
#include <sstream>
#include <iostream>

Lexer::Lexer(const std::string& input, bool silent) : input(input), pos(0), line(1), line_pos(1), silent_mode(silent) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    if (!silent_mode) {
        std::cout << "Starting tokenization\n";
    }

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
                if (!silent_mode) {
                    std::cout << "Token: KEYWORD " << word << " at line " << line << ", pos " << start_pos << "\n";
                }
            }
            else {
                tokens.emplace_back("ID", word, line, start_pos);
                if (!silent_mode) {
                    std::cout << "Token: ID " << word << " at line " << line << ", pos " << start_pos << "\n";
                }
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
            if (!silent_mode) {
                std::cout << "Token: NUMBER " << number << " at line " << line << ", pos " << start_pos << "\n";
            }
            continue;
        }

        // Символы
        if (c == '+' || c == '-' || c == '*' || c == '/' ||
            c == '=' || c == '>' || c == '<' || c == '|' ||
            c == '&' || c == '!' || c == '~' || c == '(' ||
            c == ')' || c == '{' || c == '}' || c == '[' ||
            c == ']' || c == ';' || c == ',') {
            std::string symbol(1, c);
            int start_pos = line_pos;
            pos++;
            line_pos++;
            // Проверка на ==
            if (c == '=' && pos < input.size() && input[pos] == '=') {
                symbol = "==";
                pos++;
                line_pos++;
            }
            tokens.emplace_back("SYMBOL", symbol, line, start_pos);
            if (!silent_mode) {
                std::cout << "Token: SYMBOL " << symbol << " at line " << line << ", pos " << start_pos << "\n";
            }
            continue;
        }

        // Неизвестный символ
        std::stringstream ss;
        ss << "invalid symbol '" << c << "'";
        throw Error("Lexical error", ss.str(), line, line_pos);
    }

    tokens.emplace_back("EOF", "", line, line_pos);
    if (!silent_mode) {
        std::cout << "Token: EOF at line " << line << ", pos " << line_pos << "\n";
        std::cout << "Tokenization completed with " << tokens.size() << " tokens\n";
    }
    return tokens;
}