// auto.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//удачи это понять...
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

#include "app_config.h"
#include "Groups.h"
#include "StartFuncs.h"
#include "advanced_choice.h"
#include "settings.h"
#include "Changer.h"
#include "file_io.h"
#include "data_work.h"
#include "Manager.h"
#include "ui_interactions.h"
#include "path_handler.h"
#include "win_help.h"
#include "logger.h"
#include "converter.h"
#include "Files_checker.h"
#include "ui_maker.h"
#include "fq-start.h"

static constexpr auto EXIT_CODE = -1;
using namespace std;
using std::get;
const map<short, pair<wstring, FileType>> FILE_TYPES = { //map это словарь в данном случае он объединяет широкие строки и нет
    {1, {L"игру", FileType::Game}},                            //и посути мы сделали FILE_TYPES словарь глобальным и вызываем его с авто выбором по значению...
    {2, {L"программу", FileType::Program}},
    {3, {L"ссылку", FileType::Link}},
    {4, {L"скрипт", FileType::Script}},
    {5, {L"группу", FileType::Group}}                                                   /*
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
        .lines_to_choose = show_groups(CURRENT_SETTINGS.showlines_num),
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
    int group_id = global_all_lines[FileType::Group][group_number - 1].id;
    std::wstring shortcut_path = shortcut_folder + L"\\Group_" +
        std::to_wstring(group_number) +
        L"_" + hotkey_letter +
        L".lnk";

    create_shortcut(group_number, hotkey_letter, shortcut_path);
    std::wcout << L"Ярлык создан: " << shortcut_path << std::endl;
}




static void add_or_delete(int app_type, wstring prog_view = L"") {
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
    bool is_group = app_type == 5;
    vector<wstring> lines_to_show; 
    if (is_group) lines_to_show = show_groups(CURRENT_SETTINGS.showlines_num);
    else { lines_to_show = showfile(it->second.second, CURRENT_SETTINGS.showlines_num); }

    wstring title;
    for (auto& line : lines_to_show) {
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
    bool with_flag = false;
    if (action == 2 and !is_group_or_link) {
        with_flag = true; action = 4;
    }
    if (action > 2 and !is_group_or_link and with_flag == false) { action -= 1; }
    if (it != FILE_TYPES.end()) {
        manager(action, it->second.second, prog_view, false);
        if (action == 1) { countdown(0.5, L"Осталось до возвращения: ", 1); }
        if (action == 2) { countdown(3, L"Осталось до возвращения: ", 1); }
        if (it->second.second == FileType::Script) { make_txt_for_scripts("scripts\\"); } //скрипты изначально-файлы, уже потом-запись в файл с путями
    }
    else { wcerr << L"Неверный тип контента!\n"; }
}


static void settings(wstring pr_view) {
    //std::wstring category = input_word(L"Выбор категории:\n1-Игры, 2-Программы, 3-Ссылки, 4-скрипты \033[31m5-Группы\033[0m 6-ярлык 7-настройки работы");
    while (true) {
        int category = advansed_chooser({
            .lines_to_choose = { L"Игры", L"Программы", L"Ссылки", L"скрипты", L"\033[31mГруппы\033[0m", L"Создать ярлык", L"Настройки работы" },
            .singleChoice = true,
            .title = L"Выбор категории:\n" })[0];
        if (category == EXIT_CODE) { break; }
        add_or_delete(category, CURRENT_SETTINGS.path_view_num);
    }
    
}


static void run_application(short app_type) {
    if (auto it = FILE_TYPES.find(app_type); it != FILE_TYPES.end()) { // 1 из фич с++ 17 - обявление переменной в if зачем? я хз, но тут объявляем переменную содерж. словарь в которому мы выбираем элементы
        bool is_group_or_link = app_type == 3 or app_type == 5;
        bool is_group = app_type == 5;
        bool is_link = app_type == 3;
        bool is_script = app_type == 4;
        //std::wcout << to_wstring(app_type); system("pause");
        //std::wcout << is_group_or_link; system("pause");
        vector<wstring> names = {};
        if (is_group) names = show_groups(CURRENT_SETTINGS.showlines_num);
        else names = showfile(it->second.second, CURRENT_SETTINGS.showlines_num);


        int preexpanded = 0;
        int prehovered = 0;
        bool need_rechoice = false;
        while (true) {
            if (need_rechoice) { //значит основа обновилась, возможно-пересобираем
                if (is_group) names = show_groups(CURRENT_SETTINGS.showlines_num);
                else names = showfile(it->second.second, CURRENT_SETTINGS.showlines_num);
            }
            //second.second -- во втором(элементе-словаре) второй элемент
            //здесь придётся для всех линий найти детей и для всех показать с детьми
            /* как ожидает advansed_chooserC:
            options.children = { {2,{L"излишне длинная ",L"лол это \nлол",L"kkkkkkkkkk"}},
                         {6,{L"----"}}
            };
            */
            
            //for()
            std::map<int, std::vector<std::wstring>> children = prepare_children(it->second.second, CURRENT_SETTINGS.showlines_num);
            
            //для дочерних-отсортированные
            //names[0] = PRINT_TEXTCOLOR::RED + names[0] + PRINT_TEXTCOLOR::RESET; РАБОТАЕТ. ЦВЕТ ДЛЯ ЛЮБОГО текста
            ChoiceResult choice_result = advansed_chooserC({
                .lines_to_choose = names,
                .singleChoice = true,
                .title = L"F1 - изменить основной подсвеченный сейчас элемент\n" 
                          "F2 - изменить дочерние элементы подсвеченного элемента\n"
                          "F3 - добавить элемент\n"
                          "F4 - режим удаления",
                .children = children,
                .childrenMultiplyChoice = true,
                .prehovered = prehovered,
                .preexpanded = preexpanded,
                .menucolors = CURRENT_SETTINGS.MenuColors
                });

            bool change_children = false;
            bool change_and_back = false;
            bool key_pressed = false;
            need_rechoice = false;
            if((int)choice_result.key > 0 and (int)choice_result.key < 5) key_pressed = true;

            switch (choice_result.key) {
            case ShortcutKeys::F1:
                if (is_group or is_script) {//группа или скрипт
                    colorfulPrint(L"ПОКА НЕ ПОДДЕРЖИВАЮТСЯ", PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED); //сейчас питон скриптов - просто нет, а группы совсем другие
                    Sleep(3000);
                    key_pressed = true;
                    need_rechoice = true;
                    break;
                }
                change_lines(it->second.second, global_all_lines[it->second.second][choice_result.hovered].id);
                key_pressed = true;
                need_rechoice = true;
                break;
            case ShortcutKeys::F2:
                change_and_back = true;
                key_pressed = true;
                break;
            case ShortcutKeys::F3:
                need_rechoice = true;
                key_pressed = true;
                manager(1, it->second.second, CURRENT_SETTINGS.path_view_num, false);
                break;
            case ShortcutKeys::F4:
                need_rechoice = true;
                key_pressed = true;
                manager(2, it->second.second, CURRENT_SETTINGS.path_view_num, false);
            }

            if (need_rechoice) {
                prehovered = choice_result.hovered;
                //надо проверить на наличие дочерних, но как...
                if (children.contains(choice_result.hovered) and !children[choice_result.hovered].empty()) {
                    preexpanded = choice_result.hovered;
                }
                continue;
            }

            int line_number = choice_result.hovered+1; //line_num is always 1-based hover
            if (!key_pressed) {
                line_number = choice_result.roots[0];
                if (line_number == EXIT_CODE) return;
            }
           
            FlagsContents old_fl_content = flags_parser(line_parser(it->second.second, line_number, L"").flags);

            std::map<FileType, std::vector<int>> children_flag = old_fl_content.Children;//узнаём какие дети были доступны для выбранной линии
            

            
            

            
            if (change_and_back) {
                
                LineEntry new_line;
                new_line.flags = choose_and_make_flags(global_all_lines[it->second.second][choice_result.hovered].flags, true);
                FileType root_type = it->second.second;
                replace_entity({
                    .type = root_type,
                    .old_line = global_all_lines[root_type][choice_result.hovered],
                    .new_line_entry = new_line,
                    .full = false,
                    .flags = true
                    });
                
            }
            if (change_and_back) {
                
                preexpanded = choice_result.hovered;//hover-0based
                prehovered = choice_result.hovered;
                continue;
            }
            //запуск дочерних
            struct ChildInfo { FileType type; int id; };
            std::vector<ChildInfo> all_children;
            for (const auto& [type, ids] : children_flag) {
                for (int id : ids) {
                    all_children.push_back({ type, id });
                }
            }
            size_t full_size = 0;
            for (int ch1_based : choice_result.children) {
                if (ch1_based < 1 || ch1_based > all_children.size()) continue;

                const auto& child = all_children[ch1_based - 1];
                auto entry = get_entry_by_id(child.type, child.id);
                if (entry != nullptr) {
                    startfilesN(child.type, *entry, "", false);
                }
                //std::cout << "type: " << child.type << ", id: " << child.id << std::endl;
            }
            


            startfiles(it->second.second, line_number, "", false);
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
    
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // Получаем дескриптор вывода

    CONSOLE_FONT_INFOEX fontInfo;
    fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX); // Обязательное поле для WinAPI

    // Заполняем структуру текущими настройками, чтобы не сбросить лишнее
    GetCurrentConsoleFontEx(hConsole, FALSE, &fontInfo);

    // Меняем параметры шрифта
    fontInfo.dwFontSize.X = 8;   // Автоматическая ширина
    fontInfo.dwFontSize.Y = 16;  // Высота шрифта в пикселях (крупный размер лучше для эмодзи)
    fontInfo.FontFamily = FF_DONTCARE;
    fontInfo.FontWeight = FW_NORMAL; // Обычная жирность

    // Копируем имя шрифта. Рекомендуется "Cascadia Code" или "Consolas"
    wcscpy_s(fontInfo.FaceName, L"DejaVu Sans Mono");

    // Применяем изменения
    SetCurrentConsoleFontEx(hConsole, FALSE, &fontInfo);









    start_new_log();
    load_all_files(); //загружаем в память содержимое ВООБЩЕ ВСЕХ файлов с линиями

    
    //tokinizer("test.fq");
    /*maker();
    system("pause");*/
    //в долгий ящик
    //auto as =  std::get<std::wstring> (readFile({ .file_path = "temp.fq",.for_full_read = true,.for_py_code = true,.isVector = false }));
    //std::wstring line = L"";
    //for (size_t i = 0; i < as.size();++i) {
    //    if (as[i] == L' ' or as[i] == L'\t') {
    //        continue;
    //    }
    //    //убрать коменты
    //    else if (as[i] == L'#') {
    //        for (size_t t = i; t < as.size(); ++t) {
    //            if (as[t] == L'\n') {
    //                i = t;
    //                break;
    //            }
    //        }
    //    }
    //    else { line.push_back(as[i]); }
    //}
    //std::wcout << line;
    //system("pause");


    //Sleep(100000);
    //test
    //startfiles(NULL, "", &python, WstringTo_Utf8(get<std::wstring>(readFile("1test.txt", false, true))));
    try {
        //startfiles(NULL, "", &python, WstringTo_Utf8(get<std::wstring>(readFile("1test.txt", false, true))));
        if (argc > 1 and argc != 4) {
            std::string command = argv[1];
            if (command == "--run-group" && argc > 2) {
                int group_id = std::stoi(argv[2]);
                log(L"Запуск группы:" + to_wstring(group_id) + L" по ярлыку");
                startfilesN(FileType::Group, *get_entry_by_id(FileType::Group, group_id), "",false);
                return 0;
            }
        }
        wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (argc > 1 and argc != 4) {
            if (wcscmp(argv[1], L"--Asadmin") == 0) {
                log(L"Запуск чего-то от администратора\n");
                std::vector<std::wstring> Asadmintmp = std::get<std::vector<wstring>>(readFile({ .file_path = getFileName(FileType::Asadmintmp),.isVector = true }));
                
                //Asadmintmp[1] ТЕПЕРЬ ЧИСТАЯ ЛИНИЯ там ДОЛЖНО гарантироваться, что 1-значение-тип именно в контексте FileType, а не map из main, 2-сама линия

                
                FileType type = FileType(stoi(Asadmintmp[0]));
                LineEntry line_entry = line_parser(type, 0, Asadmintmp[1]);
                log(L"линия:"+ Asadmintmp[1] +L"\nтип:"+ Asadmintmp[0]);
                startfilesN(type, line_entry, "",true);
                if (remove((FILE_NAMES.at(FileType::Asadmintmp).c_str())) == 0) { ; }
                else {log(L"Ошибка при удалении asadmintmp после запуска");}
                return 0;
            }
        }
        if (argc == 2 && wcscmp(argv[1], L"--create-task") == 0) {
            // Режим создания задачи: имя и путь переданы как аргументы
            return makeTaskAdmin();
        }
    EnableANSI();
    files_checker(); //после потому-что подуразумивается, что это не 1-ый раз работы прогрммы
    make_txt_for_scripts("scripts\\"); // 1 перезапись файла с путями дальше в add_or_delete
    read_set(); // 1 чтение настроек



    // Меню действий
    menucolors MenuColors;
    //Основной цикл
    while (true) {
        MenuColors = CURRENT_SETTINGS.MenuColors;
        std::vector<wstring> lines = {
                L"Запустить игру",
                L"Запустить программу",
                L"Запустить вашы ссылки",
                L"Запустить вашы cкрипты",
                L"Запуск \033[31mгруппы\033[0m",
                L"Настройки" };
        int choice = advansed_chooserC({
            .lines_to_choose = lines,
            .singleChoice = true,
            .title = L"Ваш вид выбора пути: " + PRINT_TEXTCOLOR::RED + CURRENT_SETTINGS.path_choose_view + PRINT_TEXTCOLOR::RESET + L"\nВаши авто-действия если путь неверный: " + CURRENT_SETTINGS.if_path_wrong + L"\nВыберите действие\n",
            .menucolors = MenuColors
            }).roots[0];
        
        if (choice == EXIT_CODE) { break; }
        switch (choice) {
        case 1: run_application(1); countdown(2, L"Осталось до возвращения: ", 1); break;
        case 2: run_application(2); countdown(2, L"Осталось до возвращения: ", 1); break;
        case 3: run_application(3); countdown(2, L"Осталось до возвращения: ", 1); break;
        case 4: run_application(4); countdown(2, L"Осталось до возвращения: ", 1); break;
        case 5: run_application(5); break;
        case 6: settings(CURRENT_SETTINGS.path_view_num); break;
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
    
    
    return 0;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
