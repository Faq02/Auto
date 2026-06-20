#include <iostream>
#include <windows.h>
#include <string>
#include <locale>
#include <vector>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <conio.h> // для ожидания ввода буквы
#include <codecvt>
#include <fcntl.h>
#include <io.h>
#include <shellapi.h>
#include <sstream>

#include "advanced_choice.h"
#include "resource.h"
#include "StartFuncs.h"
#include "app_config.h"
#include "file_io.h"
#include "converter.h"
#include "ui_interactions.h"
#include "path_handler.h"
#include "settings.h"
#include "Manager.h"
#include "data_work.h"



std::string GetPythonTemplateUTF8(int Template) {
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(Template), L"PYTHONSCRIPT");
    if (!hRes) return "";
    HGLOBAL hData = LoadResource(NULL, hRes);
    if (!hData) return "";
    DWORD size = SizeofResource(NULL, hRes);
    char* data = (char*)LockResource(hData);
    // Предполагаем, что ресурс сохранён в UTF-8
    return std::string(data, size);
}



class PythonScriptBuilder {
private:
    std::wstring code;
    std::wstring coords;

public:
    void read_coords() {
        std::wifstream fin("coords.txt"); //не работает при отладке, так как .exe в релизах и файл коордиат создаётся там-же
        std::wstring line = L"";
        std::getline(fin, line);
        fin.close();
        coords = line;
    }
    void get_coords(PythonRuntime& python) {
        std::wcout << L"Наведите курсор на нужное место и нажмите E..." << std::endl;

        // Ожидание любой клавиши
        _getch();

        
        std::string template_utf8 = GetPythonTemplateUTF8(IDR_PYTHONSCRIPT2);
        if (template_utf8.empty()) {
            std::wcerr << L"Template is empty!"; Sleep(10000);
            return;
        }
        /*std::wcout << StringToWstring(template_utf8); Sleep(10000);*/
        startfiles(FileType::null,NULL, &python, template_utf8,false);

        Sleep(1000);
        read_coords();

        if (!coords.empty()) {
            std::wcout << L"Координаты получены: " << coords << std::endl;
        }
        else {
            std::wcout << L"Не удалось получить координаты!" << std::endl;
        }
    }
    void add_delay(float seconds) {
        code = code + L"time.sleep(" + std::to_wstring(seconds) + L")" + L"\n";
    }
    void add_click_with_coords() { //у click() оч. много параметров => ?
        /*if (!has_coords) {
            std::wcout << L"Сначала получите координаты!" << std::endl;
            return;
        }*/
        code += L"pyautogui.click(" + coords + L")\n";
    }
    void add_click_current_expected_pos() {
        code += L"pyautogui.click()\n"; // клик где сейчас курсор
    }
    void move_mouse() {
        code += L"pyautogui.moveTo(" + coords + L")\n";
    }
    void add_hotkey(std::vector<std::wstring> hotkeys) {
        std::wstring inScobe = L"";
        for (size_t i = 0; i < hotkeys.size(); ++i) {
            inScobe += L"'" + hotkeys[i] + L"'";
            if (i < hotkeys.size() - 1) {
                inScobe += L",";
            }
        }
        code += L"pyautogui.hotkey(" + inScobe + L")\n"; 
    }
    void write(std::wstring text) {
        code += L"keyboard.write('" + text + L"')\n";  
    }
    void press(std::wstring key) {
        code += L"pyautogui.press('" + key + L"')\n";  
    }
    void save(std::string prog_name) {
        std::string full_path = "scripts\\" + prog_name + ".py";
        writefile(code, full_path, prog_name, false);
    }
    void standart_lines() {
        code += L"import pyautogui\nimport time\nimport keyboard\nimport os\n";
    }
    std::wstring end() {
        return code;
    }
    void check(PythonRuntime& python) {
        // Удаление стандартных 4 линий
        std::string temp_filename = "1temp_script_" + std::to_string(time(NULL)) + ".py";
        size_t pos = 0;
        for (int i = 0; i < 3; ++i) {
            size_t next_n = code.find(L'\n', pos);
            if (next_n == std::wstring::npos) {
                code.clear();
                break;
            }
            pos = next_n + 1;
        }
        if (!code.empty() && pos > 0 && pos < code.length()) {
            code.erase(0, pos);
        }

        //Загружаем шаблон
        std::string template_utf8 = GetPythonTemplateUTF8(IDR_PYTHONSCRIPT1);
        if (template_utf8.empty()) {
            std::wcerr << L"Template is empty!" << std::endl;
            return;
        }
        //Получаем HWND
        HWND hWnd = GetConsoleWindow();
        std::string hwnd_str = std::to_string(reinterpret_cast<uintptr_t>(hWnd));

        //Заменяем плейсхолдеры в UTF-8 строке
        size_t hwndPos = template_utf8.find("{{HWND_PLACEHOLDER}}");
        if (hwndPos != std::string::npos) {
            template_utf8.replace(hwndPos, 20, hwnd_str);
            std::wcout << L"HWND replaced: " << StringToWstring(hwnd_str) << std::endl;
        }
        size_t codePos = template_utf8.find("{{USER_CODE_PLACEHOLDER}}");
        if (codePos != std::string::npos) {
            //Конвертируем пользовательский код
            std::string userCodeUtf8 = WstringTo_Utf8(code);
            template_utf8.replace(codePos, 25, userCodeUtf8);
            std::wcout << L"User code inserted, size: " << code.size() << L" chars" << std::endl;
        }
        startfiles(FileType::null,NULL, &python, template_utf8,false);
        
        
    }

