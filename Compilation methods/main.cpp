#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <clocale>
#include <windows.h>

bool silent_mode = false;

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
    if (!silent_mode) {
        std::string code_content = read_file(filename);
        std::cout << "Code:\n" << code_content << std::endl;
    }

    std::string code = read_file(filename);

    try {
        Lexer lexer(code);
        std::vector<Token> tokens = lexer.tokenize();
        if (!silent_mode) {
            std::cout << "Tokens generated successfully (" << tokens.size() << " tokens)\n";
            std::cout << "Tokens:\n";
            for (const auto& t : tokens) {
                std::cout << t.type << " '" << t.value << "' (line " << t.line << ")\n";
            }
        }

        SymbolTable sym_table;
        Parser parser(sym_table);
        parser.set_silent_mode(silent_mode);

        std::vector<OPS> ops_list = parser.parse(tokens);

        Interpreter interpreter(sym_table);
        interpreter.set_silent_mode(silent_mode);

        if (!silent_mode) {
            std::cout << "Symbol table before execution:\n";
            sym_table.print();
        }

        bool needs_input = false;
        for (const auto& op_item : ops_list) {
            if (op_item.operation == "r" || op_item.operation == "array_read") {
                needs_input = true;
                break;
            }
        }
        if (needs_input && !silent_mode) {
            std::cout << "Please provide input for 'read' operations: ";
        }
        interpreter.execute(ops_list);

        if (!silent_mode) {
            std::cout << "Execution finished. Symbol table final state:\n";
            sym_table.print();
        }
        std::cout << "Test " << filename << " completed successfully\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error in " << filename << ": " << e.what() << "\n";
    }
    if (!silent_mode) {
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);
    std::locale::global(std::locale("en_US.UTF-8"));

    if (argc > 1 && std::string(argv[1]) == "--silent") {
        silent_mode = true;
    }

    silent_mode = true;
    
    std::vector<std::string> test_files = { "test1.txt", "test2.txt", "test3.1.txt", "test3.2.txt", "test3.3.txt", "test4.txt", "test5.txt", "test6.txt" };
    for (const auto& file : test_files) {
        run_test(file);
    }
    std::cout << "=== All tests completed ===\n";
    return 0;
}