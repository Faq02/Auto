#include <windows.h>
#include <fstream>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>
#include "File_funcs.h"
#include "Groups.h"
#include <ShlObj.h>
#include <comutil.h>
#include <KnownFolders.h>
#include "advanced_choice.h"
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "comsuppw.lib")
using std::wstring;
using std::string;


static wchar_t read_key()
{
    std::wstring s;
    std::getline(std::wcin, s);
    return s.empty() ? L'\0' : s[0];
}

static std::wstring read_line(const std::wstring& prompt)
{
    std::wstring s;
    std::wcout << prompt;
    std::getline(std::wcin, s);
    return s;
}

enum class WinPathResult
{
    Path,      // обычный выбранный путь
    Link,      // пользователь выбрал "1 — ввести ссылку"
    Switch,    // смена режима (` или ё)
    End,       // 0 — выйти
    Cancel     // пользователь закрыл диалог выбора
};

static std::pair<std::wstring, WinPathResult> win_path_manual()
{
    std::wcout << L"Режим: Ручное добавление (Win-path)\n";
    std::wcout << L"( ` ) — сменить режим\n0 — закончить\n1 — ввести ссылку\n";

    std::wstring key = read_line(L"Нажмите Enter для выбора пути...\n");

    if (key == L"0") return { L"", WinPathResult::End };
    if (key == L"`" || key == L"ё") return { L"", WinPathResult::Switch };
    if (key == L"1") return { L"", WinPathResult::Link };

    auto result = Win_path_selector();

    if (!std::get<0>(result))
    {
        std::wcout << L"Отменено.\n";
        Sleep(200);
        return { L"", WinPathResult::Cancel };
    }

    return { std::get<1>(result), WinPathResult::Path };
}

std::wstring group_add(std::wstring prog_view)
{
    bool manual = false;
    std::wstring gr;
    std::wstring c;
    std::tuple<std::wstring, bool> res;
    bool link = false;
    bool end = false;
    while (true)
    {
        system("cls");
        c = std::to_wstring(advansed_chooser({ 
            .lines_to_choose = { L"игры", L"программы",  L"ссылки", L"или 0—закончить" }, 
            .singleChoice = true, 
            .title = (manual ? L"Режим: Ручное добавление\n( ` ) — сменить режим\n" : L"Режим: Выбор из имеющихся\n( ` ) — сменить режим\n") })[0]);
        /*std::wcout << (manual ? L"Режим: Ручное добавление\n"
            : L"Режим: Выбор из имеющихся\n");

        std::wcout << L"( ` ) — сменить режим\n";
        std::wcout << L"0 — закончить\n";*/

        /*if (!manual)
            std::wcout << L"1-игры  2-программы  3-ссылки\n";*/

        if (c == L"0" || c == L"4") break;

        if (c == L"5")
        {
            manual = !manual;
            if (!manual) { continue; }
            system("cls");
        }

        //
        // ≡≡≡ РЕЖИМ РУЧНОГО ВВОДА ≡≡≡
        //
        if (manual)
        {
            while (true)
            {
                // режим Win-path
                if (prog_view == L"2")
                {
                    auto [value, code] = win_path_manual();

                    switch (code)
                    {
                    case WinPathResult::End:
                        end = true;
                        break;

                    case WinPathResult::Switch:
                        manual = false;
                        break;

                    case WinPathResult::Link:
                    {
                        // Запрашиваем ссылку обычным способом
                        std::wstring link_input = read_line(
                            L"Введите ссылку (0 — выход, ` — режим выбора):\n");

                        if (link_input == L"0") { end = true; }
                        else if (link_input == L"`" || link_input == L"ё") manual = false;
                        else gr += link_input + L"|";

                        break;
                    }

                    case WinPathResult::Cancel:
                        continue; // просто пропускаем

                    case WinPathResult::Path:
                        gr += value + L"|";
                        continue; // остаёмся в ручном режиме
                    }

                    if (end || !manual) break;
                    continue;
                }

                // обычный режим ввода строки
                std::wstring line = read_line(
                    L"Режим: Ручное добавление\n"
                    L"( ` ) — сменить режим\n"
                    L"0 — закончить\n"
                    L"Введите путь или ссылку:\n"
                );

                if (line == L"0") { end = true; break; }
                if (line == L"`" || line == L"ё") { manual = false; break; }

                if (!line.empty())
                    gr += line + L"|";

            }

            if (end) break;
            continue;
        }

        //
        // ≡≡≡ РЕЖИМ ВЫБОРА ИЗ СПИСКА ≡≡≡
        //
        switch (stoi(c))
        {
        case 1:
            //std::wcout << get<std::wstring>(readFile("game", false));
            //gr += choose_line(std::stoi(chooser(L"Введите номер игры:\n")), "game") + L"|";
            gr += choose_line(advansed_chooser({ 
                .lines_to_choose = get<std::vector<std::wstring>>(readFile("game", true)), 
                .singleChoice = true, 
                .title = L"Введите номер игры:\n" })[0], "game") + L"|";
            break;

        case 2:
            //std::wcout << get<std::wstring>(readFile("prog", false));
            //gr += choose_line(std::stoi(chooser(L"Введите номер программы:\n")), "prog") + L"|";
            gr += choose_line(advansed_chooser({
                .lines_to_choose = get<std::vector<std::wstring>>(readFile("prog", true)),
                .singleChoice = true,
                .title = L"Введите номер программы:\n" })[0], "prog") + L"|";
            break;

        case 3:
            //std::wcout << get<std::wstring>(readFile("link", false));
            //gr += choose_line(std::stoi(chooser(L"Введите номер ссылки:\n")), "link") + L"|";
            gr += choose_line(advansed_chooser({
                .lines_to_choose = get<std::vector<std::wstring>>(readFile("link", true)),
                .singleChoice = true,
                .title = L"Введите номер ссылки:\n" })[0], "link") + L"|";
            break;
        }
    }

    if (!gr.empty())
    {
        std::wcout << L"Итоговая группа:\n" << gr << L"\n";
        writefile(gr, "group", "", true);
    }

    return gr;
}



