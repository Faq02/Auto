#include <fstream>
#include <filesystem>
#include "logger.h"

namespace fs = std::filesystem;



//удаляет старый лог если есть 
void start_new_log() {
    if (std::filesystem::exists("app.log")) {remove("app.log");}
}


void log(const std::wstring& message)
{
    std::wofstream fout("app.log", std::ios::app);
    fout.imbue(std::locale(fout.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    if (!fout) return;

    std::time_t now = std::time(nullptr);
    std::tm tm_info; // Создаем саму структуру, а не указатель
    wchar_t time_buf[20];
    if (localtime_s(&tm_info, &now) == 0) {
        wcsftime(time_buf, 20, L"%Y-%m-%d %H:%M:%S", &tm_info);
    }
    fout << L"[" << time_buf << L"] " << message << std::endl;

}