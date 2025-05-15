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
}

std::vector<OPS> Parser::parse(const std::vector<Token>& tokens) {
    if (tokens.empty()) {
        throw std::runtime_error("Syntax error at line 1, position 1: empty token list");
    }
    tokens_list = tokens;
    current_token_idx = 0;
    ops_list.clear();
    while (!parse_stack.empty()) parse_stack.pop();
    while (!label_stack.empty()) label_stack.pop();
    declared_arrays_set.clear();
    id_for_actions.clear();
    number_for_actions.clear();
    id_for_lhs.clear();
    stored_comparison_operator.clear();
    saved_array_id.clear();
    current_initializer_count = 0;
    is_array_access = false;

    parse_stack.push("EOF");
    parse_stack.push("Программа");

    if (!silent_mode_active) {
        std::cout << "Starting LL(1) parsing with " << tokens_list.size() << " tokens\n";
    }

    while (!parse_stack.empty()) {
        std::string stack_top_symbol = parse_stack.top();
        Token current_token = (current_token_idx < tokens_list.size()) ? tokens_list[current_token_idx] : Token("EOF", "", 0, 0);
        std::string current_input_terminal_str = get_current_input_terminal_string(current_token);

        if (!silent_mode_active) {
            std::cout << "Stack top: " << stack_top_symbol << ", Current token: " << current_input_terminal_str << " ('" << current_token.value << "')\n";
        }

        if (stack_top_symbol.rfind("#ACTION", 0) == 0) {
            parse_stack.pop();
            execute_action(stack_top_symbol);
        }
        else if (ll_parse_table.find(stack_top_symbol) == ll_parse_table.end()) {
            parse_stack.pop();
            if (!match_and_advance(stack_top_symbol)) {
                std::stringstream ss;
                ss << "Syntax error at line " << current_token.line << ", position " << current_token.pos
                    << ": expected '" << stack_top_symbol << "' but found '" << current_token.value << "'";
                throw std::runtime_error(ss.str());
            }
        }
        else {
            auto& map_for_nonterminal = ll_parse_table[stack_top_symbol];
            if (map_for_nonterminal.find(current_input_terminal_str) != map_for_nonterminal.end()) {
                int rule_idx = map_for_nonterminal[current_input_terminal_str];
                const auto& rule = grammar_rules[rule_idx];
                parse_stack.pop();

                if (!silent_mode_active) {
                    std::cout << "Applying rule " << rule.id << ": " << rule.lhs << " -> ";
                    for (const auto& sym : rule.rhs) std::cout << sym << " ";
                    std::cout << "\n";
                }

                for (auto it = rule.rhs.rbegin(); it != rule.rhs.rend(); ++it) {
                    if (!it->empty()) {
                        parse_stack.push(*it);
                    }
                }
            }
            else {
                std::string expected_terminals_msg = " Expected: ";
                for (const auto& pair_item : map_for_nonterminal) {
                    expected_terminals_msg += pair_item.first + " ";
                }
                std::stringstream ss;
                ss << "Syntax error at line " << current_token.line << ", position " << current_token.pos
                    << ": no rule for nonterminal '" << stack_top_symbol << "' and token '" << current_token.value << "'."
                    << expected_terminals_msg;
                throw std::runtime_error(ss.str());
            }
        }
    }

    if (current_token_idx < tokens_list.size() && tokens_list[current_token_idx].type != "EOF") {
        std::stringstream ss;
        ss << "Syntax error at line " << tokens_list[current_token_idx].line << ", position " << tokens_list[current_token_idx].pos
            << ": unexpected token '" << tokens_list[current_token_idx].value << "'";
        throw std::runtime_error(ss.str());
    }

    if (!silent_mode_active) {
        std::cout << "Parsing completed successfully.\n";
        std::cout << "OPS generated successfully (" << ops_list.size() << " operations):\n";
        for (size_t i = 0; i < ops_list.size(); ++i) {
            std::cout << i << ": " << ops_list[i].operation << (ops_list[i].operand.empty() ? "" : " " + ops_list[i].operand) << "\n";
        }
    }

    return ops_list;
}