int group_del() {
    //std::wcout << get<std::wstring>(readFile("group", false));
    //std::wstring ch = chooser(L"Выбери группу:");
    int ch = advansed_chooser({ 
        .lines_to_choose = get<std::vector<std::wstring>>(readFile("group", true)), 
        .singleChoice = true, 
        .title = L"Выбери группу:\n" })[0];
    std::wstring line = choose_line(ch, "group");
    // 1. Разбиваем строку на элементы с сохранением-делаем массив по пробелу
    std::vector<std::wstring> elements;
    size_t start = 0;
    for (size_t i = 0; i <= line.length(); i++) {
        if (i == line.length() || line[i] == L'|') {
            if (i > start) {  // Избегаем пустых строк
                elements.push_back(line.substr(start, i - start));
            }
            start = i + 1;
        }
    }
    // 2. Показываем элементы с нумерацией
    for (size_t i = 0; i < elements.size(); i++) {
        std::wcout << i + 1 << L" " << elements[i] << std::endl;
    }
    //std::vector<int> choices_massive = massive_maker(L"Выбери номера для удаления");
     
    std::vector<int> choices_massive = advansed_chooser({ 
        .lines_to_choose = get<std::vector<std::wstring>>(readFile("group", true)), 
        .singleChoice = false, 
        .title = L"Выбери номера для удаления\n" });
    // 4. Формируем новую строку
    std::wstring retern_line;
    for (size_t i = 0; i < elements.size(); i++) {
        if (std::find(choices_massive.begin(), choices_massive.end(), i + 1) == choices_massive.end()) {
            retern_line += elements[i] + L'|';
        }
    }
    // Удаляем последний пробел
    if (!retern_line.empty()) {
        retern_line.pop_back();
    }
    std::wcout << L"Новая группа:" << retern_line+L"|" << std::endl;
    delete_lines_or_insert_one("group",{}, true, retern_line+L"|", ch,true);
    return 0;
}

void create_shortcut(int group_number, wchar_t hotkey_letter, const std::wstring& shortcut_path) {
    HRESULT hres = CoInitialize(NULL);
    if (FAILED(hres)) return;

    IShellLinkW* pShellLink = NULL;
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&pShellLink);

    if (SUCCEEDED(hres)) {
        wchar_t exe_path[MAX_PATH];
        GetModuleFileNameW(NULL, exe_path, MAX_PATH);

        // Получаем рабочую директорию
        std::wstring working_dir = exe_path;
        working_dir = working_dir.substr(0, working_dir.find_last_of(L"\\/"));

        pShellLink->SetPath(exe_path);
        std::wstring arguments = L"--run-group " + std::to_wstring(group_number);
        pShellLink->SetArguments(arguments.c_str());
        pShellLink->SetWorkingDirectory(working_dir.c_str());

        // Настройка горячей клавиши
        WORD hotkey = (HOTKEYF_CONTROL | HOTKEYF_ALT) << 8 | (WORD)hotkey_letter;
        pShellLink->SetHotkey(hotkey);

        // Дополнительные настройки
        pShellLink->SetDescription(L"иди нахер, самый умный что-ли?");
        pShellLink->SetShowCmd(SW_SHOWMINNOACTIVE);

        // Сохранение ярлыка
        IPersistFile* pPersistFile = NULL;
        hres = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
        if (SUCCEEDED(hres)) {
            pPersistFile->Save(shortcut_path.c_str(), TRUE);
            pPersistFile->Release();
        }
        pShellLink->Release();

        // Обновляем кэш горячих клавиш
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    }
    CoUninitialize();
}

std::wstring GetProgramsMenuPath() {
    PWSTR path = nullptr;
    HRESULT hr = SHGetKnownFolderPath(
        FOLDERID_Programs,  // Конкретно папка "Программы"
        0,
        NULL,
        &path
    );

    std::wstring result;
    if (SUCCEEDED(hr)) {
        result = path;
        CoTaskMemFree(path);
    }
    return result;
}

std::wstring GetOrCreateAppFolder() {
    std::wstring programs_path = GetProgramsMenuPath();
    if (programs_path.empty()) {
        return L"";
    }

    // Создаем подпапку для вашего приложения
    std::wstring app_folder = programs_path + L"\\MyAppShortcuts";

    // Создаем папку если ее нет
    if (!CreateDirectoryW(app_folder.c_str(), NULL)) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            return programs_path;  // Возвращаем основную папку при ошибке
        }
    }

    return app_folder;
}