#ifndef OPS_H
#define OPS_H

#include <string>

struct OPS {
    std::string operation;
    std::string operand;
    OPS(const std::string& op, const std::string& oper = "") : operation(op), operand(oper) {}
};

#endif