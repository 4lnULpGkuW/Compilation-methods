#include "parser.h"
#include <stdexcept>
#include <iostream>
#include <set>

Parser::Parser(SymbolTable& sym_table) : sym_table(sym_table), pos(0) {}

std::vector<OPS> Parser::parse(const std::vector<Token>& tokens) {
    if (tokens.empty()) {
        std::cerr << "Error: Token list is empty\n";
        return std::vector<OPS>();
    }
    this->tokens = tokens;
    pos = 0;
    ops.clear();
    read_ops.clear();
    label_stack = std::stack<size_t>();
    declared_arrays.clear();
    std::vector<std::string> errors;
    std::cout << "Starting parse with " << tokens.size() << " tokens\n";
    while (pos < tokens.size() && tokens[pos].type != "EOF") {
        try {
            parseStmt();
            if (!read_ops.empty()) {
                ops.insert(ops.end(), read_ops.begin(), read_ops.end());
                read_ops.clear();
            }
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
            if (pos < tokens.size() && tokens[pos].type == "SYMBOL") {
                pos++; // Advance past ; or }
            }
            if (pos < tokens.size() && tokens[pos].type == "KEYWORD" && tokens[pos].value == "else") {
                pos++; // Skip 'else'
                if (pos < tokens.size() && tokens[pos].type == "SYMBOL" && tokens[pos].value == "{") {
                    pos++; // Skip '{'
                    parseProgram();
                    if (pos < tokens.size() && tokens[pos].type == "SYMBOL" && tokens[pos].value == "}") {
                        pos++; // Skip '}'
                    }
                }
            }
        }
    }
    if (errors.empty()) {
        std::cout << "OPS generated successfully (" << ops.size() << " operations):\n";
        for (size_t i = 0; i < ops.size(); ++i) {
            std::cout << i << ": " << ops[i].operation << (ops[i].operand.empty() ? "" : " " + ops[i].operand) << "\n";
        }
    }
    else {
        std::cerr << "Parsing failed with " << errors.size() << " errors\n";
    }
    return ops;
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
    std::cout << "Expected " << type << (value.empty() ? "" : " '" + value + "'") << ", got " << tokens[pos].value << "\n";
    return tokens[pos++];
}

bool Parser::match(const std::string& type, const std::string& value) {
    if (pos < tokens.size() && tokens[pos].type == type && (value.empty() || tokens[pos].value == value)) {
        std::cout << "Matched " << type << (value.empty() ? "" : " '" + value + "'") << ": " << tokens[pos].value << "\n";
        pos++;
        return true;
    }
    return false;
}

void Parser::parseProgram() {
    std::cout << "Parsing Program at pos " << pos << "\n";
    while (pos < tokens.size() && tokens[pos].type != "EOF" && !(tokens[pos].type == "SYMBOL" && tokens[pos].value == "}")) {
        parseStmt();
    }
}

