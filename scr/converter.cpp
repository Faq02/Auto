#include <string>
#include <windows.h>
#include "converter.h"

std::wstring StringToWstring(const std::string& str) {
    if (str.empty()) return std::wstring();

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr_to(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr_to[0], size_needed);
    return wstr_to;
}

std::string WstringTo_Utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    // Получаем необходимый размер буфера
    int size_needed = WideCharToMultiByte(
        CP_UTF8,
        0,  // Флаги (0 по умолчанию)
        wstr.c_str(),
        static_cast<int>(wstr.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (size_needed <= 0) {
        // Ошибка при получении размера
        return std::string();
    }

    // Создаем строку нужного размера
    std::string result(size_needed, 0);

    // Выполняем преобразование
    int converted_length = WideCharToMultiByte(
        CP_UTF8,
        0,
        wstr.c_str(),
        static_cast<int>(wstr.size()),
        &result[0],
        size_needed,
        nullptr,
        nullptr
    );

    if (converted_length <= 0) {
        // Ошибка при преобразовании
        return std::string();
    }

    // Возвращаем результат
    return result;
}