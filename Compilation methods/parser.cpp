#include "parser.h"
#include <stdexcept>
#include <iostream>
#include <set>
#include <limits>
#include <algorithm>

Parser::Parser(SymbolTable& sym_table) : sym_table(sym_table), pos(0), silent_mode_active(false) {}

void Parser::set_silent_mode(bool mode) {
    silent_mode_active = mode;
}

std::vector<OPS> Parser::parse(const std::vector<Token>& tokens) {
    if (tokens.empty()) {
        std::cerr << "Error: Token list is empty\n";
        return std::vector<OPS>();
    }
    this->tokens = tokens;
    pos = 0;
    ops_list.clear(); // Используем ops_list
    label_stack = std::stack<size_t>();
    declared_arrays.clear();
    std::vector<std::string> errors;
    if (!silent_mode_active) {
        std::cout << "Starting parse with " << tokens.size() << " tokens\n";
    }
    while (pos < tokens.size() && tokens[pos].type != "EOF") {
        try {
            parseStmt();
        }
        catch (const std::exception& e) {
            std::cerr << "Parse error: " << e.what() << " at line " << (pos < tokens.size() ? tokens[pos].line : 0)
                << ", position " << (pos < tokens.size() ? tokens[pos].pos : 0);
            if (pos < tokens.size()) {
                std::cerr << " (token: " << tokens[pos].type << " '" << tokens[pos].value << "')\n";
            }
            else {
                std::cerr << " (end of input)\n";
            }
            errors.push_back(e.what());
            while (pos < tokens.size() && !(tokens[pos].type == "SYMBOL" && (tokens[pos].value == ";" || tokens[pos].value == "}")) && tokens[pos].type != "EOF") {
                pos++;
            }
            if (pos < tokens.size() && tokens[pos].type == "SYMBOL" && (tokens[pos].value == ";" || tokens[pos].value == "}")) {
                if (tokens[pos].value == "}") {
                }
                else {
                    pos++;
                }
            }
        }
    }
    if (errors.empty()) {
        if (!silent_mode_active) {
            std::cout << "OPS generated successfully (" << ops_list.size() << " operations):\n";
            for (size_t i = 0; i < ops_list.size(); ++i) {
                std::cout << i << ": " << ops_list[i].operation << (ops_list[i].operand.empty() ? "" : " " + ops_list[i].operand) << "\n";
            }
        }
    }
    else {
        std::cerr << "Parsing failed with " << errors.size() << " errors\n";
    }
    return ops_list; // Возвращаем ops_list
}

Token Parser::expect(const std::string& type, const std::string& value) {
    if (pos >= tokens.size()) {
        throw std::runtime_error("Unexpected end of input, expected " + type + (value.empty() ? "" : " '" + value + "'"));
    }
    if (tokens[pos].type != type || (!value.empty() && tokens[pos].value != value)) {
        throw std::runtime_error("Expected token " + type + (value.empty() ? "" : " '" + value + "')") +
            ", found '" + tokens[pos].type + "' ('" + tokens[pos].value +
            "') at line " + std::to_string(tokens[pos].line) +
            ", position " + std::to_string(tokens[pos].pos));
    }
    if (!silent_mode_active) {
        std::cout << "Expected " << type << (value.empty() ? "" : " '" + value + "'") << ", got " << tokens[pos].value << "\n";
    }
    return tokens[pos++];
}

bool Parser::match(const std::string& type, const std::string& value) {
    if (pos < tokens.size() && tokens[pos].type == type && (value.empty() || tokens[pos].value == value)) {
        if (!silent_mode_active) {
            std::cout << "Matched " << type << (value.empty() ? "" : " '" + value + "'") << ": " << tokens[pos].value << "\n";
        }
        pos++;
        return true;
    }
    return false;
}

void Parser::parseProgram() {
    if (!silent_mode_active) {
        std::cout << "Parsing Program at pos " << pos << "\n";
    }
    while (pos < tokens.size() && tokens[pos].type != "EOF" && !(tokens[pos].type == "SYMBOL" && tokens[pos].value == "}")) {
        parseStmt();
    }
}

