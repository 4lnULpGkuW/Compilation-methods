#include "interpreter.h"
#include <stdexcept>
#include <iostream>
#include <algorithm> 
#include <limits> 

Interpreter::Interpreter(SymbolTable& sym_table) : sym_table(sym_table), silent_mode_active(false) {}

void Interpreter::set_silent_mode(bool mode) {
    silent_mode_active = mode;
}

void Interpreter::execute(const std::vector<OPS>& ops_list) {
    std::stack<int> stack;
    size_t pc = 0;

    if (!silent_mode_active) {
        std::cout << "Symbol table before execution:\n";
        sym_table.print();
    }

    while (pc < ops_list.size()) {
        const OPS& op = ops_list[pc];
        if (!silent_mode_active) {
            std::cout << "Executing op " << pc << ": " << op.operation << (op.operand.empty() ? "" : " " + op.operand) << "\n";
        }

        if (op.operation == "r") {
            int value;
            std::cout << "Enter value for " << op.operand << ": ";
            std::cin >> value;
            if (std::cin.fail()) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                throw std::runtime_error("Invalid input for read operation");
            }
            sym_table.set_variable(op.operand, value);
            if (!silent_mode_active) {
                std::cout << "Read " << value << " into " << op.operand << "\n";
            }
        }
        else if (op.operation == "") {

            if (sym_table.exists(op.operand)) {
                try {
                    int value = sym_table.get_variable(op.operand);
                    stack.push(value);
                    if (!silent_mode_active) std::cout << "Pushed variable " << op.operand << ": " << value << "\n";
                }
                catch (const std::runtime_error& e) {
                    throw std::runtime_error("Operand " + op.operand + " is likely an array used as a variable, or not found. Details: " + e.what());
                }
            }
            else {
                try {
                    int value = std::stoi(op.operand);
                    stack.push(value);
                    if (!silent_mode_active) std::cout << "Pushed number: " << value << "\n";
                }
                catch (const std::invalid_argument& ia) {
                    throw std::runtime_error("Invalid number for push: " + op.operand);
                }
                catch (const std::out_of_range& oor) {
                    throw std::runtime_error("Number out of range for push: " + op.operand);
                }
            }
        }

        else if (op.operation == "+") {
            if (stack.size() < 2) throw std::runtime_error("Stack underflow for + operation at pc " + std::to_string(pc));
            int right = stack.top(); stack.pop();
            int left = stack.top(); stack.pop();
            stack.push(left + right);
            if (!silent_mode_active) {
                std::cout << "Computed " << left << " + " << right << " = " << (left + right) << "\n";
            }
        }
        else if (op.operation == "-") {
            if (stack.size() < 2) throw std::runtime_error("Stack underflow for - operation at pc " + std::to_string(pc));
            int right = stack.top(); stack.pop();
            int left = stack.top(); stack.pop();
            stack.push(left - right);
            if (!silent_mode_active) std::cout << "Computed " << left << " - " << right << " = " << (left - right) << "\n";
        }
        else if (op.operation == "*") {
            if (stack.size() < 2) throw std::runtime_error("Stack underflow for * operation at pc " + std::to_string(pc));
            int right = stack.top(); stack.pop();
            int left = stack.top(); stack.pop();
            stack.push(left * right);
            if (!silent_mode_active) std::cout << "Computed " << left << " * " << right << " = " << (left * right) << "\n";
        }
        else if (op.operation == "/") {
            if (stack.size() < 2) throw std::runtime_error("Stack underflow for / operation at pc " + std::to_string(pc));
            int right = stack.top(); stack.pop();
            int left = stack.top(); stack.pop();
            if (right == 0) throw std::runtime_error("Division by zero at pc " + std::to_string(pc));
            stack.push(left / right);
            if (!silent_mode_active) std::cout << "Computed " << left << " / " << right << " = " << (left / right) << "\n";
        }
        else if (op.operation == "~") {
            if (stack.empty()) throw std::runtime_error("Stack underflow for ~ operation at pc " + std::to_string(pc));
            int val = stack.top(); stack.pop();
            stack.push(-val);
            if (!silent_mode_active) std::cout << "Computed ~" << val << " = " << (-val) << "\n";
        }
        else if (op.operation == ">") {
            if (stack.size() < 2) throw std::runtime_error("Stack underflow for > operation at pc " + std::to_string(pc));
            int right = stack.top(); stack.pop();
            int left = stack.top(); stack.pop();
            stack.push(left > right ? 1 : 0);
            if (!silent_mode_active) std::cout << "Computed " << left << " > " << right << " = " << (left > right ? 1 : 0) << "\n";
        }
        else if (op.operation == "<") {
            if (stack.size() < 2) throw std::runtime_error("Stack underflow for < operation at pc " + std::to_string(pc));
            int right = stack.top(); stack.pop();
            int left = stack.top(); stack.pop();
            stack.push(left < right ? 1 : 0);
            if (!silent_mode_active) std::cout << "Computed " << left << " < " << right << " = " << (left < right ? 1 : 0) << "\n";
        }
        else if (op.operation == "==") {
            if (stack.size() < 2) throw std::runtime_error("Stack underflow for == operation at pc " + std::to_string(pc));
            int right = stack.top(); stack.pop();
            int left = stack.top(); stack.pop();
            stack.push(left == right ? 1 : 0);
            if (!silent_mode_active) std::cout << "Computed " << left << " == " << right << " = " << (left == right ? 1 : 0) << "\n";
        }
        else if (op.operation == "&") {
            if (stack.size() < 2) throw std::runtime_error("Stack underflow for & operation at pc " + std::to_string(pc));
            int right = stack.top(); stack.pop();
            int left = stack.top(); stack.pop();
            stack.push((left != 0) && (right != 0) ? 1 : 0);
            if (!silent_mode_active) std::cout << "Computed " << left << " & " << right << " = " << ((left != 0) && (right != 0) ? 1 : 0) << "\n";
        }
        else if (op.operation == "|") {
            if (stack.size() < 2) throw std::runtime_error("Stack underflow for | operation at pc " + std::to_string(pc));
            int right = stack.top(); stack.pop();
            int left = stack.top(); stack.pop();
            stack.push((left != 0) || (right != 0) ? 1 : 0);
            if (!silent_mode_active) std::cout << "Computed " << left << " | " << right << " = " << ((left != 0) || (right != 0) ? 1 : 0) << "\n";
        }
        else if (op.operation == "!") {
            if (stack.empty()) throw std::runtime_error("Stack underflow for ! operation at pc " + std::to_string(pc));
            int val = stack.top(); stack.pop();
            stack.push(val == 0 ? 1 : 0);
            if (!silent_mode_active) std::cout << "Computed !" << val << " = " << (val == 0 ? 1 : 0) << "\n";
        }
        else if (op.operation == "jf") {
            if (op.operand.empty()) throw std::runtime_error("jf missing target operand at pc " + std::to_string(pc));
            if (stack.empty()) throw std::runtime_error("Stack underflow for jf condition at pc " + std::to_string(pc));
            int condition = stack.top(); stack.pop();
            if (!silent_mode_active) {
                std::cout << "jf condition: " << condition << ", target: " << op.operand << "\n";
            }
            if (condition == 0) {
                try {
                    pc = std::stoul(op.operand);
                }
                catch (const std::invalid_argument& ia) {
                    throw std::runtime_error("Invalid target for jf: " + op.operand + " at pc " + std::to_string(pc - 1));
                }
                catch (const std::out_of_range& oor) {
                    throw std::runtime_error("Target out of range for jf: " + op.operand + " at pc " + std::to_string(pc - 1));
                }
                if (!silent_mode_active) {
                    std::cout << "Jumping to " << pc << "\n";
                }
                continue;
            }
        }
        else if (op.operation == "j") {
            if (op.operand.empty()) throw std::runtime_error("j missing target operand at pc " + std::to_string(pc));
            try {
                pc = std::stoul(op.operand);
            }
            catch (const std::invalid_argument& ia) {
                throw std::runtime_error("Invalid target for j: " + op.operand + " at pc " + std::to_string(pc - 1));
            }
            catch (const std::out_of_range& oor) {
                throw std::runtime_error("Target out of range for j: " + op.operand + " at pc " + std::to_string(pc - 1));
            }
            if (!silent_mode_active) {
                std::cout << "Jumping to " << pc << "\n";
            }
            continue;
        }
        else if (op.operation == "=") {
            if (stack.empty()) throw std::runtime_error("Stack underflow for = operation (value) at pc " + std::to_string(pc));
            int value = stack.top(); stack.pop();
            sym_table.set_variable(op.operand, value);
            if (!silent_mode_active) {
                std::cout << "Set " << op.operand << " = " << value << "\n";
            }
        }
        else if (op.operation == "alloc_array") {
            if (stack.empty()) throw std::runtime_error("Stack underflow for alloc_array size at pc " + std::to_string(pc));
            int size = stack.top(); stack.pop();
            if (size <= 0) throw std::runtime_error("Invalid array size: " + std::to_string(size) + " for array " + op.operand + " at pc " + std::to_string(pc));
            sym_table.add_array(op.operand, size);
            if (!silent_mode_active) {
                std::cout << "Allocated array " << op.operand << " of size " << size << "\n";
            }
        }
        else if (op.operation == "init_array") {
            int num_initializers = 0;
            try {
                num_initializers = std::stoi(op.operand);
            }
            catch (const std::exception& e) {
                throw std::runtime_error("Invalid operand for init_array: " + op.operand + " at pc " + std::to_string(pc));
            }

            if (num_initializers < 0) throw std::runtime_error("Negative number of initializers for init_array at pc " + std::to_string(pc));
            if (stack.size() < static_cast<size_t>(num_initializers)) {
                throw std::runtime_error("Stack underflow during array initialization, expected " + std::to_string(num_initializers) + " values, got " + std::to_string(stack.size()) + " at pc " + std::to_string(pc));
            }

            std::vector<int> initial_values(num_initializers);
            for (int i = num_initializers - 1; i >= 0; --i) {
                initial_values[i] = stack.top();
                stack.pop();
            }

            std::string array_name;
            bool found_alloc = false;

            int search_k_limit = static_cast<int>(pc) - 1;
            for (int k = search_k_limit; k >= 0; --k) {
                if (k < static_cast<int>(ops_list.size()) && ops_list[k].operation == "alloc_array") { // Проверка границ k
                    array_name = ops_list[k].operand;
                    found_alloc = true;
                    break;
                }
            }
            if (!found_alloc || array_name.empty()) {
                throw std::runtime_error("Could not find corresponding alloc_array for init_array of (operand was " + op.operand + ", num_initializers: " + std::to_string(num_initializers) + ") at pc " + std::to_string(pc));
            }

            std::vector<int>& arr = sym_table.get_array(array_name);
            if (num_initializers > static_cast<int>(arr.size())) {
                throw std::runtime_error("Too many initializers (" + std::to_string(num_initializers) + ") for array " + array_name + " of size " + std::to_string(arr.size()) + " at pc " + std::to_string(pc));
            }
            for (int i = 0; i < num_initializers; ++i) {
                arr[i] = initial_values[i];
            }
            if (!silent_mode_active) {
                std::cout << "Initialized array " << array_name << " with " << num_initializers << " values\n";
            }
        }
        else if (op.operation == "array_read") {
            if (stack.empty()) throw std::runtime_error("Stack underflow for array_read index at pc " + std::to_string(pc));
            int index = stack.top(); stack.pop();
            int value;
            std::cout << "Enter value for " << op.operand << "[" << index << "]: "; // Оставляем этот вывод
            std::cin >> value;
            if (std::cin.fail()) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                throw std::runtime_error("Invalid input for array_read operation");
            }
            sym_table.set_array_element(op.operand, index, value);
            if (!silent_mode_active) {
                std::cout << "Read " << value << " into " << op.operand << "[" << index << "]\n";
            }
        }
        else if (op.operation == "array_get") {
            if (stack.empty()) throw std::runtime_error("Stack underflow for array_get index at pc " + std::to_string(pc));
            int index = stack.top(); stack.pop();
            const std::vector<int>& arr = sym_table.get_array(op.operand);
            if (index < 0 || index >= static_cast<int>(arr.size())) {
                throw std::runtime_error("Array index out of bounds: " + std::to_string(index) + " for array " + op.operand + " of size " + std::to_string(arr.size()) + " at pc " + std::to_string(pc));
            }
            stack.push(arr[index]);
            if (!silent_mode_active) {
                std::cout << "Pushed " << op.operand << "[" << index << "] = " << arr[index] << "\n";
            }
        }
        else if (op.operation == "array_set") {
            if (stack.size() < 2) throw std::runtime_error("Stack underflow for array_set operation (value or index missing) at pc " + std::to_string(pc));
            int value = stack.top(); stack.pop();
            int index = stack.top(); stack.pop();
            sym_table.set_array_element(op.operand, index, value);
            if (!silent_mode_active) {
                std::cout << "Set " << op.operand << "[" << index << "] = " << value << "\n";
            }
        }
        else if (op.operation == "w") {
            if (stack.empty()) throw std::runtime_error("Stack is empty for 'w' operation at pc " + std::to_string(pc));
            int value = stack.top(); stack.pop();
            std::cout << "Output: " << value << "\n"; // Оставляем этот вывод
        }
        else {
            throw std::runtime_error("Unknown operation: " + op.operation + " at pc " + std::to_string(pc));
        }
        pc++;
    }
    if (!silent_mode_active) {
        std::cout << "Execution finished. Symbol table final state:\n";
        sym_table.print();
    }
}