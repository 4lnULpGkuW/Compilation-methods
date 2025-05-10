#include "interpreter.h"
#include <stdexcept>
#include <iostream>

Interpreter::Interpreter(SymbolTable& sym_table) : sym_table(sym_table) {}

void Interpreter::execute(const std::vector<OPS>& ops) {
    std::stack<int> stack;
    size_t pc = 0;

    std::cout << "Symbol table before execution:\n";
    sym_table.print();

    while (pc < ops.size()) {
        const OPS& op = ops[pc];
        std::cout << "Executing op " << pc << ": " << op.operation << (op.operand.empty() ? "" : " " + op.operand) << "\n";

        if (op.operation == "r") {
            int value;
            std::cin >> value;
            sym_table.set_variable(op.operand, value);
            std::cout << "Read " << value << " into " << op.operand << "\n";
        }
        else if (op.operation == "") {
            if (sym_table.exists(op.operand)) {
                int value = sym_table.get_variable(op.operand);
                stack.push(value);
                std::cout << "Pushed variable " << op.operand << ": " << value << "\n";
            }
            else {
                int value = std::stoi(op.operand);
                stack.push(value);
                std::cout << "Pushed number: " << value << "\n";
            }
        }
        else if (op.operation == "+") {
            int right = stack.top(); stack.pop();
            int left = stack.top(); stack.pop();
            stack.push(left + right);
            std::cout << "Computed " << left << " + " << right << " = " << (left + right) << "\n";
        }
        else if (op.operation == "-") {
            int right = stack.top(); stack.pop();
            int left = stack.top(); stack.pop();
            stack.push(left - right);
            std::cout << "Computed " << left << " - " << right << " = " << (left - right) << "\n";
        }
        else if (op.operation == "*") {
            int right = stack.top(); stack.pop();
            int left = stack.top(); stack.pop();
            stack.push(left * right);
            std::cout << "Computed " << left << " * " << right << " = " << (left * right) << "\n";
        }
        else if (op.operation == "/") {
            int right = stack.top(); stack.pop();
            int left = stack.top(); stack.pop();
            if (right == 0) throw std::runtime_error("Division by zero");
            stack.push(left / right);
            std::cout << "Computed " << left << " / " << right << " = " << (left / right) << "\n";
        }
        else if (op.operation == ">") {
            int right = stack.top(); stack.pop();
            int left = stack.top(); stack.pop();
            int result = left > right ? 1 : 0;
            stack.push(result);
            std::cout << "Comparing " << left << " > " << right << " = " << result << "\n";
        }
        else if (op.operation == "jf") {
            int condition = stack.top(); stack.pop();
            std::cout << "jf condition: " << condition << ", target: " << op.operand << "\n";
            if (condition == 0) {
                pc = std::stoul(op.operand);
                std::cout << "Jumping to " << pc << "\n";
                continue;
            }
        }
        else if (op.operation == "j") {
            pc = std::stoul(op.operand);
            std::cout << "Jumping to " << pc << "\n";
            continue;
        }
        else if (op.operation == "=") {
            int value = stack.top(); stack.pop();
            sym_table.set_variable(op.operand, value);
            std::cout << "Set " << op.operand << " = " << value << "\n";
        }
        else if (op.operation == "alloc_array") {
            int size = stack.top(); stack.pop();
            if (size <= 0) throw std::runtime_error("Invalid array size: " + std::to_string(size));
            sym_table.add_array(op.operand, size);
            std::cout << "Allocated array " << op.operand << " of size " << size << "\n";
        }
        else if (op.operation == "array_read") {
            int index = stack.top(); stack.pop();
            int value;
            std::cin >> value;
            sym_table.set_array_element(op.operand, index, value);
            std::cout << "Read " << value << " into " << op.operand << "[" << index << "]\n";
        }
        else if (op.operation == "array_get") {
            int index = stack.top(); stack.pop();
            std::vector<int>& arr = sym_table.get_array(op.operand);
            if (index < 0 || index >= arr.size()) {
                throw std::runtime_error("Array index out of bounds: " + std::to_string(index) + " for array " + op.operand);
            }
            stack.push(arr[index]);
            std::cout << "Pushed " << op.operand << "[" << index << "] = " << arr[index] << "\n";
        }
        else if (op.operation == "array_set") {
            int value = stack.top(); stack.pop();
            int index = stack.top(); stack.pop();
            sym_table.set_array_element(op.operand, index, value);
            std::cout << "Set " << op.operand << "[" << index << "] = " << value << "\n";
        }
        else if (op.operation == "w") {
            int value = stack.top(); stack.pop();
            std::cout << value << "\n";
        }
        else {
            throw std::runtime_error("Unknown operation: " + op.operation);
        }

        pc++;
    }
}