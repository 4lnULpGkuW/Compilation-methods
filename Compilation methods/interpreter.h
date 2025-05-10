#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "symbol_table.h"
#include "ops.h"
#include <stack>
#include <vector>

class Interpreter {
public:
    Interpreter(SymbolTable& sym_table, bool silent = false);
    void execute(const std::vector<OPS>& ops);

private:
    SymbolTable& sym_table;
    bool silent_mode;
    size_t pc;
    std::stack<int> stack;
};

#endif // INTERPRETER_H