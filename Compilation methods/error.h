#ifndef ERROR_H
#define ERROR_H

#include <string>

class Error {
public:
    Error(const std::string& type, const std::string& symbol, int line, int pos);
    std::string message() const;

private:
    std::string type;   // ��� ������: lexical, syntax, runtime
    std::string symbol; // ��������� ������ ��� �����
    int line;           // ����� ������
    int pos;            // �������
};

#endif