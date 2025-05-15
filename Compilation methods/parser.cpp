#include "parser.h"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <limits>

Parser::Parser(SymbolTable& sym_table) :
    sym_table(sym_table),
    current_token_idx(0),
    silent_mode_active(false),
    current_initializer_count(0),
    is_array_access(false) {
    initialize_grammar_and_table();
}

void Parser::set_silent_mode(bool mode) {
    silent_mode_active = mode;
    sym_table.set_silent_mode(mode);
}

bool Parser::is_variable_declared(const std::string& name) const {
    return sym_table.exists(name) || declared_arrays_set.count(name);
}

std::string Parser::get_current_input_terminal_string(const Token& token) {
    if (token.type == "KEYWORD" || token.type == "SYMBOL") {
        return token.value;
    }
    else if (token.type == "ID") {
        return "ID";
    }
    else if (token.type == "NUMBER") {
        return "NUMBER";
    }
    else if (token.type == "EOF") {
        return "EOF";
    }
    throw std::runtime_error("Unknown token type for input terminal string: " + token.type);
}

bool Parser::match_and_advance(const std::string& expected_terminal_in_rule) {
    if (current_token_idx >= tokens_list.size()) {
        return expected_terminal_in_rule == "EOF";
    }

    const Token& current_token = tokens_list[current_token_idx];
    bool matched = false;

    if (expected_terminal_in_rule == current_token.value && (current_token.type == "KEYWORD" || current_token.type == "SYMBOL")) {
        matched = true;
    }
    else if (expected_terminal_in_rule == "ID" && current_token.type == "ID") {
        id_for_actions = current_token.value;
        number_for_actions.clear();
        matched = true;
    }
    else if (expected_terminal_in_rule == "NUMBER" && current_token.type == "NUMBER") {
        number_for_actions = current_token.value;
        id_for_actions.clear();
        matched = true;
    }
    else if (expected_terminal_in_rule == "IDorNUMBER" && (current_token.type == "ID" || current_token.type == "NUMBER")) {
        if (current_token.type == "ID") {
            id_for_actions = current_token.value;
            number_for_actions.clear();
        }
        else {
            number_for_actions = current_token.value;
            id_for_actions.clear();
        }
        matched = true;
    }
    else if (expected_terminal_in_rule == "EOF" && current_token.type == "EOF") {
        matched = true;
    }

    if (matched) {
        if (!silent_mode_active) {
            std::cout << "Matched and consumed: " << expected_terminal_in_rule << " ('" << current_token.value
                << "'), id_for_actions: '" << id_for_actions << "', number_for_actions: '" << number_for_actions << "'\n";
        }
        current_token_idx++;
        return true;
    }
    return false;
}

