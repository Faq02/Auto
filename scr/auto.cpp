// auto.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#include "File_funcs.h"
#include "Groups.h"
#include "StartFuncs.h"
#include "advanced_choice.h"
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


const map<wstring, wstring> PATH_CHOOSE_VIEW = {
    {L"1", L"Консоль-страндарт"},
    {L"2", L"По вин-путям"}
};
const map<wstring, wstring> IF_PATH_WRONG = {
    {L"1", L"Использовать поиск"},
    {L"2", L"Перевыбор"},
    {L"3", L"Ничего не делать"}
};

struct ProgSettings {
    wstring path_choose_view;
    wstring path_view_num;
    wstring if_path_wrong;
    wstring if_wrong_num;
};
ProgSettings CURRENT_SETTINGS;

static void read_set() {
    std::vector<std::wstring> sett = get<std::vector<std::wstring>>(readFile("sett", true));
    auto it = PATH_CHOOSE_VIEW.find(sett[0]);
    if (it != PATH_CHOOSE_VIEW.end()) {
        CURRENT_SETTINGS.path_choose_view = it->second;
        CURRENT_SETTINGS.path_view_num = it->first;
    }
    auto it1 = IF_PATH_WRONG.find(sett[1]);
    if (it1 != IF_PATH_WRONG.end()) {
        CURRENT_SETTINGS.if_path_wrong = it1->second;
        CURRENT_SETTINGS.if_wrong_num = it1->first;
    }
}

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
        .lines_to_choose = get<vector<wstring>>(readFile("group", true)), 
        .singleChoice = true, 
        .title = L"Выберите группу для создания ярлыка:\n" })[0];

    std::wcout << L"Введите букву для комбинации Ctrl+Alt+<буква>:\n";
    wchar_t hotkey_letter;
    std::wcin >> hotkey_letter;
    std::wcin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

    hotkey_letter = towupper(hotkey_letter);
    
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




static void add_or_delete(int content_type, wstring prog_view = L"") {
    auto it = FILE_TYPES.find(content_type);
    if (content_type == 7) //программные настройки
    {
        prog_settings(true,NULL);
        read_set(); //если смена настроек, меняем структуру, вписывая новые значения!
        return;
    }
    if (content_type == 6) //создание ярлыка группы
    {
        create_group_shortcut();
        return;
    }
    //if (prog_view == L"2" && it != FILE_TYPES.end())//it->second.second
    //{
    //    std::wcout << std::get<std::wstring> (readFile(it->second.second, false));
    //}
    int action = advansed_chooser({ 
        .lines_to_choose = {L"Добавить", L"Удалить"}, 
        .singleChoice = true, 
        .title = get<wstring>(readFile(it->second.second, false)) + L"\n" })[0];
    if (it != FILE_TYPES.end()) {
        func_linker(action, it->second.second, prog_view, false, NULL);
        if (action == 1) { countdown(0.5, L"Осталось до возвращения: ", 1); }
        if (action == 2) { countdown(3, L"Осталось до возвращения: ", 1); }
        if (it->second.second == "scripts") { make_txt_for_scripts("scripts\\"); } //скрипты изначально-файлы, уже потом-запись в файл с путями
    }
    else { wcerr << L"Неверный тип контента!\n"; }
}


static void settings(wstring pr_view) {
    //std::wstring category = chooser(L"Выбор категории:\n1-Игры, 2-Программы, 3-Ссылки, 4-скрипты \033[31m5-Группы\033[0m 6-ярлык 7-настройки работы");
    while (true) {
        int category = advansed_chooser({
            .lines_to_choose = { L"Игры", L"Программы", L"Ссылки", L"скрипты", L"\033[31mГруппы\033[0m", L"ярлык", L"настройки работы" },
            .singleChoice = true,
            .title = L"Выбор категории:\n" })[0];
        if (category == EXIT_CODE) { break; }
        add_or_delete(category, CURRENT_SETTINGS.path_view_num);
    }
    
}

static void run_application(short app_type) {
    if (auto it = FILE_TYPES.find(app_type); it != FILE_TYPES.end()) { // 1 из фич с++ 17 - обявление переменной в if зачем? я хз, но тут объявляем переменную содерж. словарь в которому мы выбираем элементы
        //second.second -- во втором(элементе-словаре) второй элемент
        int line_number = advansed_chooser({ 
            .lines_to_choose = get<std::vector<std::wstring>>(readFile(it->second.second, true)), 
            .singleChoice = true, 
            .title = L"Выберите " + it->second.first + L"\n" })[0];
        startfiles(line_number, it->second.second);
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

    if (argc > 1) {
        std::string command = argv[1];
        if (command == "--run-group" && argc > 2) {
            int group_number = std::stoi(argv[2]);
            std::wcout << group_number << std::endl;
            startfiles(group_number, "group");
            return 0;
        }
    }
    EnableANSI();
    files_checker(); //после потому-что подуразумивается, что это не 1-ый раз работы прогрммы
    make_txt_for_scripts("scripts\\"); // 1 перезапись файла с путями дальше в add_or_delete
    read_set(); // 1 чтение настроек




    
    
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
            .title = L"Ваш вид выбора пути: " + CURRENT_SETTINGS.path_choose_view + L"\nВаши авто-действия если путь неверный: " + CURRENT_SETTINGS.if_path_wrong + L"\nВыберите действие\n"})[0];
        if (choice == EXIT_CODE) { break; }
        switch (choice) {
            case 1: run_application(1); countdown(3, L"Осталось до возвращения: ", 1); break;
            case 2: run_application(2); countdown(3, L"Осталось до возвращения: ", 1); break;
            case 3: run_application(3); countdown(3, L"Осталось до возвращения: ", 1); break;
            case 4: run_application(4); countdown(3, L"Осталось до возвращения: ", 1); break;
            case 5: run_application(5); break;
            case 6: settings(CURRENT_SETTINGS.path_view_num); break;
            default: wcerr << L"Неверный выбор!\n"; break;
        }
    }
    return 0;
}
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.

