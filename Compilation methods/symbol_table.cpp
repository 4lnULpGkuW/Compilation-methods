#include "symbol_table.h"
#include <stdexcept>
#include <iostream>

SymbolTable::SymbolTable() {}

void SymbolTable::add_variable(const std::string& name, int value) {
    if (variables.find(name) != variables.end() || arrays.find(name) != arrays.end()) {
        throw std::runtime_error("Variable or array '" + name + "' already exists");
    }
    variables[name] = value;
    if (!silent_mode_active) {
        std::cout << "Added variable: " << name << " = " << value << "\n";
    }
}

void SymbolTable::add_array(const std::string& name, int size) {
    if (variables.find(name) != variables.end() || arrays.find(name) != arrays.end()) {
        throw std::runtime_error("Variable or array '" + name + "' already exists");
    }
    arrays[name] = std::vector<int>(size, 0);
    if (!silent_mode_active) {
        std::cout << "Added array: " << name << " with size " << size << "\n";
    }
}

bool SymbolTable::exists(const std::string& name) const {
    return variables.find(name) != variables.end() || arrays.find(name) != arrays.end();
}

int SymbolTable::get_variable(const std::string& name) const {
    auto it = variables.find(name);
    if (it == variables.end()) {
        throw std::runtime_error("Variable '" + name + "' not found");
    }
    return it->second;
}

void SymbolTable::set_variable(const std::string& name, int value) {
    auto it = variables.find(name);
    if (it == variables.end()) {
        throw std::runtime_error("Variable '" + name + "' not found");
    }
    it->second = value;
}

std::vector<int>& SymbolTable::get_array(const std::string& name) {
    auto it = arrays.find(name);
    if (it == arrays.end()) {
        throw std::runtime_error("Array '" + name + "' not found");
    }
    return it->second;
}

void SymbolTable::set_array_element(const std::string& name, int index, int value) {
    auto it = arrays.find(name);
    if (it == arrays.end()) {
        throw std::runtime_error("Array '" + name + "' not found");
    }
    if (index < 0 || index >= static_cast<int>(it->second.size())) {
        throw std::runtime_error("Array index out of bounds for '" + name + "': " + std::to_string(index));
    }
    it->second[index] = value;
}

const std::map<std::string, int>& SymbolTable::get_variables() const {
    return variables;
}

void SymbolTable::clear() {
    variables.clear();
    arrays.clear();
}

void SymbolTable::print() const {
    for (const auto& var : variables) {
        std::cout << "Variable: " << var.first << ", Value: " << var.second << "\n";
    }
    for (const auto& arr : arrays) {
        std::cout << "Array: " << arr.first << ", Size: " << arr.second.size() << ", Values: [";
        for (size_t i = 0; i < arr.second.size(); ++i) {
            std::cout << arr.second[i];
            if (i < arr.second.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
    }
}

std::vector<int>* SymbolTable::get_array_maybe(const std::string& name) {
    auto it = arrays.find(name);
    if (it == arrays.end()) {
        return nullptr;
    }
    return &(it->second);
}

const std::vector<int>* SymbolTable::get_array_maybe(const std::string& name) const {
    auto it = arrays.find(name);
    if (it == arrays.end()) {
        return nullptr;
    }
    return &(it->second);
}

void SymbolTable::set_silent_mode(bool mode) {
    silent_mode_active = mode;
}