void Parser::parseStmt() {
    std::cout << "Parsing Stmt at pos " << pos;
    if (pos < tokens.size()) {
        std::cout << " (token: " << tokens[pos].type << " '" << tokens[pos].value << "')\n";
    }
    else {
        std::cout << " (end of input)\n";
    }
    if (pos >= tokens.size() || (tokens[pos].type == "SYMBOL" && tokens[pos].value == "}")) {
        pos++;
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
        size_t jf_pos = parseLogExpr();
        expect("SYMBOL", ")");
        push_label(jf_pos);
        expect("SYMBOL", "{");
        parseProgram();
        expect("SYMBOL", "}");
        size_t label_if_end = ops.size();
        add_ops("j", ""); // Placeholder for else or end
        size_t j_pos = ops.size() - 1;
        set_jump(jf_pos, label_if_end); // jf to skip if block
        if (pos < tokens.size() && tokens[pos].type == "KEYWORD" && tokens[pos].value == "else") {
            match("KEYWORD", "else");
            expect("SYMBOL", "{");
            set_jump(j_pos, ops.size()); // Jump to else block
            parseProgram();
            expect("SYMBOL", "}");
        }
        else {
            set_jump(j_pos, ops.size()); // Jump to end if no else
        }
    }
    else if (tokens[pos].type == "KEYWORD" && tokens[pos].value == "while") {
        match("KEYWORD", "while");
        size_t loop_start = ops.size();
        expect("SYMBOL", "(");
        size_t jf_pos = parseLogExpr();
        expect("SYMBOL", ")");
        expect("SYMBOL", "{");
        parseProgram();
        expect("SYMBOL", "}");
        add_ops("j", std::to_string(loop_start));
        size_t loop_end = ops.size();
        set_jump(jf_pos, loop_end); // jf to after the loop's j instruction
    }
    else if (tokens[pos].type == "KEYWORD" && tokens[pos].value == "read") {
        match("KEYWORD", "read");
        expect("SYMBOL", "(");
        std::string id = expect("ID").value;
        if (!sym_table.exists(id) && declared_arrays.find(id) == declared_arrays.end()) {
            throw std::runtime_error("Undefined variable or array '" + id + "' at line " +
                std::to_string(tokens[pos].line) + ", position " +
                std::to_string(tokens[pos].pos));
        }
        if (match("SYMBOL", "[")) {
            parseExpr();
            ops.emplace_back("array_read", id);
            expect("SYMBOL", "]");
        }
        else {
            read_ops.emplace_back("r", id);
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
    else {
        throw std::runtime_error("Invalid statement at line " +
            std::to_string(pos < tokens.size() ? tokens[pos].line : 0) +
            ", position " +
            std::to_string(pos < tokens.size() ? tokens[pos].pos : 0));
    }
}

void Parser::parseDeclInt() {
    std::cout << "Parsing DeclInt\n";
    std::string id = expect("ID").value;
    if (sym_table.exists(id) || declared_arrays.find(id) != declared_arrays.end()) {
        throw std::runtime_error("Variable or array '" + id + "' already declared at line " +
            std::to_string(tokens[pos - 1].line) + ", position " +
            std::to_string(tokens[pos - 1].pos));
    }
    if (match("SYMBOL", ";")) {
        sym_table.add_variable(id, 0);
        std::cout << "Added variable to sym_table: " << id << "\n";
    }
    else if (match("SYMBOL", "=")) {
        parseExpr();
        add_ops("=", id);
        expect("SYMBOL", ";");
        sym_table.add_variable(id, 0);
    }
    else if (match("SYMBOL", "[")) {
        std::string size_id;
        if (pos < tokens.size() && tokens[pos].type == "ID") {
            size_id = expect("ID").value;
            if (!sym_table.exists(size_id)) {
                throw std::runtime_error("Undefined array size variable '" + size_id + "' at line " +
                    std::to_string(tokens[pos - 1].line) + ", position " +
                    std::to_string(tokens[pos - 1].pos));
            }
            ops.emplace_back("", size_id); // Use existing value of size_id (e.g., n)
        }
        else if (pos < tokens.size() && tokens[pos].type == "NUMBER") {
            ops.emplace_back("", expect("NUMBER").value);
        }
        else {
            throw std::runtime_error("Expected ID or NUMBER for array size at line " +
                std::to_string(tokens[pos].line) + ", position " +
                std::to_string(tokens[pos].pos));
        }
        expect("SYMBOL", "]");
        ops.emplace_back("alloc_array", id);
        declared_arrays.insert(id);
        if (match("SYMBOL", "=")) {
            expect("SYMBOL", "{");
            parseInitializers();
            expect("SYMBOL", "}");
            expect("SYMBOL", ";");
        }
        else {
            expect("SYMBOL", ";");
        }
    }
    else {
        throw std::runtime_error("Expected ';', '=', or '[' after identifier '" + id +
            "' at line " + std::to_string(tokens[pos].line) +
            ", position " + std::to_string(tokens[pos].pos));
    }
}

void Parser::parseUseInt() {
    std::cout << "Parsing UseInt\n";
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
        throw std::runtime_error("Expected '=' or '[' after identifier '" + id +
            "' at line " + std::to_string(tokens[pos].line) +
            ", position " + std::to_string(tokens[pos].pos));
    }
}

void Parser::parseAlternative() {
    if (pos < tokens.size() && tokens[pos].type == "KEYWORD" && tokens[pos].value == "else") {
        match("KEYWORD", "else");
        expect("SYMBOL", "{");
        parseProgram();
        expect("SYMBOL", "}");
    }
}

void Parser::parseInitializers() {
    if (pos < tokens.size() && (tokens[pos].type == "NUMBER" || tokens[pos].type == "ID")) {
        parseFactor();
        parseInitCont();
    }
}

void Parser::parseInitCont() {
    if (match("SYMBOL", ",")) {
        parseFactor();
        parseInitCont();
    }
}

void Parser::parseExpr() {
    std::cout << "Parsing Expr\n";
    parseTerm();
    parseExprCont();
}

void Parser::parseExprCont() {
    if (match("SYMBOL", "+")) {
        parseTerm();
        add_ops("+");
        parseExprCont();
    }
    else if (match("SYMBOL", "-")) {
        parseTerm();
        add_ops("-");
        parseExprCont();
    }
}

void Parser::parseTerm() {
    parseFactor();
    parseTermCont();
}

void Parser::parseTermCont() {
    if (match("SYMBOL", "*")) {
        parseFactor();
        add_ops("*");
        parseTermCont();
    }
    else if (match("SYMBOL", "/")) {
        parseFactor();
        add_ops("/");
        parseTermCont();
    }
}

void Parser::parseFactor() {
    std::cout << "Parsing Factor\n";
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
        parseExpr();
        expect("SYMBOL", ")");
    }
    else if (match("SYMBOL", "~")) {
        parseFactor();
        add_ops("~");
    }
    else {
        throw std::runtime_error("Invalid factor at line " +
            std::to_string(pos < tokens.size() ? tokens[pos].line : 0) +
            ", position " +
            std::to_string(pos < tokens.size() ? tokens[pos].pos : 0));
    }
}

