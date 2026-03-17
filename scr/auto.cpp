// auto.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#include "File_funcs.h"
#include "Groups.h"
#include "StartFuncs.h"
#include "advanced_choice.h"
#include "settings.h"
#include "Changer.h"
#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>
#include <locale>
#include <codecvt>
#include <fcntl.h>
#include <io.h>
#include <map>
#include <functional>
#include <shellapi.h>
#include <limits>
#include <vector>
#include <sstream>
#include <filesystem>
#include <stdexcept>

static constexpr auto EXIT_CODE = -1;
using namespace std;
using std::get;
const map<short, pair<wstring, string>> FILE_TYPES = { //map это словарь в данном случае он объединяет широкие строки и нет
    {1, {L"игру", "game"}},                            //и посути мы сделали FILE_TYPES словарь глобальным и вызываем его с авто выбором по значению...
    {2, {L"программу", "prog"}},
    {3, {L"ссылку", "link"}},
    {4, {L"скрипт", "scripts"}},
    {5, {L"группу", "group"}}                                                   /*
                                                         {1, {L"игру", "game"}} где объявление игру-wstring, game-string
                                                          ↑    ↑       ↑
                                                          |    |       it->second.second → "game"
                                                          |    it->second.first → L"игру"
                                                          it->first → 1
                                                       */
};




void EnableANSI() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}



static void create_group_shortcut() {
    //std::wcout << get<std::wstring>(readFile("group", false));
    int group_number = advansed_chooser({ 
        .lines_to_choose = get<vector<wstring>>(readFile({.file_type = "group", .isVector = true})),
        .singleChoice = true, 
        .title = L"Выберите группу для создания ярлыка:\n" })[0];
    if (group_number == EXIT_CODE) return;
    std::wcout << L"Введите букву для комбинации Ctrl+Alt+<буква>:\n";
    wchar_t hotkey_letter;
    std::wcin >> hotkey_letter;
    std::wcin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

    // Преобразуем в верхний регистр
    hotkey_letter = towupper(hotkey_letter);

    // Создаем путь для ярлыка
    std::wstring shortcut_folder = GetOrCreateAppFolder();
    if (shortcut_folder.empty()) {
        shortcut_folder = GetProgramsMenuPath();
    }

    std::wstring shortcut_path = shortcut_folder + L"\\Group_" +
        std::to_wstring(group_number) +
        L"_" + hotkey_letter +
        L".lnk";

    create_shortcut(group_number, hotkey_letter, shortcut_path);
    std::wcout << L"Ярлык создан: " << shortcut_path << std::endl;
}




static void add_or_delete(int app_type, PythonRuntime& python, wstring prog_view = L"") {
    auto it = FILE_TYPES.find(app_type);
    if (app_type == 7) //программные настройки
    {
        prog_settings(true,NULL);
        read_set(); //если смена настроек, меняем структуру, вписывая новые значения!
        return;
    }
    if (app_type == 6) //создание ярлыка группы
    {
        create_group_shortcut();
        return;
    }
    //if (prog_view == L"2" && it != FILE_TYPES.end())//it->second.second
    //{
    //    std::wcout << std::get<std::wstring> (readFile(it->second.second, false));
    //}
    bool is_group_or_link = app_type == 3 or app_type == 5;
    vector<wstring> lineees = showfile(it->second.second, CURRENT_SETTINGS.showlines_num);
    wstring title;
    for (auto& line : lineees) {
        title += line + L"\n";
    }
    vector<wstring> lines_to_ch = { L"Добавить" };
        if (!is_group_or_link) { lines_to_ch.push_back(L"Добавить с флагом"); }
        lines_to_ch.push_back(L"Удалить");
        lines_to_ch.push_back(L"Изменить");
    int action = advansed_chooser({
        .lines_to_choose = lines_to_ch,
        .singleChoice = true,
        .title = title })[0];

    if (it != FILE_TYPES.end()) {
        func_linker(action, it->second.second, prog_view, false, NULL, &python);
        if (action == 1) { countdown(0.5, L"Осталось до возвращения: ", 1); }
        if (action == 2) { countdown(3, L"Осталось до возвращения: ", 1); }
        if (it->second.second == "scripts") { make_txt_for_scripts("scripts\\"); } //скрипты изначально-файлы, уже потом-запись в файл с путями
    }
    else { wcerr << L"Неверный тип контента!\n"; }
}


static void settings(wstring pr_view, PythonRuntime& python) {
    //std::wstring category = chooser(L"Выбор категории:\n1-Игры, 2-Программы, 3-Ссылки, 4-скрипты \033[31m5-Группы\033[0m 6-ярлык 7-настройки работы");
    while (true) {
        int category = advansed_chooser({
            .lines_to_choose = { L"Игры", L"Программы", L"Ссылки", L"скрипты", L"\033[31mГруппы\033[0m", L"Создать ярлык", L"Настройки работы" },
            .singleChoice = true,
            .title = L"Выбор категории:\n" })[0];
        if (category == EXIT_CODE) { break; }
        add_or_delete(category, python, CURRENT_SETTINGS.path_view_num);
    }
    
}