void Parser::execute_action(const std::string& action_symbol) {
    // Получаем текущий или предыдущий токен для указания строки и позиции в ошибках
    const Token& token = (current_token_idx > 0 && current_token_idx <= tokens_list.size())
        ? tokens_list[current_token_idx - 1]
        : (current_token_idx < tokens_list.size() ? tokens_list[current_token_idx] : Token("EOF", "", 0, 0));

    if (!silent_mode_active) {
        std::cout << "Executing action: " << action_symbol << ", id_for_actions: '" << id_for_actions
            << "', number_for_actions: '" << number_for_actions << "', is_array_access: '" << is_array_access
            << "', saved_array_id: '" << saved_array_id << "'\n";
    }

    if (action_symbol == "#ACTION_STORE_ID") {
        if (id_for_actions.empty()) {
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": no identifier provided for variable or array access";
            throw std::runtime_error(ss.str());
        }
        if (current_token_idx < tokens_list.size() && tokens_list[current_token_idx].value == "[") {
            saved_array_id = id_for_actions;
            id_for_actions.clear();
        }
    }
    else if (action_symbol == "#ACTION_STORE_ID_FOR_LHS") {
        if (id_for_actions.empty()) {
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": no identifier provided for left-hand side";
            throw std::runtime_error(ss.str());
        }
        if (current_token_idx < tokens_list.size() && tokens_list[current_token_idx].value == "[") {
            saved_array_id = id_for_actions;
        }
        id_for_lhs = id_for_actions;
    }
    else if (action_symbol == "#ACTION_CHECK_VAR_EXISTS") {
        if (!is_variable_declared(id_for_lhs)) {
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": undeclared variable or array '" << id_for_lhs << "'";
            throw std::runtime_error(ss.str());
        }
    }
    else if (action_symbol == "#ACTION_PROCESS_NUMBER") {
        if (number_for_actions.empty()) {
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": no number provided for numeric expression";
            throw std::runtime_error(ss.str());
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
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": no comparison operator set for comparison operation";
            throw std::runtime_error(ss.str());
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
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": undeclared variable '" << id_for_lhs << "' in assignment";
            throw std::runtime_error(ss.str());
        }
        add_ops_instruction("=", id_for_lhs);
        id_for_lhs.clear();
        is_array_access = false;
    }
    else if (action_symbol == "#ACTION_STORE_SIZE_ID_OR_NUM") {
        if (!id_for_actions.empty()) {
            if (!is_variable_declared(id_for_actions)) {
                std::stringstream ss;
                ss << "Semantic error at line " << token.line << ", position " << token.pos
                    << ": undeclared variable '" << id_for_actions << "' used as array size";
                throw std::runtime_error(ss.str());
            }
            add_ops_instruction("", id_for_actions);
            id_for_actions.clear();
        }
        else if (!number_for_actions.empty()) {
            add_ops_instruction("", number_for_actions);
            number_for_actions.clear();
        }
        else {
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": no identifier or number provided for array size for '" << id_for_lhs << "'";
            throw std::runtime_error(ss.str());
        }
    }
    else if (action_symbol == "#ACTION_ALLOC_ARRAY") {
        if (is_variable_declared(id_for_lhs)) {
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": array or variable '" << id_for_lhs << "' already declared";
            throw std::runtime_error(ss.str());
        }
        add_ops_instruction("alloc_array", id_for_lhs);
        declared_arrays_set.insert(id_for_lhs);
        id_for_lhs.clear();
    }
    else if (action_symbol == "#ACTION_SET_ARRAY_ACCESS") {
        if (saved_array_id.empty()) {
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": no array identifier provided for array access";
            throw std::runtime_error(ss.str());
        }
        is_array_access = true;
        id_for_actions = saved_array_id;
        saved_array_id.clear();
    }
    else if (action_symbol == "#ACTION_VAR_OR_ARRAY_GET") {
        if (id_for_actions.empty()) {
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": no identifier provided for variable or array access";
            throw std::runtime_error(ss.str());
        }
        if (!is_variable_declared(id_for_actions)) {
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": undeclared variable or array '" << id_for_actions << "' in expression";
            throw std::runtime_error(ss.str());
        }
        if (declared_arrays_set.count(id_for_actions) && is_array_access) {
            add_ops_instruction("array_get", id_for_actions);
        }
        else {
            add_ops_instruction("", id_for_actions);
        }
        id_for_actions.clear();
        is_array_access = false;
    }
    else if (action_symbol == "#ACTION_ARRAY_INDEX_FOR_SET") {
        is_array_access = true;
    }
    else if (action_symbol == "#ACTION_ARRAY_SET") {
        if (!is_variable_declared(id_for_lhs)) {
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": undeclared array '" << id_for_lhs << "' in array set operation";
            throw std::runtime_error(ss.str());
        }
        add_ops_instruction("array_set", id_for_lhs);
        id_for_lhs.clear();
        is_array_access = false;
    }
    else if (action_symbol == "#ACTION_READ_VAR_OR_ARRAY") {
        if (!is_variable_declared(id_for_lhs)) {
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": undeclared variable or array '" << id_for_lhs << "' in read operation";
            throw std::runtime_error(ss.str());
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
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": variable or array '" << id_for_lhs << "' already declared";
            throw std::runtime_error(ss.str());
        }
        sym_table.add_variable(id_for_lhs, 0);
        id_for_lhs.clear();
    }
    else if (action_symbol == "#ACTION_DECL_INIT") {
        if (is_variable_declared(id_for_lhs)) {
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": variable or array '" << id_for_lhs << "' already declared";
            throw std::runtime_error(ss.str());
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
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": label stack underflow in else branch";
            throw std::runtime_error(ss.str());
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
            std::stringstream ss;
            ss << "Semantic error at line " << token.line << ", position " << token.pos
                << ": label stack underflow in while loop";
            throw std::runtime_error(ss.str());
        }
        size_t jf_target_pos = pop_label_ops_stack();
        size_t loop_start_pos = pop_label_ops_stack();
        add_ops_instruction("j", std::to_string(loop_start_pos));
        set_jump_target(jf_target_pos, ops_list.size());
    }
    else {
        std::stringstream ss;
        ss << "Semantic error at line " << token.line << ", position " << token.pos
            << ": unknown semantic action '" << action_symbol << "'";
        throw std::runtime_error(ss.str());
    }
}