void Parser::initialize_grammar_and_table() {
    
    grammar_rules = {
        { "Программа", {"СписокОператоров", "EOF"}, 0 },
        { "СписокОператоров", {"Оператор", "СписокОператоров"}, 1 },
        { "СписокОператоров", {}, 2 },
        { "Оператор", {"int", "ID", "#ACTION_STORE_ID_FOR_LHS", "ХвостОбъявления"}, 3 },
        { "Оператор", {"ID", "#ACTION_STORE_ID_FOR_LHS", "#ACTION_CHECK_VAR_EXISTS", "ХвостИспользования"}, 4 },
        { "Оператор", {"if", "(", "ЛогВыраж", "#ACTION_PROG1", ")", "{", "ПрограммаВнутриБлока", "}", "Альтернатива", "#ACTION_PROG3_IF_END"}, 5 },
        { "Оператор", {"while", "#ACTION_PROG4", "(", "ЛогВыраж", "#ACTION_PROG1", ")", "{", "ПрограммаВнутриБлока", "}", "#ACTION_PROG5"}, 6 },
        { "Оператор", {"read", "(", "ДоступКПеременнойДляRead", ")", ";"}, 7 },
        { "Оператор", {"print", "(", "ЛогВыраж", ")", ";", "#ACTION_PRINT"}, 8 },
        { "ПрограммаВнутриБлока", {"СписокОператоров"}, 9 },
        { "ХвостОбъявления", {";", "#ACTION_DECL_SIMPLE"}, 10 },
        { "ХвостОбъявления", {"=", "АрифмВыраж", ";", "#ACTION_DECL_INIT"}, 11 },
        { "ХвостОбъявления", {"[", "IDorNUMBER", "#ACTION_STORE_SIZE_ID_OR_NUM", "]", "#ACTION_ALLOC_ARRAY", "ХвостИницМассива", ";"}, 12 },
        { "ХвостИницМассива", {"=", "{", "#ACTION_INIT_COUNT_RESET", "СписокИниц", "}", "#ACTION_INIT_ARRAY_WITH_VALUES"}, 13 },
        { "ХвостИницМассива", {}, 14 },
        { "ХвостИспользования", {"=", "АрифмВыраж", ";", "#ACTION_ASSIGN_VAR"}, 15 },
        { "ХвостИспользования", {"[", "АрифмВыраж", "#ACTION_ARRAY_INDEX_FOR_SET", "]", "=", "АрифмВыраж", ";", "#ACTION_ARRAY_SET"}, 16 },
        { "Альтернатива", {"else", "#ACTION_PROG2", "{", "ПрограммаВнутриБлока", "}"}, 17 },
        { "Альтернатива", {}, 18 },
        { "СписокИниц", {"АрифмВыраж", "#ACTION_INIT_COUNT_INC", "СписокИницПрод"}, 19 },
        { "СписокИниц", {}, 20 },
        { "СписокИницПрод", {",", "АрифмВыраж", "#ACTION_INIT_COUNT_INC", "СписокИницПрод"}, 21 },
        { "СписокИницПрод", {}, 22 },
        { "АрифмВыраж", {"Терм", "АрифмВыражПрод"}, 23 },
        { "АрифмВыражПрод", {"+", "Терм", "#ACTION_GEN_PLUS", "АрифмВыражПрод"}, 24 },
        { "АрифмВыражПрод", {"-", "Терм", "#ACTION_GEN_MINUS_BIN", "АрифмВыражПрод"}, 25 },
        { "АрифмВыражПрод", {}, 26 },
        { "Терм", {"Фактор", "ТермПрод"}, 27 },
        { "ТермПрод", {"*", "Фактор", "#ACTION_GEN_MUL", "ТермПрод"}, 28 },
        { "ТермПрод", {"/", "Фактор", "#ACTION_GEN_DIV", "ТермПрод"}, 29 },
        { "ТермПрод", {}, 30 },
        { "Фактор", {"~", "Фактор", "#ACTION_GEN_UNARY_MINUS"}, 31 },
        { "Фактор", {"ПервичноеАрифм"}, 32 },
        { "ПервичноеАрифм", {"ID", "#ACTION_STORE_ID", "ХвостИндекса", "#ACTION_VAR_OR_ARRAY_GET"}, 33 },
        { "ПервичноеАрифм", {"NUMBER", "#ACTION_PROCESS_NUMBER"}, 34 },
        { "ПервичноеАрифм", {"(", "АрифмВыраж", ")"}, 35 },
        { "ХвостИндекса", {"[", "АрифмВыраж", "]", "#ACTION_SET_ARRAY_ACCESS"}, 36 },
        { "ХвостИндекса", {}, 37 },
        { "ЛогВыраж", {"ЛогИЛИ_Терм", "ЛогИЛИ_Прод"}, 38 },
        { "ЛогИЛИ_Прод", {"|", "ЛогИЛИ_Терм", "#ACTION_GEN_OR", "ЛогИЛИ_Прод"}, 39 },
        { "ЛогИЛИ_Прод", {}, 40 },
        { "ЛогИЛИ_Терм", {"ЛогИ_Терм", "ЛогИ_Прод"}, 41 },
        { "ЛогИ_Прод", {"&", "ЛогИ_Терм", "#ACTION_GEN_AND", "ЛогИ_Прод"}, 42 },
        { "ЛогИ_Прод", {}, 43 },
        { "ЛогИ_Терм", {"!", "ЛогИ_Терм", "#ACTION_GEN_NOT"}, 44 },
        { "ЛогИ_Терм", {"СравнениеИлиПервичноеЛог"}, 45 },
        { "СравнениеИлиПервичноеЛог", {"АрифмВыраж", "ХвостСравнения"}, 46 },
        { "СравнениеИлиПервичноеЛог", {"(", "ЛогВыраж", ")"}, 47 },
        { "ХвостСравнения", {"ОператорСравнения", "АрифмВыраж", "#ACTION_GEN_COMPARE_OP"}, 48 },
        { "ХвостСравнения", {}, 49 },
        { "ОператорСравнения", {">", "#ACTION_SET_COMP_OP_GT"}, 50 },
        { "ОператорСравнения", {"<", "#ACTION_SET_COMP_OP_LT"}, 51 },
        { "ОператорСравнения", {"==", "#ACTION_SET_COMP_OP_EQ"}, 52 },
        { "ДоступКПеременнойДляRead", {"ID", "#ACTION_STORE_ID_FOR_LHS", "ХвостИндекса", "#ACTION_READ_VAR_OR_ARRAY"}, 53 }
    };

    ll_parse_table["Программа"]["int"] = 0;
    ll_parse_table["Программа"]["ID"] = 0;
    ll_parse_table["Программа"]["if"] = 0;
    ll_parse_table["Программа"]["while"] = 0;
    ll_parse_table["Программа"]["read"] = 0;
    ll_parse_table["Программа"]["print"] = 0;
    ll_parse_table["Программа"]["EOF"] = 0;

    ll_parse_table["СписокОператоров"]["int"] = 1;
    ll_parse_table["СписокОператоров"]["ID"] = 1;
    ll_parse_table["СписокОператоров"]["if"] = 1;
    ll_parse_table["СписокОператоров"]["while"] = 1;
    ll_parse_table["СписокОператоров"]["read"] = 1;
    ll_parse_table["СписокОператоров"]["print"] = 1;
    ll_parse_table["СписокОператоров"]["EOF"] = 2;
    ll_parse_table["СписокОператоров"]["}"] = 2;

    ll_parse_table["Оператор"]["int"] = 3;
    ll_parse_table["Оператор"]["ID"] = 4;
    ll_parse_table["Оператор"]["if"] = 5;
    ll_parse_table["Оператор"]["while"] = 6;
    ll_parse_table["Оператор"]["read"] = 7;
    ll_parse_table["Оператор"]["print"] = 8;

    ll_parse_table["ПрограммаВнутриБлока"]["int"] = 9;
    ll_parse_table["ПрограммаВнутриБлока"]["ID"] = 9;
    ll_parse_table["ПрограммаВнутриБлока"]["if"] = 9;
    ll_parse_table["ПрограммаВнутриБлока"]["while"] = 9;
    ll_parse_table["ПрограммаВнутриБлока"]["read"] = 9;
    ll_parse_table["ПрограммаВнутриБлока"]["print"] = 9;
    ll_parse_table["ПрограммаВнутриБлока"]["}"] = 9;

    ll_parse_table["ХвостОбъявления"][";"] = 10;
    ll_parse_table["ХвостОбъявления"]["="] = 11;
    ll_parse_table["ХвостОбъявления"]["["] = 12;

    ll_parse_table["ХвостИницМассива"]["="] = 13;
    ll_parse_table["ХвостИницМассива"][";"] = 14;

    ll_parse_table["ХвостИспользования"]["="] = 15;
    ll_parse_table["ХвостИспользования"]["["] = 16;

    ll_parse_table["Альтернатива"]["else"] = 17;
    ll_parse_table["Альтернатива"]["int"] = 18;
    ll_parse_table["Альтернатива"]["ID"] = 18;
    ll_parse_table["Альтернатива"]["if"] = 18;
    ll_parse_table["Альтернатива"]["while"] = 18;
    ll_parse_table["Альтернатива"]["read"] = 18;
    ll_parse_table["Альтернатива"]["print"] = 18;
    ll_parse_table["Альтернатива"]["}"] = 18;
    ll_parse_table["Альтернатива"]["EOF"] = 18;
    ll_parse_table["Альтернатива"][";"] = 18;

    ll_parse_table["СписокИниц"]["ID"] = 19;
    ll_parse_table["СписокИниц"]["NUMBER"] = 19;
    ll_parse_table["СписокИниц"]["("] = 19;
    ll_parse_table["СписокИниц"]["~"] = 19;
    ll_parse_table["СписокИниц"]["}"] = 20;

    ll_parse_table["СписокИницПрод"][","] = 21;
    ll_parse_table["СписокИницПрод"]["}"] = 22;

    ll_parse_table["АрифмВыраж"]["ID"] = 23;
    ll_parse_table["АрифмВыраж"]["NUMBER"] = 23;
    ll_parse_table["АрифмВыраж"]["("] = 23;
    ll_parse_table["АрифмВыраж"]["~"] = 23;

    ll_parse_table["АрифмВыражПрод"]["+"] = 24;
    ll_parse_table["АрифмВыражПрод"]["-"] = 25;
    ll_parse_table["АрифмВыражПрод"][")"] = 26;
    ll_parse_table["АрифмВыражПрод"]["]"] = 26;
    ll_parse_table["АрифмВыражПрод"][";"] = 26;
    ll_parse_table["АрифмВыражПрод"][","] = 26;
    ll_parse_table["АрифмВыражПрод"]["}"] = 26;
    ll_parse_table["АрифмВыражПрод"][">"] = 26;
    ll_parse_table["АрифмВыражПрод"]["<"] = 26;
    ll_parse_table["АрифмВыражПрод"]["=="] = 26;
    ll_parse_table["АрифмВыражПрод"]["&"] = 26;
    ll_parse_table["АрифмВыражПрод"]["|"] = 26;
    ll_parse_table["АрифмВыражПрод"]["EOF"] = 26;

    ll_parse_table["Терм"]["ID"] = 27;
    ll_parse_table["Терм"]["NUMBER"] = 27;
    ll_parse_table["Терм"]["("] = 27;
    ll_parse_table["Терм"]["~"] = 27;

    ll_parse_table["ТермПрод"]["*"] = 28;
    ll_parse_table["ТермПрод"]["/"] = 29;
    ll_parse_table["ТермПрод"]["+"] = 30;
    ll_parse_table["ТермПрод"]["-"] = 30;
    ll_parse_table["ТермПрод"][")"] = 30;
    ll_parse_table["ТермПрод"]["]"] = 30;
    ll_parse_table["ТермПрод"][";"] = 30;
    ll_parse_table["ТермПрод"][","] = 30;
    ll_parse_table["ТермПрод"]["}"] = 30;
    ll_parse_table["ТермПрод"][">"] = 30;
    ll_parse_table["ТермПрод"]["<"] = 30;
    ll_parse_table["ТермПрод"]["=="] = 30;
    ll_parse_table["ТермПрод"]["&"] = 30;
    ll_parse_table["ТермПрод"]["|"] = 30;
    ll_parse_table["ТермПрод"]["EOF"] = 30;

    ll_parse_table["Фактор"]["~"] = 31;
    ll_parse_table["Фактор"]["ID"] = 32;
    ll_parse_table["Фактор"]["NUMBER"] = 32;
    ll_parse_table["Фактор"]["("] = 32;

    ll_parse_table["ПервичноеАрифм"]["ID"] = 33;
    ll_parse_table["ПервичноеАрифм"]["NUMBER"] = 34;
    ll_parse_table["ПервичноеАрифм"]["("] = 35;

    ll_parse_table["ХвостИндекса"]["["] = 36;
    ll_parse_table["ХвостИндекса"]["*"] = 37;
    ll_parse_table["ХвостИндекса"]["/"] = 37;
    ll_parse_table["ХвостИндекса"]["+"] = 37;
    ll_parse_table["ХвостИндекса"]["-"] = 37;
    ll_parse_table["ХвостИндекса"][")"] = 37;
    ll_parse_table["ХвостИндекса"]["]"] = 37;
    ll_parse_table["ХвостИндекса"][";"] = 37;
    ll_parse_table["ХвостИндекса"][","] = 37;
    ll_parse_table["ХвостИндекса"]["}"] = 37;
    ll_parse_table["ХвостИндекса"][">"] = 37;
    ll_parse_table["ХвостИндекса"]["<"] = 37;
    ll_parse_table["ХвостИндекса"]["=="] = 37;
    ll_parse_table["ХвостИндекса"]["&"] = 37;
    ll_parse_table["ХвостИндекса"]["|"] = 37;
    ll_parse_table["ХвостИндекса"]["EOF"] = 37;

    ll_parse_table["ЛогВыраж"]["!"] = 38;
    ll_parse_table["ЛогВыраж"]["("] = 38;
    ll_parse_table["ЛогВыраж"]["ID"] = 38;
    ll_parse_table["ЛогВыраж"]["NUMBER"] = 38;
    ll_parse_table["ЛогВыраж"]["~"] = 38;

    ll_parse_table["ЛогИЛИ_Прод"]["|"] = 39;
    ll_parse_table["ЛогИЛИ_Прод"][")"] = 40;
    ll_parse_table["ЛогИЛИ_Прод"][";"] = 40;
    ll_parse_table["ЛогИЛИ_Прод"]["EOF"] = 40;
    ll_parse_table["ЛогИЛИ_Прод"]["+"] = 40;
    ll_parse_table["ЛогИЛИ_Прод"]["-"] = 40;

    ll_parse_table["ЛогИЛИ_Терм"]["!"] = 41;
    ll_parse_table["ЛогИЛИ_Терм"]["("] = 41;
    ll_parse_table["ЛогИЛИ_Терм"]["ID"] = 41;
    ll_parse_table["ЛогИЛИ_Терм"]["NUMBER"] = 41;
    ll_parse_table["ЛогИЛИ_Терм"]["~"] = 41;

    ll_parse_table["ЛогИ_Прод"]["&"] = 42;
    ll_parse_table["ЛогИ_Прод"]["|"] = 43;
    ll_parse_table["ЛогИ_Прод"][")"] = 43;
    ll_parse_table["ЛогИ_Прод"][";"] = 43;
    ll_parse_table["ЛогИ_Прод"]["EOF"] = 43;

    ll_parse_table["ЛогИ_Прод"]["+"] = 43;
    ll_parse_table["ЛогИ_Прод"]["-"] = 43;
    ll_parse_table["ЛогИ_Прод"]["|"] = 43;
    ll_parse_table["ЛогИ_Прод"][")"] = 43;
    ll_parse_table["ЛогИ_Прод"][";"] = 43;
    ll_parse_table["ЛогИ_Прод"]["EOF"] = 43;

    ll_parse_table["ЛогИ_Терм"]["!"] = 44;
    ll_parse_table["ЛогИ_Терм"]["("] = 45;
    ll_parse_table["ЛогИ_Терм"]["ID"] = 45;
    ll_parse_table["ЛогИ_Терм"]["NUMBER"] = 45;
    ll_parse_table["ЛогИ_Терм"]["~"] = 45;

    ll_parse_table["СравнениеИлиПервичноеЛог"]["("] = 47;
    ll_parse_table["СравнениеИлиПервичноеЛог"]["ID"] = 46;
    ll_parse_table["СравнениеИлиПервичноеЛог"]["NUMBER"] = 46;
    ll_parse_table["СравнениеИлиПервичноеЛог"]["~"] = 46;

    ll_parse_table["ХвостСравнения"][">"] = 48;
    ll_parse_table["ХвостСравнения"]["<"] = 48;
    ll_parse_table["ХвостСравнения"]["=="] = 48;
    ll_parse_table["ХвостСравнения"]["&"] = 49;
    ll_parse_table["ХвостСравнения"]["|"] = 49;
    ll_parse_table["ХвостСравнения"][")"] = 49;
    ll_parse_table["ХвостСравнения"][";"] = 49;
    ll_parse_table["ХвостСравнения"]["+"] = 49;
    ll_parse_table["ХвостСравнения"]["-"] = 49;
    ll_parse_table["ХвостСравнения"]["EOF"] = 49;

    ll_parse_table["ОператорСравнения"][">"] = 50;
    ll_parse_table["ОператорСравнения"]["<"] = 51;
    ll_parse_table["ОператорСравнения"]["=="] = 52;

    ll_parse_table["ДоступКПеременнойДляRead"]["ID"] = 53;

    if (!silent_mode_active) {
        std::cout << "Parse table initialized.\n";
    }
}