void Parser::parseStmt() {
    if (!silent_mode_active) {
        std::cout << "Parsing Stmt at pos " << pos;
        if (pos < tokens.size()) {
            std::cout << " (token: " << tokens[pos].type << " '" << tokens[pos].value << "')\n";
        }
        else {
            std::cout << " (end of input)\n";
        }
    }
    if (pos >= tokens.size() || (pos < tokens.size() && tokens[pos].type == "SYMBOL" && tokens[pos].value == "}")) {
        return;
    }


    if (tokens[pos].type == "KEYWORD" && tokens[pos].value == "int") {
        match("KEYWORD", "int");
        parseDeclInt();
    }
    else if (tokens[pos].type == "ID") {
        parseUseInt();
    }
    else if (tokens[pos].type == "KEYWORD" && tokens[pos].value == "if") {
        match("KEYWORD", "if");
        expect("SYMBOL", "(");
        parseLogExpr();
        expect("SYMBOL", ")");

        add_ops("jf", "");
        size_t jf_pos = ops_list.size() - 1;

        expect("SYMBOL", "{");
        parseProgram();
        expect("SYMBOL", "}");

        if (pos < tokens.size() && tokens[pos].type == "KEYWORD" && tokens[pos].value == "else") {
            match("KEYWORD", "else");
            add_ops("j", "");
            size_t j_else_skip_pos = ops_list.size() - 1;
            set_jump(jf_pos, ops_list.size());

            expect("SYMBOL", "{");
            parseProgram();
            expect("SYMBOL", "}");
            set_jump(j_else_skip_pos, ops_list.size());
        }
        else {
            set_jump(jf_pos, ops_list.size());
        }
    }
    else if (tokens[pos].type == "KEYWORD" && tokens[pos].value == "while") {
        match("KEYWORD", "while");
        size_t loop_start = ops_list.size();
        expect("SYMBOL", "(");
        parseLogExpr();
        expect("SYMBOL", ")");

        add_ops("jf", "");
        size_t jf_pos = ops_list.size() - 1;

        expect("SYMBOL", "{");
        parseProgram();
        expect("SYMBOL", "}");
        add_ops("j", std::to_string(loop_start));
        set_jump(jf_pos, ops_list.size());
    }
    else if (tokens[pos].type == "KEYWORD" && tokens[pos].value == "read") {
        match("KEYWORD", "read");
        expect("SYMBOL", "(");
        std::string id = expect("ID").value;
        if (!sym_table.exists(id) && declared_arrays.find(id) == declared_arrays.end()) {
            throw std::runtime_error("Undefined variable or array '" + id + "' at line " +
                std::to_string(tokens[pos - 1].line) + ", position " +
                std::to_string(tokens[pos - 1].pos));
        }
        if (match("SYMBOL", "[")) {
            parseExpr();
            expect("SYMBOL", "]");
            add_ops("array_read", id);
        }
        else {
            add_ops("r", id);
        }
        expect("SYMBOL", ")");
        expect("SYMBOL", ";");
    }
    else if (tokens[pos].type == "KEYWORD" && tokens[pos].value == "print") {
        match("KEYWORD", "print");
        expect("SYMBOL", "(");
        parseLogExpr();
        expect("SYMBOL", ")");
        expect("SYMBOL", ";");
        add_ops("w", "");
    }
    else if (pos < tokens.size() && tokens[pos].type == "SYMBOL" && tokens[pos].value == "}") {
        return;
    }
    else {
        if (pos >= tokens.size()) throw std::runtime_error("Unexpected end of input in parseStmt");
        throw std::runtime_error("Invalid statement at line " +
            std::to_string(tokens[pos].line) +
            ", position " +
            std::to_string(tokens[pos].pos) +
            " (token: " + tokens[pos].type + " '" + tokens[pos].value + "')");
    }
}