    void choose_program_to_start(PythonRuntime& python) { //сразу TODO-добавть выбор способа старта
        int choice = advansed_chooser({ .lines_to_choose = {L"Игры", L"Программы", L"Ссылки", L"Скрипты", L"Выбрать на пк или ввести путь"},
            .singleChoice = true,
            .title = L"Выберите что запускать:" })[0];
        std::wstring path = L"";
        std::vector<std::wstring> case_3_lines_to_choose = {};
        int ch = 0;
        std::vector<int> ch_vec = {};
        std::vector<std::wstring> lines = {};
        //меняет path в зависимости от выбора пользователя
        auto select_path = [](std::wstring& path, FileType type,std::wstring cancel_prompt,int seconds_to_wait = 1,bool allow_manual = true) { 
            SelectedItem item = select_from_file_or_manual(type, allow_manual, false, false);
            if (item.cancelled) {
                colorfulPrint(cancel_prompt, PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
                countdown(seconds_to_wait, L"Осталось для возвращения ", 1);
                return false;
            }
            path = item.path;
            return true;
        };


        switch (choice) {
            //дальше - код из групп с убранными вариантами с флагами
        case 1: if(!select_path(path, FileType::Game, L"\nОтмена")) return; break;
        case 2: if(!select_path(path, FileType::Program, L"\nОтмена")) return; break;
        case 3:
            case_3_lines_to_choose = get<std::vector<std::wstring>>(readFile({ .file_path = FILE_NAMES.at(FileType::Link), .isVector = true }));
            case_3_lines_to_choose.push_back(L" ");
            case_3_lines_to_choose.push_back(L"Написать самому");
            ch_vec = advansed_chooser({
                .lines_to_choose = case_3_lines_to_choose,
                .singleChoice = true,
                .title = L"Введите номер ссылки:\n" });

            if (ch_vec.empty() or ch_vec[0] == case_3_lines_to_choose.size()-1) { std::wcout << L"у вас нету ссылок для выбора\n"; ch = case_3_lines_to_choose.size(); }
            else { ch = ch_vec[0]; }
            if (ch == case_3_lines_to_choose.size()) path = input_line(L"Вводите:");
            else { path = choose_line(ch, FileType::Link); }
            break;

        case 4: if(!select_path(path, FileType::Script, L"\nУ тебя нету скриптов или отмена", 3, false)) return; break;
        case 5:
            path = choose_file_on_pc(CURRENT_SETTINGS.path_view_num, FileType::Program);//здесь FileType::Program, 2 просто заглушки, т.к не хочу менять функцию
            if (path.empty()) return;
            //при отмене кстати пишет пустую строку-плохо, но не критично, мне пока лень
            break;
        default:
            return;
        }
        if (choice == 4 or choice == 3) { //ссылка или скрипт
            code += L"os.system(r\'\"start " + path + L"\"\')\n";
        }
        else { code += L"os.system(r\'\"" + path + L"\"\')\n"; }
        
    }

    void show_script() {
        std::vector<std::wstring> scr_insides_lines = split(code,L'\n');
        for (auto line : scr_insides_lines) {
            std::wcout << line << L'\n';
        }
        int state = 0;
        bool need_translate = true;
        while(true) {
            state = show_scripts(state, scr_insides_lines, need_translate);
            if (state == 1 or state == -1) break;
            if (state == 2) need_translate = !need_translate;
        }
    }
};



std::wstring python_script_make(std::string prog_name, bool from_changer = false, PythonRuntime* python=nullptr) {
    //std::wcout << L"приветствую в создателе скриптов питона на основе библиотеки pyautogui!\nОЧЕНЬ ВАЖНО -- добавляй задержку! (можно не всегда, но надо думать, иначе программа(на которую скрипт) не успеет отреагировать)\n";
    //std::wcout << L"ОЧЕНЬ ВАЖНО -- добавляй задержку! (можно не всегда, но надо думать, иначе программа(на которую скрипт) не успеет отреагировать)\n";
    PythonScriptBuilder build;
    if (!from_changer) { build.standart_lines(); }
    bool end = false;
    int count_escape = 0;
    std::wstring line;
    while (true) {
        float sec = 0;
        int choice = advansed_chooser({ 
            .lines_to_choose = { L"добавить Клик по координатам",
            L"добавить Клик без координат(на предпологаемом расположении мышки)",
            L"передвинуть мышку на координаты",
            L"добавить комбинацию клавиш",
            L"добавить ввод с клавиатуры",
            L"нажать кнопку",
            L"ЗАДЕРЖКА",
            L"запуск чего-то(выполняется консолью)",
            L"проверка",
            L"Показать",
            from_changer ? L" " : L"или 0-сохранить(закончить)"},
            .singleChoice = true, 
            .title = L"приветствую в создателе скриптов питона на основе библиотеки pyautogui!\nОЧЕНЬ ВАЖНО -- добавляй задержку! (можно не всегда, но надо думать, иначе программа(на которую скрипт) не успеет отреагировать)\n" })[0];
        std::wstring lime = L"";
        std::vector<std::wstring> hotkeys = {};
        switch (choice) {
            case 1:
                if (python != nullptr) {
                    build.get_coords(*python);
                    build.add_click_with_coords();
                    break;
                }
                break;
            case 2:
                build.add_click_current_expected_pos();
                break;
            case 3:
                build.move_mouse();
                break;
            case 4:
                std::wcout << L"Напишите ваши клавиши через пробел(например: \"ctrl c\" или: \"alt tab\")\n";
                hotkeys = make_massive_of_wstr();
                build.add_hotkey(hotkeys);
                break;
            case 5:
                std::wcout << L"Напиши то, что хочешь вводить:\n";
                std::getline(std::wcin, lime);
                build.write(lime);
                break;
            case 6:
                std::wcout << L"Введи кнопку, которую хочешь нажимать в скрипте\n";
                std::getline(std::wcin, lime);
                build.press(lime);
                break;
            case 7:
                sec = std::stof(input_word(L"в секундах (например: 0.2)"));
                build.add_delay(sec);
                break;
            case 8:
                build.choose_program_to_start(*python);
                break;
            case 9:
                if (python != nullptr) {
                    build.check(*python);
                    break;
                }
                break;
            case 10:
                build.show_script();
                break;
            case 0:
                if (from_changer) {
                    continue;
                    break;
                }
                build.save(prog_name);
                end = true;
                break;
            case 11:
                if (from_changer) {
                    continue;
                    break;
                }
                std::wcout << L"Сохраняем\n";
                build.save(prog_name);
                end = true;
                break;
            case -1:
                if (!from_changer) {
                    if (count_escape < 2) { std::wcout << L"СОХРАНИТЬ? при последующем попадении сюда будет УДАЛЕНИЕ текущего скрипта\n\n"; }
                    count_escape++;
                    if (count_escape >= 2) {
                        end = true;
                        std::wcout << L"Я ПРЕДУПРЕЖДАЛ!\n\n";
                        break;
                    }
                    countdown(5, L"до возвращения: ", 0);
                    break;
                }
                break;
        }
        if (true == end) {
            break;
        }
        if (from_changer) {
            line = build.end();
            return line;
            break;
        }
    }
    return L"0";
}