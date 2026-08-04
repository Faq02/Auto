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
#include "ui_interactions.h"
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











//скрипты?
enum class SELECT_FROM_LIST {
    Games = 1, GamesWithFlags = 2, Programs = 3, ProgramsWithFlags = 4, Links = 5
};
//НЕПРОВЕРЕНО!!!!!!
void group_add(std::wstring path_choose_view, bool from_changer)
{
    LineEntry group;
    //2 варианта добавления:
    /*
    1-ручной ввод строк
    2-выбор из файла
    */
    int int_type = -1;
    bool with_flags = false;
    bool manual = false;
    std::wstring group_path = L"";

    std::pair<bool, std::wstring> win_manual_pathChoice(false, L"1");
    if (path_choose_view == L"2") {
        win_manual_pathChoice.first = true; win_manual_pathChoice.second = L"2";
    }
    std::vector<std::wstring> lines_to_choose = {};
    SelectedItem selected_item;
    while (true) {
        std::wstring path_choose = (win_manual_pathChoice.first ? L"Win проводник" : L"Консоль стандарт");

        lines_to_choose = { L"Игры", L"Программы", L"Ссылки", L"Cкрипты",L"",
                          
            (with_flags ? L"Флаги: [вкл.]" : L"Флаги: [выкл.]"),
            
            (manual ? L"Ручной выбор: [вкл.]" : L"Ручной выбор: [выкл.]") };
        
        if (manual) lines_to_choose.push_back(L"Ручной выбор(для смены нажмите): " + path_choose);
        lines_to_choose.push_back(L"Закончить");

            


        int_type = advansed_chooser({
            .lines_to_choose = lines_to_choose,
            .singleChoice = true,
            .title = L"Выберите тип(EXIT_CODE для выхода и сохранения или последний пункт): "
            })[0] - 1;//они 0-based, так games это 0
        if (int_type == -2) { //-2 так как если escape то возвращается -1
            break;
        }
        if (int_type == 5) {
            with_flags = !with_flags;
            continue;
        }
        
        if (int_type == 6) {
            manual = !manual;
            continue;
        }
        if (int_type == 7) {
            if (manual) {
                win_manual_pathChoice.first = !win_manual_pathChoice.first;
                win_manual_pathChoice.second = win_manual_pathChoice.first ? L"2" : L"1";
                continue;
            }
            break;
        }
        if (manual and int_type == 8) break;
        FileType type = static_cast<FileType>(int_type);
        selected_item = select_from_file_or_manual(static_cast<FileType>(int_type),false,with_flags,manual, win_manual_pathChoice.second);

        if (selected_item.cancelled) continue;
        int id = selected_item.id;
        if (!selected_item.manual_path.empty()) {
            LineEntry new_line;
            new_line.path = selected_item.manual_path;
            if (!selected_item.flags.empty()) new_line.flags = selected_item.flags;
            id = get_max_id(type)+1;
            new_line.name = L"from_group_make_" + std::to_wstring(id);
            new_line.id = id;
            std::vector<LineEntry> vec_entry = {new_line};
            add_entries(type, vec_entry);//сама сохраняет в файл
        }
        group_path += std::to_wstring(int_type) + L"-" + std::to_wstring(id) + L"|";

    }
    //ручной
    //SelectedItem selected_item = ;
    

    if (group_path.empty()) return;
    group.path = group_path;
    group.id = get_max_id(FileType::Group)+1;
    group.name = L"";
    group.flags = L"";//пока не придумал не одного флага для группы
    std::vector<LineEntry> vec_group_entry = { group };
    add_entries(FileType::Group, vec_group_entry);
    return;
}





//говнокод очень старый
//int group_del() {
//    //std::wcout << get<std::wstring>(readFile("group", false));
//    //std::wstring ch = input_word(L"Выбери группу:");
//    int ch = advansed_chooser({ 
//        .lines_to_choose = get<std::vector<std::wstring>>(readFile({.file_path = FILE_NAMES.at(FileType::Group), .isVector = true})),
//        .singleChoice = true, 
//        .title = L"Выбери группу:\n" })[0];
//    std::wstring line = choose_line(ch, FileType::Group);
//    // 1. Разбиваем строку на элементы с сохранением-делаем массив по пробелу
//    std::vector<std::wstring> elements;
//    size_t start = 0;
//    for (size_t i = 0; i <= line.length(); i++) {
//        if (i == line.length() || line[i] == L'|') {
//            if (i > start) {  // Избегаем пустых строк
//                elements.push_back(line.substr(start, i - start));
//            }
//            start = i + 1;
//        }
//    }
//    // 2. Показываем элементы с нумерацией
//    for (size_t i = 0; i < elements.size(); i++) {
//        std::wcout << i + 1 << L" " << elements[i] << std::endl;
//    }
//    //std::vector<int> choices_massive = massive_maker(L"Выбери номера для удаления");
//     
//    std::vector<int> choices_massive = advansed_chooser({ 
//        .lines_to_choose = get<std::vector<std::wstring>>(readFile({.file_path = FILE_NAMES.at(FileType::Group), .isVector = true})),
//        .singleChoice = false, 
//        .title = L"Выбери номера для удаления\n" });
//    // 4. Формируем новую строку
//    std::wstring retern_line;
//    for (size_t i = 0; i < elements.size(); i++) {
//        if (std::find(choices_massive.begin(), choices_massive.end(), i + 1) == choices_massive.end()) {
//            retern_line += elements[i] + L'|';
//        }
//    }
//    // Удаляем последний пробел
//    if (!retern_line.empty()) {
//        retern_line.pop_back();
//    }
//    std::wcout << L"Новая группа:" << retern_line+L"|" << std::endl;
//    delete_lines_or_insert_or_add_one(FileType::Group,{}, true, retern_line+L"|", ch, true, false);
//    return 0;
//}

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