void Parser::parseDeclInt() {
    if (!silent_mode_active) {
        std::cout << "Parsing DeclInt\n";
    }
    std::string id = expect("ID").value;
    if (sym_table.exists(id) || declared_arrays.find(id) != declared_arrays.end()) {
        throw std::runtime_error("Variable or array '" + id + "' already declared at line " +
            std::to_string(tokens[pos - 1].line) + ", position " +
            std::to_string(tokens[pos - 1].pos));
    }
    if (match("SYMBOL", ";")) {
        sym_table.add_variable(id, 0);
        if (!silent_mode_active) {
            std::cout << "Added variable to sym_table: " << id << "\n";
        }
    }
    else if (match("SYMBOL", "=")) {
        parseExpr();
        add_ops("=", id);
        expect("SYMBOL", ";");
        sym_table.add_variable(id, 0);
    }
    else if (match("SYMBOL", "[")) {
        std::string size_val;
        if (pos < tokens.size() && tokens[pos].type == "ID") {
            size_val = expect("ID").value;
            if (!sym_table.exists(size_val)) {
                throw std::runtime_error("Undefined array size variable '" + size_val + "' at line " +
                    std::to_string(tokens[pos - 1].line) + ", position " +
                    std::to_string(tokens[pos - 1].pos));
            }
            add_ops("", size_val);
        }
        else if (pos < tokens.size() && tokens[pos].type == "NUMBER") {
            size_val = expect("NUMBER").value;
            add_ops("", size_val);
        }
        else {
            if (pos >= tokens.size()) throw std::runtime_error("Unexpected EOF while expecting array size");
            throw std::runtime_error("Expected ID or NUMBER for array size at line " +
                std::to_string(tokens[pos].line) + ", position " +
                std::to_string(tokens[pos].pos));
        }
        expect("SYMBOL", "]");
        add_ops("alloc_array", id);
        declared_arrays.insert(id);
        if (match("SYMBOL", "=")) {
            expect("SYMBOL", "{");
            int initializer_count = 0;
            parseInitializers(initializer_count);
            add_ops("init_array", std::to_string(initializer_count));
            expect("SYMBOL", "}");
            expect("SYMBOL", ";");
        }
        else {
            expect("SYMBOL", ";");
        }
    }
    else {
        if (pos >= tokens.size()) throw std::runtime_error("Unexpected EOF after int declaration for " + id);
        throw std::runtime_error("Expected ';', '=', or '[' after identifier '" + id +
            "' at line " + std::to_string(tokens[pos].line) +
            ", position " + std::to_string(tokens[pos].pos));
    }
}

void Parser::parseUseInt() {
    if (!silent_mode_active) {
        std::cout << "Parsing UseInt\n";
    }
    std::string id = expect("ID").value;
    if (!sym_table.exists(id) && declared_arrays.find(id) == declared_arrays.end()) {
        throw std::runtime_error("Undefined variable or array '" + id + "' at line " +
            std::to_string(tokens[pos - 1].line) + ", position " +
            std::to_string(tokens[pos - 1].pos));
    }
    if (match("SYMBOL", "=")) {
        parseExpr();
        add_ops("=", id);
        expect("SYMBOL", ";");
    }
    else if (match("SYMBOL", "[")) {
        parseExpr();
        expect("SYMBOL", "]");
        expect("SYMBOL", "=");
        parseExpr();
        add_ops("array_set", id);
        expect("SYMBOL", ";");
    }
    else {
        if (pos >= tokens.size()) throw std::runtime_error("Unexpected EOF after identifier " + id);
        throw std::runtime_error("Expected '=' or '[' after identifier '" + id +
            "' at line " + std::to_string(tokens[pos].line) +
            ", position " + std::to_string(tokens[pos].pos));
    }
}

void Parser::parseInitializers(int& count) {
    if (!silent_mode_active) {
        std::cout << "Parsing Initializers\n";
    }
    if (pos < tokens.size() && !(tokens[pos].type == "SYMBOL" && tokens[pos].value == "}")) {
        parseFactor();
        count++;
        parseInitCont(count);
    }
}

void Parser::parseInitCont(int& count) {
    if (match("SYMBOL", ",")) {
        if (pos < tokens.size() && !(tokens[pos].type == "SYMBOL" && tokens[pos].value == "}")) {
            parseFactor();
            count++;
            parseInitCont(count);
        }
        else if (pos < tokens.size() && tokens[pos].type == "SYMBOL" && tokens[pos].value == "}") {
        }
        else if (pos >= tokens.size() || (pos < tokens.size() && tokens[pos].type == "SYMBOL" && tokens[pos].value != "}")) {
            throw std::runtime_error("Expected expression or '}' after comma in initializer list at line " +
                std::to_string(tokens[pos - 1].line) + ", position " + std::to_string(tokens[pos - 1].pos));
        }
    }
}

void Parser::parseExpr() {
    if (!silent_mode_active) {
        std::cout << "Parsing Expr\n";
    }
    parseTerm();
    parseExprCont();
}

void Parser::parseExprCont() {
    while (pos < tokens.size() && tokens[pos].type == "SYMBOL" && (tokens[pos].value == "+" || tokens[pos].value == "-")) {
        std::string op_val = tokens[pos].value;
        match("SYMBOL", op_val);
        parseTerm();
        add_ops(op_val);
    }
}

void Parser::parseTerm() {
    parseFactor();
    parseTermCont();
}

void Parser::parseTermCont() {
    while (pos < tokens.size() && tokens[pos].type == "SYMBOL" && (tokens[pos].value == "*" || tokens[pos].value == "/")) {
        std::string op_val = tokens[pos].value;
        match("SYMBOL", op_val);
        parseFactor();
        add_ops(op_val);
    }
}

