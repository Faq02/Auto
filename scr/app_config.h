#pragma once
#include <string>
#include <map>
enum class FileType { Game, Program, Link, Script, Group, Settings, Scrtemp, Asadmintmp, Grouptmp, null };
static const std::map<FileType, std::string> FILE_NAMES = {
    {FileType::Game, "gamespaths.txt"},
    {FileType::Link, "linkspath.txt"},
    {FileType::Program, "progpaths.txt"},
    {FileType::Script, "scripts.txt"},
    {FileType::Group, "groups.txt"},
    {FileType::Settings, "settings.txt"},
    {FileType::Scrtemp, "scrtemp.txt"},
    {FileType::Asadmintmp, "Asadmintmp.txt"},
    {FileType::Grouptmp, "Grouptmp.txt"},
    {FileType::null, ""},
};
struct Flags {
    static inline const std::wstring Asadmin = L"Asadmin";
    static inline const std::wstring CloseAfter = L"CloseAfter";
};
inline std::string getFileName(FileType type) {
    return FILE_NAMES.at(type);
}
static const std::map<std::wstring, std::wstring> sorted_dict = {
    {L"pyautogui.click", L"клик по координатам: " },
    {L"time.sleep", L"пауза на: " },
    {L"pyautogui.hotkey", L"нажатие горячей клавиши: " },
    {L"keyboard.write", L"ввод с клавиатуры: " },
    {L"pyautogui.press", L"нажатие клавиши: "},
    {L"pyautogui.moveTo", L"передвинуть мышку на координаты" }
};