std::vector<OPS> Parser::parse(const std::vector<Token>& tokens) {
    tokens_list = tokens;
    current_token_idx = 0;
    ops_list.clear();
    while (!parse_stack.empty()) parse_stack.pop();
    while (!label_stack.empty()) label_stack.pop();
    declared_arrays_set.clear();
    id_for_actions.clear(); number_for_actions.clear(); id_for_lhs.clear(); stored_comparison_operator.clear();
    current_initializer_count = 0; is_array_access = false;

    parse_stack.push("EOF");
    parse_stack.push("Программа");

    while (!parse_stack.empty()) {
        std::string top = parse_stack.top();
        Token curr = (current_token_idx < tokens_list.size()) ? tokens_list[current_token_idx] : Token("EOF", "", 0, 0);
        std::string term = get_current_input_terminal_string(curr);
        if (!silent_mode_active) std::cout << "Stack: " << top << ", Token: " << term << " ('" << curr.value << "')\n";
        if (top.rfind("#ACTION", 0) == 0) {
            parse_stack.pop();
            execute_action(top);
        }
        else if (!ll_parse_table.count(top)) {
            parse_stack.pop();
            if (!match_and_advance(top)) throw std::runtime_error("Unexpected '" + curr.value + "'");
        }
        else {
            auto& row = ll_parse_table[top];
            if (!row.count(term)) throw std::runtime_error("No rule for '" + top + "' and '" + term + "'");
            int idx = row[term];
            auto& rule = grammar_rules[idx];
            parse_stack.pop();
            if (!silent_mode_active) { std::cout << "Apply rule " << rule.id << "\n"; }
            for (auto it = rule.rhs.rbegin(); it != rule.rhs.rend(); ++it) if (!it->empty()) parse_stack.push(*it);
        }
    }
    if (current_token_idx < tokens_list.size() && tokens_list[current_token_idx].type != "EOF")
        throw std::runtime_error("Extra token '" + tokens_list[current_token_idx].value + "'");
    if (!silent_mode_active) std::cout << "Parsing done. OPS count=" << ops_list.size() << "\n";
    return ops_list;
}

