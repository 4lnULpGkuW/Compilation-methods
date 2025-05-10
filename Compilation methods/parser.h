#ifndef PARSER_H
#define PARSER_H

#include "symbol_table.h"
#include "token.h"
#include "ops.h"
#include <vector>
#include <string>
#include <stack>
#include <set>

class Parser {
public:
    Parser(SymbolTable& sym_table);
    std::vector<OPS> parse(const std::vector<Token>& tokens);

private:
    SymbolTable& sym_table;
    std::vector<Token> tokens;
    size_t pos;
    std::vector<OPS> ops;
    std::vector<OPS> read_ops;
    std::stack<size_t> label_stack;
    std::set<std::string> declared_arrays;

    Token expect(const std::string& type, const std::string& value = "");
    bool match(const std::string& type, const std::string& value = "");
    void parseProgram();
    void parseStmt();
    void parseDeclInt();
    void parseUseInt();
    void parseAlternative();
    void parseInitializers();
    void parseInitCont();
    void parseExpr();
    void parseExprCont();
    void parseTerm();
    void parseTermCont();
    void parseFactor();
    size_t parseLogExpr();
    size_t parseLogCmp(std::vector<OPS>& left_ops);
    void parseLogCont();
    void add_ops(const std::string& operation, const std::string& operand = "");
    void push_label(size_t pos);
    size_t pop_label();
    void set_jump(size_t label_pos, size_t target);
};

#endif