static void run_application(short app_type, PythonRuntime& python) {
    if (auto it = FILE_TYPES.find(app_type); it != FILE_TYPES.end()) { // 1 из фич с++ 17 - обявление переменной в if зачем? я хз, но тут объявляем переменную содерж. словарь в которому мы выбираем элементы
        bool is_group_or_link = app_type == 3 or app_type == 5;
        //std::wcout << to_wstring(app_type); system("pause");
        //std::wcout << is_group_or_link; system("pause");
        
        vector<wstring> names = showfile(it->second.second, CURRENT_SETTINGS.showlines_num);
        
        //vector<wstring> lines = get<std::vector<std::wstring>>(readFile({
        //    .file_type = it->second.second,
        //    .isVector = true,
        //    .for_py_code = false, //"костыль" так как не хочу добавлять ещё флаг и код и показ имён никогда не встретится, поэтому если true, то это либо группа либо ссылка
        //    })); //один раз прочитали можно запомнить размер массива!!!!!!!!!!!!!!!!!!!!!!!!
        auto orig_lines_count = names.size();
        names.push_back(L" ");
        names.push_back(L"Выбрать файл на компьютере");
        
        
        
        while (true) {
            //second.second -- во втором(элементе-словаре) второй элемент
            int line_number = advansed_chooser({
                .lines_to_choose = names,
                .singleChoice = true,
                .title = L"Выберите " + it->second.first + L"\n" })[0];
            if (line_number == orig_lines_count + 1) { continue; }//предпоследний выбор-пустой выбор
            if (line_number == orig_lines_count + 2) {
                wstring custom_path = choose_file_on_pc(CURRENT_SETTINGS.path_view_num, app_type);
                if (!custom_path.empty()) {
                    //std::wcout << custom_path; Sleep(10000);
                    short app_typE = is_valid_file_type(custom_path);
                    //std::wcout << to_wstring(app_typE); Sleep(10000);
                    if (it->first == 1 && app_typE == 2) { app_typE = 1; }
                    if (auto iT = FILE_TYPES.find(app_typE); iT != FILE_TYPES.end()) { 
                        writefile(custom_path, iT->second.second, "", false);
                        startfiles(orig_lines_count + 1, iT->second.second, &python, "");
                    }
                    return;
                }
                countdown(1, L"Вы передумали или ошибка, возвращаемся...", 1);
                return;
            }
            if (line_number == EXIT_CODE) return;
            startfiles(line_number, it->second.second, &python, "");
            return;
        }
    }
    else { wcerr << L"\033[31mНеверный тип приложения!\033[0m\n"; }
}

int main(int argc, char* argv[]) {
    // Настройка консоли
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);

    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    if (!std::filesystem::exists("python_embed") ||
        !std::filesystem::exists("python_embed/python310.zip")) {
        std::wcerr << L"ОШИБКА: Папка python_embed или python310.zip не найдены!\n";
        std::wcerr << L"Разместите папку python_embed рядом с exe-файлом.\n";
        system("pause");
        return 1;
    }

    PythonRuntime python;
    //Sleep(10000);
    //test
    //startfiles(NULL, "", &python, WstringTo_Utf8(get<std::wstring>(readFile("1test.txt", false, true))));
    try {
        //startfiles(NULL, "", &python, WstringTo_Utf8(get<std::wstring>(readFile("1test.txt", false, true))));
        if (argc > 1 and argc != 4) {
            std::string command = argv[1];
            if (command == "--run-group" && argc > 2) {
                int group_number = std::stoi(argv[2]);
                std::wcout << group_number << std::endl;
                startfiles(group_number, "group", &python, "");
                return 0;
            }
        }
        wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (argc >= 4 && wcscmp(argv[1], L"--create-task") == 0) {
            // Режим создания задачи: имя и путь переданы как аргументы
            std::wstring name = argv[2];
            std::wstring path = argv[3];
            return makeTaskAdmin(name, path);
        }
    
    EnableANSI();
    files_checker(); //после потому-что подуразумивается, что это не 1-ый раз работы прогрммы
    make_txt_for_scripts("scripts\\"); // 1 перезапись файла с путями дальше в add_or_delete
    read_set(); // 1 чтение настроек



    // Меню действий

    //Основной цикл
    while (true) {
        std::vector<wstring> lines = {
                L"Запустить игру",
                L"Запустить программу",
                L"Запустить вашы ссылки",
                L"Запустить вашы cкрипты",
                L"Запуск \033[31mгруппы\033[0m",
                L"Настройки" };
        int choice = advansed_chooser({
            .lines_to_choose = lines,
            .singleChoice = true,
            .title = L"Ваш вид выбора пути: " + CURRENT_SETTINGS.path_choose_view + L"\nВаши авто-действия если путь неверный: " + CURRENT_SETTINGS.if_path_wrong + L"\nВыберите действие\n" })[0];
        if (choice == EXIT_CODE) { break; }
        switch (choice) {
        case 1: run_application(1, python); countdown(2, L"Осталось до возвращения: ", 1); break;
        case 2: run_application(2, python); countdown(2, L"Осталось до возвращения: ", 1); break;
        case 3: run_application(3, python); countdown(2, L"Осталось до возвращения: ", 1); break;
        case 4: run_application(4, python); countdown(2, L"Осталось до возвращения: ", 1); break;
        case 5: run_application(5, python); break;
        case 6: settings(CURRENT_SETTINGS.path_view_num,python); break;
        default: wcerr << L"Неверный выбор!\n"; break;
        }
    }
    }
    catch (const std::exception& e) {
        std::wcerr << L"Ошибка выполнения: " << e.what() << std::endl;
        // Можно добавить дополнительную информацию
        #ifdef _DEBUG
        std::wcerr << L"Exception details: " << e.what() << std::endl;
        #endif
    }
    catch (const std::string& e) {
        std::wcerr << L"Строковая ошибка: " << e.c_str() << std::endl;
    }
    catch (const char* e) {
        std::wcerr << L"Ошибка C-строки: " << e << std::endl;
    }
    catch (...) {
        std::wcerr << L"Неизвестная критическая ошибка!" << std::endl;
        #ifdef _WIN32
        DWORD error = GetLastError();
        if (error != 0) {
            std::wcerr << L"Код ошибки Windows: " << error << std::endl;
        }
        #endif
    }
    return 0;
}
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.