void Parser::execute_action(const std::string& action_symbol) {

    if (!silent_mode_active) std::cout << "Action: " << action_symbol << " id='" << id_for_actions << "' num='" << number_for_actions << "' arr='" << is_array_access << "'\n";
   
    if (action_symbol == "#ACTION_STORE_ID") {
        // ID в выражениях
        if (id_for_actions.empty()) throw std::runtime_error("No ID for STORE_ID");
        // если сразу после ID скобка — сохраняем имя массива
        if (current_token_idx < tokens_list.size() && tokens_list[current_token_idx].value == "[") {
            saved_array_id = id_for_actions;
            id_for_actions.clear();
        }
    }
    else if (action_symbol == "#ACTION_STORE_ID_FOR_LHS") {
        // ID в левой части объявления/присваивания (в том числе read)
        if (id_for_actions.empty()) throw std::runtime_error("No ID for STORE_ID_FOR_LHS");
        // тоже проверяем скобку и сохраняем массив
        if (current_token_idx < tokens_list.size() && tokens_list[current_token_idx].value == "[") {
            saved_array_id = id_for_actions;
            // не чистим id_for_actions — он понадобится в id_for_lhs
        }
        id_for_lhs = id_for_actions;
    }

    else if (action_symbol == "#ACTION_CHECK_VAR_EXISTS") {
        if (!is_variable_declared(id_for_lhs)) {
            throw std::runtime_error("Undeclared variable or array '" + id_for_lhs + "' at line " +
                (current_token_idx > 0 ? std::to_string(tokens_list[current_token_idx - 1].line) : "unknown"));
        }
    }
    else if (action_symbol == "#ACTION_PROCESS_NUMBER") {
        if (number_for_actions.empty()) {
            throw std::runtime_error("No number stored for #ACTION_PROCESS_NUMBER at line " +
                (current_token_idx > 0 ? std::to_string(tokens_list[current_token_idx - 1].line) : "unknown"));
        }
        add_ops_instruction("", number_for_actions);
        number_for_actions.clear();
    }
    else if (action_symbol == "#ACTION_GEN_PLUS") {
        add_ops_instruction("+");
    }
    else if (action_symbol == "#ACTION_GEN_MINUS_BIN") {
        add_ops_instruction("-");
    }
    else if (action_symbol == "#ACTION_GEN_MUL") {
        add_ops_instruction("*");
    }
    else if (action_symbol == "#ACTION_GEN_DIV") {
        add_ops_instruction("/");
    }
    else if (action_symbol == "#ACTION_GEN_UNARY_MINUS") {
        add_ops_instruction("~");
    }
    else if (action_symbol == "#ACTION_SET_COMP_OP_GT") {
        stored_comparison_operator = ">";
    }
    else if (action_symbol == "#ACTION_SET_COMP_OP_LT") {
        stored_comparison_operator = "<";
    }
    else if (action_symbol == "#ACTION_SET_COMP_OP_EQ") {
        stored_comparison_operator = "==";
    }
    else if (action_symbol == "#ACTION_GEN_COMPARE_OP") {
        if (stored_comparison_operator.empty()) {
            throw std::runtime_error("ACTION_GEN_COMPARE_OP called without a comparison operator set");
        }
        add_ops_instruction(stored_comparison_operator);
        stored_comparison_operator.clear();
    }
    else if (action_symbol == "#ACTION_GEN_AND") {
        add_ops_instruction("&");
    }
    else if (action_symbol == "#ACTION_GEN_OR") {
        add_ops_instruction("|");
    }
    else if (action_symbol == "#ACTION_GEN_NOT") {
        add_ops_instruction("!");
    }
    else if (action_symbol == "#ACTION_ASSIGN_VAR") {
        if (!is_variable_declared(id_for_lhs)) {
            throw std::runtime_error("Undeclared variable '" + id_for_lhs + "' in assignment");
        }
        add_ops_instruction("=", id_for_lhs);
        id_for_lhs.clear();
        is_array_access = false;
    }
    else if (action_symbol == "#ACTION_STORE_SIZE_ID_OR_NUM") {
        if (!id_for_actions.empty()) {
            if (!is_variable_declared(id_for_actions)) {
                throw std::runtime_error("Array size variable '" + id_for_actions + "' not declared");
            }
            add_ops_instruction("", id_for_actions);
            id_for_actions.clear();
        }
        else if (!number_for_actions.empty()) {
            add_ops_instruction("", number_for_actions);
            number_for_actions.clear();
        }
        else {
            throw std::runtime_error("No ID or NUMBER stored for array size for '" + id_for_lhs + "'");
        }
    }
    else if (action_symbol == "#ACTION_ALLOC_ARRAY") {
        if (is_variable_declared(id_for_lhs)) {
            throw std::runtime_error("Array or variable '" + id_for_lhs + "' already declared");
        }
        add_ops_instruction("alloc_array", id_for_lhs);
        declared_arrays_set.insert(id_for_lhs);
        id_for_lhs.clear();
    }
    else if (action_symbol == "#ACTION_SET_ARRAY_ACCESS") {
        if (saved_array_id.empty()) {
            throw std::runtime_error("No array identifier for #ACTION_SET_ARRAY_ACCESS at line " +
                (current_token_idx > 0 ? std::to_string(tokens_list[current_token_idx - 1].line) : "unknown"));
        }
        is_array_access = true;
        id_for_actions = saved_array_id;
        saved_array_id.clear();
    }

    else if (action_symbol == "#ACTION_VAR_OR_ARRAY_GET") {
        if (id_for_actions.empty()) {
            throw std::runtime_error("No ID for VAR_OR_ARRAY_GET");
        }
        if (!is_variable_declared(id_for_actions)) {
            throw std::runtime_error("Undeclared variable or array '" + id_for_actions + "'");
        }
        if (declared_arrays_set.count(id_for_actions) && is_array_access) {
            add_ops_instruction("array_get", id_for_actions);
        }
        else {
            add_ops_instruction("", id_for_actions);
        }
        // Reset
        id_for_actions.clear();
        is_array_access = false;
    }

    else if (action_symbol == "#ACTION_ARRAY_INDEX_FOR_SET") {
        is_array_access = true;
    }
    else if (action_symbol == "#ACTION_ARRAY_SET") {
        if (!is_variable_declared(id_for_lhs)) {
            throw std::runtime_error("Undeclared array '" + id_for_lhs + "' in array set operation");
        }
        add_ops_instruction("array_set", id_for_lhs);
        id_for_lhs.clear();
        is_array_access = false;
    }
    else if (action_symbol == "#ACTION_READ_VAR_OR_ARRAY") {
        if (!is_variable_declared(id_for_lhs)) {
            throw std::runtime_error("Undeclared variable or array '" + id_for_lhs + "' in read operation");
        }
        if (declared_arrays_set.count(id_for_lhs) && is_array_access) {
            add_ops_instruction("array_read", id_for_lhs);
        }
        else {
            add_ops_instruction("r", id_for_lhs);
        }
        id_for_lhs.clear();
        is_array_access = false;
    }
    else if (action_symbol == "#ACTION_PRINT") {
        add_ops_instruction("w");
    }
    else if (action_symbol == "#ACTION_DECL_SIMPLE") {
        if (is_variable_declared(id_for_lhs)) {
            throw std::runtime_error("Variable or array '" + id_for_lhs + "' already exists");
        }
        sym_table.add_variable(id_for_lhs, 0);
        id_for_lhs.clear();
    }
    else if (action_symbol == "#ACTION_DECL_INIT") {
        if (is_variable_declared(id_for_lhs)) {
            throw std::runtime_error("Variable or array '" + id_for_lhs + "' already exists");
        }
        sym_table.add_variable(id_for_lhs, 0);
        add_ops_instruction("=", id_for_lhs);
        id_for_lhs.clear();
    }
    else if (action_symbol == "#ACTION_INIT_COUNT_RESET") {
        current_initializer_count = 0;
    }
    else if (action_symbol == "#ACTION_INIT_COUNT_INC") {
        current_initializer_count++;
    }
    else if (action_symbol == "#ACTION_INIT_ARRAY_WITH_VALUES") {
        add_ops_instruction("init_array", std::to_string(current_initializer_count));
    }
    else if (action_symbol == "#ACTION_PROG1") {
        push_label_ops_stack(ops_list.size());
        add_ops_instruction("jf", "");
    }
    else if (action_symbol == "#ACTION_PROG2") {
        if (label_stack.empty()) {
            throw std::runtime_error("Label stack underflow in PROG2");
        }
        size_t jf_target_pos = pop_label_ops_stack();
        push_label_ops_stack(ops_list.size());
        add_ops_instruction("j", "");
        set_jump_target(jf_target_pos, ops_list.size());
    }
    else if (action_symbol == "#ACTION_PROG3_IF_END") {
        if (!label_stack.empty()) {
            size_t jump_target_pos = pop_label_ops_stack();
            set_jump_target(jump_target_pos, ops_list.size());
        }
    }
    else if (action_symbol == "#ACTION_PROG4") {
        push_label_ops_stack(ops_list.size());
    }
    else if (action_symbol == "#ACTION_PROG5") {
        if (label_stack.size() < 2) {
            throw std::runtime_error("Label stack underflow in PROG5");
        }
        size_t jf_target_pos = pop_label_ops_stack();
        size_t loop_start_pos = pop_label_ops_stack();
        add_ops_instruction("j", std::to_string(loop_start_pos));
        set_jump_target(jf_target_pos, ops_list.size());
    }
    else {
        throw std::runtime_error("Unknown semantic action: " + action_symbol);
    }
}

void Parser::add_ops_instruction(const std::string& op, const std::string& arg) {
    if (op.empty() && arg.empty()) throw std::runtime_error("Empty OPS");
    if (!arg.empty() && !isdigit(arg[0]) && op != "alloc_array" && op != "init_array" && !is_variable_declared(arg))
        throw std::runtime_error("Undeclared in OPS '" + arg + "'");
    ops_list.emplace_back(op, arg);
    if (!silent_mode_active) std::cout << "Added OPS: " << op << " " << arg << "\n";
}

void Parser::push_label_ops_stack(size_t p) { label_stack.push(p); }
size_t Parser::pop_label_ops_stack() {
    if (label_stack.empty()) throw std::runtime_error("Label stack empty");
    size_t v = label_stack.top(); label_stack.pop(); return v;
}
void Parser::set_jump_target(size_t p, size_t t) {
    if (p >= ops_list.size()) throw std::runtime_error("Invalid jump label");
    if (ops_list[p].operation != "jf" && ops_list[p].operation != "j")
        throw std::runtime_error("Not jump instruction at " + std::to_string(p));
    ops_list[p].operand = std::to_string(t);
    if (!silent_mode_active) std::cout << "Set target " << p << "->" << t << "\n";
}
