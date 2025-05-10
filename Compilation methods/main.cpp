#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

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

void run_test(const std::string& filename, bool silent_mode) {
    std::cout << "=== Running test: " << filename << " ===\n";
    std::string code = read_file(filename);
    if (!silent_mode) {
        std::cout << "Code:\n" << code << std::endl;
    }

    try {
        Lexer lexer(code, silent_mode);
        std::vector<Token> tokens = lexer.tokenize();
        if (!silent_mode) {
            std::cout << "Tokens generated successfully (" << tokens.size() << " tokens)\n";
        }

        SymbolTable sym_table;
        Parser parser(sym_table, silent_mode);
        std::vector<OPS> ops = parser.parse(tokens);
        if (!silent_mode) {
            std::cout << "OPS generated successfully (" << ops.size() << " operations):\n";
            for (size_t i = 0; i < ops.size(); ++i) {
                std::cout << i << ": " << ops[i].operation << " " << ops[i].operand << "\n";
            }
        }

        Interpreter interpreter(sym_table, silent_mode);
        if (!silent_mode) {
            std::cout << "Please provide input for 'read' operations (e.g., '3 4' for test1.txt): ";
        }
        interpreter.execute(ops);

        std::cout << "Test completed successfully\n";
    }
    catch (const Error& e) {
        std::cerr << "Error in " << filename << ": " << e.message() << "\n";
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    bool silent_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--silent") {
            silent_mode = true;
            break;
        }
    }
    silent_mode = true;
    std::vector<std::string> test_files = { "test1.txt", "test2.txt", "test3.txt", "test4.txt" };
    for (const auto& file : test_files) {
        run_test(file, silent_mode);
    }
    std::cout << "=== All tests completed ===\n";
    return 0;
}