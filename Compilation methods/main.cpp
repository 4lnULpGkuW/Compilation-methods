#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector> // Добавлено для std::vector
#include <string> // Добавлено для std::string

// Глобальный флаг для тихого режима
bool silent_mode = false; // Установите true для тихого режима

std::string read_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void run_test(const std::string& filename) {
    if (!silent_mode) {
        std::cout << "=== Running test: " << filename << " ===\n";
        std::string code_content = read_file(filename); // Читаем содержимое файла для вывода
        std::cout << "Code:\n" << code_content << std::endl;
    }


    std::string code = read_file(filename); // Для лексера

    try {
        Lexer lexer(code);
        std::vector<Token> tokens = lexer.tokenize();
        if (!silent_mode) {
            std::cout << "Tokens generated successfully (" << tokens.size() << " tokens)\n";
        }

        SymbolTable sym_table;
        Parser parser(sym_table);
        parser.set_silent_mode(silent_mode); // Передаем флаг в парсер

        std::vector<OPS> ops_list = parser.parse(tokens); // Переименовал ops в ops_list
        if (!silent_mode) {
            std::cout << "OPS generated successfully (" << ops_list.size() << " operations):\n";
            for (size_t i = 0; i < ops_list.size(); ++i) {
                std::cout << i << ": " << ops_list[i].operation << (ops_list[i].operand.empty() ? "" : " " + ops_list[i].operand) << "\n";
            }
        }

        Interpreter interpreter(sym_table);
        interpreter.set_silent_mode(silent_mode); // Передаем флаг в интерпретатор

        if (!silent_mode) { // Приглашение к вводу показываем всегда, когда оно нужно
            // Но в данном случае, если silent_mode, пользователь не будет знать, что от него ждут ввода.
            // Это нужно обдумать. Пока что, если silent, то и этого не будет.
            // Либо, если есть операции 'r', то всегда выводить "Please provide input..."
            bool needs_input = false;
            for (const auto& op_item : ops_list) { // Переименовал op в op_item
                if (op_item.operation == "r" || op_item.operation == "array_read") {
                    needs_input = true;
                    break;
                }
            }
            if (needs_input) {
                std::cout << "Please provide input for 'read' operations (if any for " << filename << "): ";
            }
        }
        interpreter.execute(ops_list);

        std::cout << "Test " << filename << " completed successfully\n";
    }
    catch (const Error& e) {
        std::cerr << "Error in " << filename << ": " << e.message() << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Standard Exception in " << filename << ": " << e.what() << "\n";
    }
    if (!silent_mode) {
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) { // Добавляем argc и argv для аргументов командной строки
    if (argc > 1 && std::string(argv[1]) == "--silent") {
        silent_mode = true;
    }
    silent_mode = true;
    std::vector<std::string> test_files = { "test1.txt", "test2.txt", "test3.txt", "test4.txt", "test5.txt" };
    for (const auto& file : test_files) {
        run_test(file);
    }
    if (!silent_mode) {
        std::cout << "=== All tests completed ===\n";
    }
    else {
        std::cout << "All tests processed (silent mode).\n"; // Краткое сообщение в тихом режиме
    }
    return 0;
}