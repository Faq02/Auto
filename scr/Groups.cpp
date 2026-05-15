#include <windows.h>
#include <fstream>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>

#include "Groups.h"
#include <ShlObj.h>
#include <comutil.h>
#include <KnownFolders.h>
#include "advanced_choice.h"
#include "path_handler.h"
#include "file_io.h"
#include "data_work.h"
#include "logger.h"
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "comsuppw.lib")
using std::wstring;
using std::string;

//...
static wchar_t read_key()
{
    std::wstring s;
    std::getline(std::wcin, s);
    return s.empty() ? L'\0' : s[0];
}
//это вообще что?
static std::wstring read_line(const std::wstring& prompt)
{
    std::wstring s;
    std::wcout << prompt;
    std::getline(std::wcin, s);
    return s;
}

//принимает группу и возвращает с добавленой строкой-выбором
std::wstring select_with_flags(std::wstring group, FileType type, std::wstring promt) {
    int line_num = advansed_chooser({
                .lines_to_choose = get<std::vector<std::wstring>>(readFile({.file_path = FILE_NAMES.at(type), .isVector = true})),
                .singleChoice = true,
                .title = L"Введите номер " + promt + L":\n"})[0];

    LineEntry entry = line_parser(type, line_num, L"");
    int choice = advansed_chooser({
        .lines_to_choose = {L"Заменить флаги", L"Оставить существующие"},
        .singleChoice = true,
        .title = L"Путь:" + entry.path + L"\n" + L"Имя:" + entry.name + L"\n" + L"Флаги:" + entry.flags + L"\n" })[0];
    
    std::wstring flags;
    switch (choice) {
    case 1:
        flags = make_flags();
        group += choose_line(line_num, type) + L"\"" + entry.name + L"\"" + flags + L"|";
        break;
    case 2:
        group += choose_line(line_num, type) + L"\"" + entry.name + L"\"" + entry.flags + L"|";
        break;
    }
    return group;
}










