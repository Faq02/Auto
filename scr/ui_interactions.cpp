#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <windows.h>
#include <filesystem>
#include <thread>
#include "ui_interactions.h"
#include <map>
#include "app_config.h"
#include "Manager.h"
#include "settings.h"
#include "advanced_choice.h"
#include "data_work.h"
#include "file_io.h"
#include "converter.h"
#include "path_handler.h"
static constexpr auto EXIT_CODE = -1;

std::vector<std::wstring> make_massive_of_wstr() {
    std::vector<std::wstring> wideStrings;
    std::wstring line;

    // Считываем всю строку до Enter
    if (std::getline(std::wcin, line)) {
        std::wistringstream iss(line);
        std::wstring tempStr;

        // Разбиваем строку на слова
        while (iss >> tempStr) {
            wideStrings.push_back(tempStr);
        }
    }
    return wideStrings;
}

void countdown(double seconds_to_wait, std::wstring wstring_to_show, int duration) {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    std::wcout << std::fixed << std::setprecision(duration);
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - start;
        double remaining = seconds_to_wait - elapsed.count();

        if (remaining <= 0.0) {
            GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
            cursorInfo.bVisible = TRUE;
            SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
            break;
        }
        std::wcout << L"\r" << wstring_to_show << remaining;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

int additional_option_logic(std::map<short, std::pair<std::wstring, FileType>>::const_iterator it, int option, size_t orig_lines_count, PythonRuntime& python, short app_type) {
    enum class OPTION {
        ADD = 1, SHOW_ONE_TRANSLATED = 2
    };
    enum class SHOW_ONE_STATE {
        START = 1, SHOW_ANOTHER = 2, CHANGE_TRANSLATE = 3
    };
    bool need_translate = true;
    std::vector<std::wstring> names = {};
    int state = (int)SHOW_ONE_STATE::SHOW_ANOTHER;
    std::wstring scr_path = L"";
    std::string scr_name = "";
    std::vector<std::wstring> scr_insides_lines = {};
    std::wstring scr_title = L"";
    int line_number = 0;
    switch (option) {
    case (int)OPTION::ADD:
        manager(1, it->second.second, CURRENT_SETTINGS.path_view_num,false,0,&python);
        break;
    case (int)OPTION::SHOW_ONE_TRANSLATED:
        //цикл показа и возможно запуска
        while (true) {
            if (state == (int)SHOW_ONE_STATE::CHANGE_TRANSLATE) {
                need_translate = !need_translate;
            }
            if (state == (int)SHOW_ONE_STATE::START) {
                startfiles(it->second.second, line_number, &python, "", false);
                return 0;
            }


            if (state == (int)SHOW_ONE_STATE::SHOW_ANOTHER or state == EXIT_CODE or state == (int)SHOW_ONE_STATE::CHANGE_TRANSLATE) {
                scr_title = L"";
                if (state != (int)SHOW_ONE_STATE::CHANGE_TRANSLATE) {
                    names = showfile(it->second.second, CURRENT_SETTINGS.showlines_num);
                    line_number = advansed_chooser({
                        .lines_to_choose = names,
                        .singleChoice = true,
                        .title = L"Выберите " + it->second.first + L" для показа\n" })[0];
                    if (line_number == EXIT_CODE) return EXIT_CODE;

                    scr_path = choose_line(line_number, it->second.second, true);
                    scr_name = WstringTo_Utf8(extract_filename(scr_path));
                }
                scr_insides_lines = std::get<std::vector<std::wstring>>(readFile({ .file_path = "scripts\\" + scr_name,.isVector = true })); //не переведённые
                if (need_translate == true) scr_insides_lines = translate_script_insides(scr_insides_lines); //перевод на русский
                for (auto& line : scr_insides_lines) {
                    scr_title += line + L"\n";
                }
                state = advansed_chooser({
                    .lines_to_choose = {L"Запустить", L"Сменить(вернуться)", (need_translate == true ? L" Показать без перевода" : L"Показать перевод")},
                    .singleChoice = true,
                    .title = scr_title })[0];
            }
        }
    }
    return 0;
}