size_t Parser::parseLogExpr() {
    std::cout << "Parsing LogExpr\n";
    if (match("SYMBOL", "!")) {
        size_t jf_pos = parseLogExpr();
        add_ops("!");
        return jf_pos;
    }
    else if (match("SYMBOL", "(")) {
        size_t jf_pos = parseLogExpr();
        expect("SYMBOL", ")");
        parseLogCont();
        return jf_pos;
    }
    else {
        std::vector<OPS> left_ops;
        size_t start_pos = ops.size();
        parseExpr();
        left_ops.assign(ops.begin() + start_pos, ops.end());
        ops.erase(ops.begin() + start_pos, ops.end());
        return parseLogCmp(left_ops);
    }
}

size_t Parser::parseLogCmp(std::vector<OPS>& left_ops) {
    std::cout << "Parsing LogCmp\n";
    if (match("SYMBOL", ">")) {
        std::vector<OPS> right_ops;
        size_t start_pos = ops.size();
        parseExpr();
        right_ops.assign(ops.begin() + start_pos, ops.end());
        ops.erase(ops.begin() + start_pos, ops.end());
        ops.insert(ops.end(), left_ops.begin(), left_ops.end());
        ops.insert(ops.end(), right_ops.begin(), right_ops.end());
        add_ops(">");
        add_ops("jf", "");
        size_t jf_pos = ops.size() - 1;
        parseLogCont();
        return jf_pos;
    }
    else if (match("SYMBOL", "<")) {
        std::vector<OPS> right_ops;
        size_t start_pos = ops.size();
        parseExpr();
        right_ops.assign(ops.begin() + start_pos, ops.end());
        ops.erase(ops.begin() + start_pos, ops.end());
        ops.insert(ops.end(), right_ops.begin(), right_ops.end());
        ops.insert(ops.end(), left_ops.begin(), left_ops.end());
        add_ops(">");
        add_ops("jf", "");
        size_t jf_pos = ops.size() - 1;
        parseLogCont();
        return jf_pos;
    }
    else if (match("SYMBOL", "==")) {
        throw std::runtime_error("Equality operator '==' not supported by interpreter at line " +
            std::to_string(tokens[pos].line) + ", position " +
            std::to_string(tokens[pos].pos));
    }
    else {
        ops.insert(ops.end(), left_ops.begin(), left_ops.end());
        parseLogCont();
        return std::numeric_limits<size_t>::max();
    }
}

void Parser::parseLogCont() {
    if (match("SYMBOL", "&")) {
        parseLogExpr();
        add_ops("and");
        parseLogCont();
    }
    else if (match("SYMBOL", "|")) {
        parseLogExpr();
        add_ops("or");
        parseLogCont();
    }
}

void Parser::add_ops(const std::string& operation, const std::string& operand) {
    ops.emplace_back(operation, operand);
    std::cout << "Added OPS: " << operation << (operand.empty() ? "" : " " + operand) << "\n";
}

void Parser::push_label(size_t pos) {
    label_stack.push(pos);
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
    if (label_pos < ops.size() && label_pos != std::numeric_limits<size_t>::max()) {
        ops[label_pos].operand = std::to_string(target);
        std::cout << "Set jump at " << label_pos << " to " << target << "\n";
    }
}