enum class WinPathResult
{
    Path,      // обычный выбранный путь
    Link,      // пользователь выбрал "1 — ввести ссылку"
    WithFlags,
    Switch,    // смена режима (` или ё)
    End,       // 0 — выйти
    Cancel     // пользователь закрыл диалог выбора
};
//старый говнокод
static std::pair<std::wstring, WinPathResult> win_path_manual()
{
    std::wcout << L"Режим: Ручное добавление (Win-path)\n";
    std::wcout << L"( ` ) — сменить режим\n0 — закончить\n1 — ввести ссылку\n2 - добавить с флагами(откроется выбор пути, затем выбор флагов)";

    std::wstring key = read_line(L"Выберите и нажмите Enter... или нажмите просто Enter для выбора пути без флагов\n");

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
    if (key == L"2") return { std::get<1>(result), WinPathResult::WithFlags };
    return { std::get<1>(result), WinPathResult::Path };
}
//скрипты?
enum class SELECT_FROM_LIST {
    Games = 1, GamesWithFlags = 2, Programs = 3, ProgramsWithFlags = 4, Links = 5
};
//старый говнокод
std::wstring group_add(std::wstring prog_view, bool from_changer)
{
    log(L"group_add called\n");
    bool manual = false;
    std::wstring gr;
    std::wstring c;
    std::tuple<std::wstring, bool> res;
    bool link = false;
    bool end = false;
    while (true)
    {
        //if (from_changer == true) end = true; 
        system("cls");
        c = std::to_wstring(advansed_chooser({ 
            .lines_to_choose = { L"игра", L"игра с флагами", L"программа", L"программа с флагами",  L"ссылка", L"или 0—закончить"},
            .singleChoice = true, 
            .title = (manual ? L"Режим: Ручное добавление\n( ` ) — сменить режим\n" : L"Режим: Выбор из имеющихся\n( ` ) — сменить режим\n") })[0]);

        if (c == L"0" || c == L"6") break;

        if (c == L"7")
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
            std::wstring flgs;
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

                    case WinPathResult::WithFlags:
                        flgs = make_flags();
                        gr += value + L"\"\"" + flgs + L"|"; //пока пустое имя, так как выбор из неизвестно чего
                        break;
                    }
                    if (from_changer == true and !gr.empty()) end = true;
                    if (end || !manual) break;
                    continue;
                }

                // обычный режим ввода строки
                std::wstring line = read_line(
                    L"Режим: Ручное добавление\n"
                    L"( ` ) — сменить режим\n"
                    L"0 — закончить\n"
                    L"1 - добавить флаги после выбора\n"
                    L"Введите путь или ссылку:\n"
                );

                if (line == L"0") { end = true; break; }
                if (line == L"`" || line == L"ё") { manual = false; break; }
                if (!line.empty() and line != L"1")
                    gr += line + L"|";
                
                
                if (line == L"1") {
                    line = read_line(L"LВведите путь или ссылку : \n");
                    flgs = make_flags();
                    gr += line + L"\"\"" + flgs + L"|"; //опять без имени, так как предпологается, что это чисто информация для показа потом в title во время выбора групп
                }

            }
            if (end) break;
            continue;
        }

        //bb
        // ≡≡≡ РЕЖИМ ВЫБОРА ИЗ СПИСКА ≡≡≡
        //bb
        int line_num;
        int choice;
        std::wstring flags;
        LineEntry entry;
        switch (stoi(c))
        {
        case (int)SELECT_FROM_LIST::Games:
            gr += choose_line(advansed_chooser({ 
                .lines_to_choose = get<std::vector<std::wstring>>(readFile({.file_path = FILE_NAMES.at(FileType::Game), .isVector = true})),
                .singleChoice = true, 
                .title = L"Введите номер игры:\n" })[0], FileType::Game) + L"|";
            break;

        case (int)SELECT_FROM_LIST::GamesWithFlags:
            gr = select_with_flags(gr, FileType::Game, L"игры");
            break;

        case (int)SELECT_FROM_LIST::Programs:
            gr += choose_line(advansed_chooser({
                .lines_to_choose = get<std::vector<std::wstring>>(readFile({.file_path = FILE_NAMES.at(FileType::Program), .isVector = true})),
                .singleChoice = true,
                .title = L"Введите номер программы:\n" })[0], FileType::Program) + L"|";
            break;

        case (int)SELECT_FROM_LIST::ProgramsWithFlags:
            gr = select_with_flags(gr, FileType::Program, L"программы");
            break;

        case (int)SELECT_FROM_LIST::Links:
            gr += choose_line(advansed_chooser({
                .lines_to_choose = get<std::vector<std::wstring>>(readFile({.file_path = FILE_NAMES.at(FileType::Link), .isVector = true})),
                .singleChoice = true,
                .title = L"Введите номер ссылки:\n" })[0], FileType::Link) + L"|";
            break;

        }
        if (!gr.empty() and from_changer == true) end = true;
        if (end == true) break;
    }

    if (!gr.empty() and !from_changer)
    {
        std::wcout << L"Итоговая группа:\n" << gr << L"\n";
        writefile(gr, FILE_NAMES.at(FileType::Group), "", true);
    }
    log(L"итоговая группа:" + gr);
    return gr;
}


//говнокод
int group_del() {
    //std::wcout << get<std::wstring>(readFile("group", false));
    //std::wstring ch = input_word(L"Выбери группу:");
    int ch = advansed_chooser({ 
        .lines_to_choose = get<std::vector<std::wstring>>(readFile({.file_path = FILE_NAMES.at(FileType::Group), .isVector = true})),
        .singleChoice = true, 
        .title = L"Выбери группу:\n" })[0];
    std::wstring line = choose_line(ch, FileType::Group);
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
        .lines_to_choose = get<std::vector<std::wstring>>(readFile({.file_path = FILE_NAMES.at(FileType::Group), .isVector = true})),
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
    delete_lines_or_insert_or_add_one(FileType::Group,{}, true, retern_line+L"|", ch, true, false);
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


//не с чем не боролся, умирал от лени