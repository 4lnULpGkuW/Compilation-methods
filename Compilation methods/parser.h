#ifndef PARSER_H
#define PARSER_H

#include "symbol_table.h"
#include "token.h"
#include "ops.h"
#include <vector>
#include <string>
#include <stack>
#include <set>
#include <limits>

class Parser {
public:
    Parser(SymbolTable& sym_table);
    std::vector<OPS> parse(const std::vector<Token>& tokens);
    void set_silent_mode(bool mode); // Новый метод

private:
    SymbolTable& sym_table;
    std::vector<Token> tokens;
    size_t pos;
    std::vector<OPS> ops_list; // Переименовано с ops на ops_list во избежание конфликта имен
    std::stack<size_t> label_stack;
    std::set<std::string> declared_arrays;
    bool silent_mode_active; // Флаг для парсера

    Token expect(const std::string& type, const std::string& value = "");
    bool match(const std::string& type, const std::string& value = "");
    void parseProgram();
    void parseStmt();
    void parseDeclInt();
    void parseUseInt();
    void parseInitializers(int& count);
    void parseInitCont(int& count);
    void parseExpr();
    void parseExprCont();
    void parseTerm();
    void parseTermCont();
    void parseFactor();
    void parseLogExpr();
    void parseLogOrCont();
    void parseLogAndTerm();
    void parseLogAndCont();
    void parseLogNotTerm();
    void add_ops(const std::string& operation, const std::string& operand = "");
    void push_label(size_t p);
    size_t pop_label();
    void set_jump(size_t label_pos, size_t target);
};

#endif