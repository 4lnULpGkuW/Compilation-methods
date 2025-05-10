#include "error.h"
#include <sstream>

Error::Error(const std::string& type, const std::string& symbol, int line, int pos)
    : type(type), symbol(symbol), line(line), pos(pos) {}

std::string Error::message() const {
    std::stringstream ss;
    ss << type << " at line " << line << ", position " << pos << ": invalid symbol '" << symbol << "'";
    return ss.str();
}