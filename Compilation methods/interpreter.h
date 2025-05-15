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
    void set_silent_mode(bool mode); // Новый метод
private:
    SymbolTable& sym_table;
    bool silent_mode_active; // Флаг для интерпретатора
};

#endif