void Parser::parseFactor() {
    if (!silent_mode_active) {
        std::cout << "Parsing Factor\n";
    }
    if (pos < tokens.size() && tokens[pos].type == "ID") {
        std::string id = expect("ID").value;
        if (!sym_table.exists(id) && declared_arrays.find(id) == declared_arrays.end()) {
            throw std::runtime_error("Undefined variable or array '" + id + "' at line " +
                std::to_string(tokens[pos - 1].line) + ", position " +
                std::to_string(tokens[pos - 1].pos));
        }
        if (match("SYMBOL", "[")) {
            parseExpr();
            expect("SYMBOL", "]");
            add_ops("array_get", id);
        }
        else {
            add_ops("", id);
        }
    }
    else if (pos < tokens.size() && tokens[pos].type == "NUMBER") {
        add_ops("", expect("NUMBER").value);
    }
    else if (match("SYMBOL", "(")) {
        parseLogExpr();
        expect("SYMBOL", ")");
    }
    else if (match("SYMBOL", "~")) {
        parseFactor();
        add_ops("~");
    }
    else {
        if (pos >= tokens.size()) throw std::runtime_error("Unexpected EOF in parseFactor");
        throw std::runtime_error("Invalid factor at line " +
            std::to_string(tokens[pos].line) +
            ", position " + std::to_string(tokens[pos].pos));
    }
}

void Parser::parseLogExpr() {
    if (!silent_mode_active) {
        std::cout << "Parsing LogExpr\n";
    }
    parseLogAndTerm();
    while (pos < tokens.size() && tokens[pos].type == "SYMBOL" && tokens[pos].value == "|") {
        match("SYMBOL", "|");
        parseLogAndTerm();
        add_ops("|");
    }
}

void Parser::parseLogAndTerm() {
    parseLogNotTerm();
    while (pos < tokens.size() && tokens[pos].type == "SYMBOL" && tokens[pos].value == "&") {
        match("SYMBOL", "&");
        parseLogNotTerm();
        add_ops("&");
    }
}

void Parser::parseLogNotTerm() {
    if (match("SYMBOL", "!")) {
        parseLogNotTerm();
        add_ops("!");
    }
    else if (match("SYMBOL", "(")) {
        parseLogExpr();
        expect("SYMBOL", ")");
    }
    else {
        parseExpr();
        if (pos < tokens.size() && (tokens[pos].type == "SYMBOL" && (tokens[pos].value == ">" || tokens[pos].value == "<" || tokens[pos].value == "=="))) {
            std::string op_val = tokens[pos].value;
            match("SYMBOL", op_val);
            parseExpr();
            add_ops(op_val);
        }
    }
}

void Parser::add_ops(const std::string& operation, const std::string& operand) {
    ops_list.emplace_back(operation, operand); // Используем ops_list
    if (!silent_mode_active) {
        std::cout << "Added OPS: " << operation << (operand.empty() ? "" : " " + operand) << "\n";
    }
}

void Parser::push_label(size_t p) {
    label_stack.push(p);
}

size_t Parser::pop_label() {
    if (label_stack.empty()) {
        throw std::runtime_error("Label stack is empty");
    }
    size_t label = label_stack.top();
    label_stack.pop();
    return label;
}

void Parser::set_jump(size_t label_pos, size_t target) {
    if (label_pos < ops_list.size() && label_pos != std::numeric_limits<size_t>::max()) { // Используем ops_list
        if (ops_list[label_pos].operation == "jf" || ops_list[label_pos].operation == "j") {
            ops_list[label_pos].operand = std::to_string(target);
            if (!silent_mode_active) {
                std::cout << "Set jump at " << label_pos << " to " << target << "\n";
            }
        }
        else {
            if (!silent_mode_active) {
                std::cerr << "Warning: Attempted to set_jump on non-jump instruction at " << label_pos << " (op: " << ops_list[label_pos].operation << ")\n";
            }
        }
    }
    else if (label_pos == std::numeric_limits<size_t>::max()) {
        if (!silent_mode_active) {
            std::cout << "Note: Attempted to set_jump on invalid label_pos (max_size_t), possibly no jump needed for an already resolved value.\n";
        }
    }
    else {
        if (!silent_mode_active) {
            std::cerr << "Warning: Attempted to set_jump on out-of-bounds label_pos " << label_pos << "\n";
        }
    }
}