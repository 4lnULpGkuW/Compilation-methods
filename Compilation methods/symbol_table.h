#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "error.h"
#include <string>
#include <map>
#include <vector>

class SymbolTable {
public:
    SymbolTable();
    void add_variable(const std::string& name, int value);
    void add_array(const std::string& name, int size);
    bool exists(const std::string& name) const;
    int get_variable(const std::string& name) const;
    void set_variable(const std::string& name, int value);
    std::vector<int>& get_array(const std::string& name);
    void set_array_element(const std::string& name, int index, int value);
    const std::map<std::string, int>& get_variables() const;
    void clear();
    void print() const;
    std::vector<int>* get_array_maybe(const std::string& name);
    const std::vector<int>* get_array_maybe(const std::string& name) const;
    void set_silent_mode(bool mode);
private:
    std::map<std::string, int> variables;
    std::map<std::string, std::vector<int>> arrays;

    bool silent_mode_active;
};

#endif // SYMBOL_TABLE_H