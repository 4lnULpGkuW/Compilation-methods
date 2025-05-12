#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector> // ��������� ��� std::vector
#include <string> // ��������� ��� std::string

// ���������� ���� ��� ������ ������
bool silent_mode = false; // ���������� true ��� ������ ������

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
        std::string code_content = read_file(filename); // ������ ���������� ����� ��� ������
        std::cout << "Code:\n" << code_content << std::endl;
    }


    std::string code = read_file(filename); // ��� �������

    try {
        Lexer lexer(code);
        std::vector<Token> tokens = lexer.tokenize();
        if (!silent_mode) {
            std::cout << "Tokens generated successfully (" << tokens.size() << " tokens)\n";
        }

        SymbolTable sym_table;
        Parser parser(sym_table);
        parser.set_silent_mode(silent_mode); // �������� ���� � ������

        std::vector<OPS> ops_list = parser.parse(tokens); // ������������ ops � ops_list
        if (!silent_mode) {
            std::cout << "OPS generated successfully (" << ops_list.size() << " operations):\n";
            for (size_t i = 0; i < ops_list.size(); ++i) {
                std::cout << i << ": " << ops_list[i].operation << (ops_list[i].operand.empty() ? "" : " " + ops_list[i].operand) << "\n";
            }
        }

        Interpreter interpreter(sym_table);
        interpreter.set_silent_mode(silent_mode); // �������� ���� � �������������

        if (!silent_mode) { // ����������� � ����� ���������� ������, ����� ��� �����
            // �� � ������ ������, ���� silent_mode, ������������ �� ����� �����, ��� �� ���� ���� �����.
            // ��� ����� ��������. ���� ���, ���� silent, �� � ����� �� �����.
            // ����, ���� ���� �������� 'r', �� ������ �������� "Please provide input..."
            bool needs_input = false;
            for (const auto& op_item : ops_list) { // ������������ op � op_item
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

int main(int argc, char* argv[]) { // ��������� argc � argv ��� ���������� ��������� ������
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
        std::cout << "All tests processed (silent mode).\n"; // ������� ��������� � ����� ������
    }
    return 0;
}