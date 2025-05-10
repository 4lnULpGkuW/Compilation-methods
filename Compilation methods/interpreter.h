#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "symbol_table.h"
#include "ops.h"
#include <stack>
#include <vector>
#include <string>

class Interpreter {
public:
    Interpreter(SymbolTable& sym_table);
    void execute(const std::vector<OPS>& ops);
private:
    SymbolTable& sym_table;
    size_t pc; // Program counter
    std::stack<int> stack; // Operand stack
    bool is_number(const std::string& s);
};

#endif // INTERPRETER_H