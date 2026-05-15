#pragma once
#include <string>
#include "app_config.h"
class PythonRuntime {
private:
    bool m_initialized;  // Флаг состояния
public:
    // Конструктор по умолчанию
    PythonRuntime();
    ~PythonRuntime();
    // Удаляем копирование и присваивание
    PythonRuntime(const PythonRuntime&) = delete;
    PythonRuntime& operator=(const PythonRuntime&) = delete;

    // Методы для работы с Python
    int execute(const std::string& code);
    bool execute_safe(const std::string& code);
};
int startfiles(FileType type, int line_number, PythonRuntime* python, std::string codem, bool from_admin);
std::wstring where_are_you_go_lnk(const std::wstring& lnkPath);
