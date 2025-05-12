#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include <iostream>
#include <fstream>
#include <sstream>

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
    std::cout << "=== Running test: " << filename << " ===\n";
    std::string code = read_file(filename);
    std::cout << "Code:\n" << code << std::endl;

    try {
        Lexer lexer(code);
        std::vector<Token> tokens = lexer.tokenize();
        std::cout << "Tokens generated successfully (" << tokens.size() << " tokens)\n";

        SymbolTable sym_table;
        Parser parser(sym_table);
        std::vector<OPS> ops = parser.parse(tokens);
        std::cout << "OPS generated successfully (" << ops.size() << " operations):\n";
        for (size_t i = 0; i < ops.size(); ++i) {
            std::cout << i << ": " << ops[i].operation << " " << ops[i].operand << "\n";
        }

        Interpreter interpreter(sym_table);
        std::cout << "Please provide input for 'read' operations (e.g., '3 4' for test1.txt): ";
        interpreter.execute(ops);

        std::cout << "Test completed successfully\n";
    }
    catch (const Error& e) {
        std::cerr << "Error in " << filename << ": " << e.message() << "\n";
    }
    std::cout << std::endl;
}

// "test1.txt", "test2.txt", "test3.txt", "test4.txt", "test5.txt"

int main() { 
    std::vector<std::string> test_files = { "test1.txt", "test2.txt", "test3.txt", "test4.txt", "test5.txt" };
    for (const auto& file : test_files) {
        run_test(file);
    }
    std::cout << "=== All tests completed ===\n";
    return 0;
}