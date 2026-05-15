#include <string>
#include <windows.h>
#include <iostream>
#include <vector>
#include "path_handler.h"
#include "logger.h"
#include "app_config.h"

constexpr std::wstring Console_standart = L"1";
constexpr std::wstring Win_path = L"2";
std::tuple<bool, std::wstring> Win_path_selector()
{
    OPENFILENAME ofn;
    wchar_t filePath[MAX_PATH] = L"";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = L"*.exe *.bat *.cmd *.txt *.doc *.url *.py\0*.exe;*.bat;*.cmd;*.txt;*.doc;*.url;*.py\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"Что записать в файл?";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileName(&ofn) == TRUE) {
        std::wstring tmp = std::wstring(filePath);
        return std::make_tuple(true, tmp);
    }
    else { return std::make_tuple(false, L""); }
    // Пользователь отменил выбор или произошла ошибка
    //DWORD error = CommDlgExtendedError();
}

void If_prog_view_is_2_and_false()
{
    DWORD error = CommDlgExtendedError();
    if (error) {
        std::wcerr << L"Ошибка: " << error << L"\n";
    }
    else {
        std::wcout << L"Выбор файла отменен" << L"\n";
    }
}

std::wstring choose_file_on_pc(std::wstring& path_choose_view, FileType type, short app_type) {
    if (path_choose_view == Win_path and (app_type != 3 or type != FileType::Link)) {
        std::tuple<bool, std::wstring> has_chousen;
        has_chousen = Win_path_selector();
        if (std::get<0>(has_chousen) == true) {
            return std::get<1>(has_chousen);
        }
        else {
            If_prog_view_is_2_and_false();
            return L"";
        }
    }
    if (path_choose_view == Console_standart) {
        std::wstring file_path;
        file_path = input_line(L"Введите путь: ");
        return file_path;
    }
    return input_line(L"Введите ссылку: \n");
}
//скорей всего её отсюда надо убрать в ui_interactions
//делитель - почти всё \n пробел и \t
std::wstring input_word(const std::wstring& what_input)
{
    std::wcout << what_input << L'\n';
    std::wstring choice;
    //choice.reserve(4); -- было нужно для числовых значений, но есть использование как строки
    std::wcin >> choice;
    std::wcin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    return choice;
}
//делитель - новая строка
std::wstring input_line(const std::wstring& prompt) {
    std::wcout << prompt << L'\n';
    std::wstring line;
    std::getline(std::wcin, line);
    return line;
}

static bool isNotSpace(wchar_t c) {
    return !std::isspace(static_cast<wint_t>(c));
}

std::vector<std::wstring> split_lineOn_paths(std::wstring& line) {
    log(L"inter split_lineOn_paths\noriginal line: " + line + L"\nsplited:\n");
    std::vector<std::wstring> lines;
    auto it = std::find_if(line.rbegin(), line.rend(), isNotSpace); //узнаём 1 символ не пробел с конца строки
    line.erase(it.base(), line.end()); //режем по этот символ включ.

    int c = -1;
    int first = 0, second = 0;

    for (int i = 0; i < line.size(); ++i) {
        c++;
        bool is_path_start = (std::isupper(line[i]) && (i + 2 < line.size()) && (line.substr(i + 1, 2) == L":\\" || line.substr(i + 1, 2) == L":/"));
        bool is_end = (c == line.size() - 1);
        if ((is_path_start || is_end) && c != 0) {
            first = second;
            second = c;
            if (is_end) {
                second += 2;
            }
            if (first < second) {
                lines.push_back(line.substr(first, second - first - 1));
            }
        }
    }
    for (std::wstring l : lines) {
        log(l + L"\n");
    }
    return lines;
}

std::wstring extract_filename(const std::wstring& path) {
    const size_t last_slash = path.find_last_of(L"\\/");
    return (last_slash == std::wstring::npos) ? path : path.substr(last_slash + 1);
}   