void Parser::add_ops_instruction(const std::string& op, const std::string& arg) {
    const Token& token = (current_token_idx > 0 && current_token_idx <= tokens_list.size())
        ? tokens_list[current_token_idx - 1]
        : (current_token_idx < tokens_list.size() ? tokens_list[current_token_idx] : Token("EOF", "", 0, 0));

    if (op.empty() && arg.empty()) {
        std::stringstream ss;
        ss << "Semantic error at line " << token.line << ", position " << token.pos
            << ": attempt to add empty OPS instruction";
        throw std::runtime_error(ss.str());
    }
    if (!arg.empty() && !isdigit(arg[0]) && op != "alloc_array" && op != "init_array" && !is_variable_declared(arg)) {
        std::stringstream ss;
        ss << "Semantic error at line " << token.line << ", position " << token.pos
            << ": undeclared variable or array '" << arg << "' in OPS instruction";
        throw std::runtime_error(ss.str());
    }
    ops_list.emplace_back(op, arg);
    if (!silent_mode_active) {
        std::cout << "Added OPS: " << op << (arg.empty() ? "" : " " + arg) << "\n";
    }
}

void Parser::push_label_ops_stack(size_t p) {
    label_stack.push(p);
}

size_t Parser::pop_label_ops_stack() {
    const Token& token = (current_token_idx > 0 && current_token_idx <= tokens_list.size())
        ? tokens_list[current_token_idx - 1]
        : (current_token_idx < tokens_list.size() ? tokens_list[current_token_idx] : Token("EOF", "", 0, 0));

    if (label_stack.empty()) {
        std::stringstream ss;
        ss << "Semantic error at line " << token.line << ", position " << token.pos
            << ": label stack is empty";
        throw std::runtime_error(ss.str());
    }
    size_t v = label_stack.top();
    label_stack.pop();
    return v;
}

void Parser::set_jump_target(size_t p, size_t t) {
    const Token& token = (current_token_idx > 0 && current_token_idx <= tokens_list.size())
        ? tokens_list[current_token_idx - 1]
        : (current_token_idx < tokens_list.size() ? tokens_list[current_token_idx] : Token("EOF", "", 0, 0));

    if (p >= ops_list.size()) {
        std::stringstream ss;
        ss << "Semantic error at line " << token.line << ", position " << token.pos
            << ": invalid jump label position " << p;
        throw std::runtime_error(ss.str());
    }
    if (ops_list[p].operation != "jf" && ops_list[p].operation != "j") {
        std::stringstream ss;
        ss << "Semantic error at line " << token.line << ", position " << token.pos
            << ": attempt to set jump target on non-jump instruction at position " << p;
        throw std::runtime_error(ss.str());
    }
    ops_list[p].operand = std::to_string(t);
    if (!silent_mode_active) {
        std::cout << "Set target " << p << "->" << t << "\n";
    }
}