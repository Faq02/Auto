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
    {L"pyautogui.moveTo", L"передвинуть мышку на координаты" },
    {L"os.system", L"условно(наделе выолнение команды cmd) запуск: "}
};
struct PRINT_TEXTCOLOR {
    inline static const std::wstring BLACK = L"\033[30m";
    inline static const std::wstring RED = L"\033[31m";
    inline static const std::wstring GREEN = L"\033[32m";
    inline static const std::wstring YELLOW = L"\033[33m";
    inline static const std::wstring BLUE = L"\033[34m";
    inline static const std::wstring MAGENTA = L"\033[35m";
    inline static const std::wstring CYAN = L"\033[36m";
    inline static const std::wstring WHITE = L"\033[37m";
};

struct PRINT_BACKGROUNDCOLOR {
    inline static const std::wstring BLACK = L"\033[40m";
    inline static const std::wstring RED = L"\033[41m";
    inline static const std::wstring GREEN = L"\033[42m";
    inline static const std::wstring YELLOW = L"\033[43m";
    inline static const std::wstring BLUE = L"\033[44m";
    inline static const std::wstring MAGENTA = L"\033[45m";
    inline static const std::wstring CYAN = L"\033[46m";
    inline static const std::wstring WHITE = L"\033[47m";
};

struct SelectedItem {
    std::wstring path;    // путь, ссылка или команда
    std::wstring flags;   // флаги (если нужны)
    bool is_link = false; // true – это ссылка (требует start)
    bool cancelled = false; // пользователь отказался
};