#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <windows.h>
#include <filesystem>
#include <thread>
#include <map>
#include <tuple>

#include "app_config.h"
#include "Manager.h"
#include "settings.h"
#include "advanced_choice.h"
#include "data_work.h"
#include "file_io.h"
#include "converter.h"
#include "path_handler.h"
#include "StartFuncs.h"
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


int show_scripts(int state, std::vector<std::wstring> scr_insides_lines, bool need_translate, bool from_scr_start) {
    
    std::vector<std::wstring> lines_to_choose = { L"Вернуться",(need_translate == true ? L" Показать без перевода" : L"Показать перевод") };
    if (from_scr_start) {
        lines_to_choose.pop_back();
        lines_to_choose.pop_back();
        //оставляю внизу, чтобы было как раньше-смена перевода последняя опция
        lines_to_choose.push_back(L"Запустить");
        lines_to_choose.push_back(L"Сменить(вернуться)");
        lines_to_choose.push_back((need_translate == true ? L" Показать без перевода" : L"Показать перевод"));
    }
    std::wstring scr_title = L"";
    if (need_translate == true) scr_insides_lines = translate_script_insides(scr_insides_lines); //перевод на русский-пользовательский
    for (auto& line : scr_insides_lines) {
        scr_title += line + L"\n";
    }
    state = advansed_chooser({
        .lines_to_choose = lines_to_choose,
        .singleChoice = true,
        .title = scr_title })[0];
    return state;
}




int additional_option_logic(std::map<short, std::pair<std::wstring, FileType>>::const_iterator& it, int option, size_t orig_lines_count, short app_type) {
    
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
        manager(1, it->second.second, CURRENT_SETTINGS.path_view_num,false);
        break;
    case (int)OPTION::SHOW_ONE_TRANSLATED:
        //цикл показа и возможно запуска
        while (true) {
            if (state == (int)SHOW_ONE_STATE::CHANGE_TRANSLATE) {
                need_translate = !need_translate;
            }
            if (state == (int)SHOW_ONE_STATE::START) {
                startfiles(it->second.second, line_number, "", false);
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

                    scr_path = global_all_lines[FileType::Script][line_number-1].path; //предпологается, что names по размеру такой-же что и vector<LineEntry>
                    std::wstring sdf; sdf = extract_filename(scr_path);
                    scr_name = WstringTo_Utf8(sdf);
                }
                scr_insides_lines = std::get<std::vector<std::wstring>>(readFile({ .file_path = "scripts\\" + scr_name, .for_full_read = true, .isVector = true })); //не переведённые
                state = show_scripts(state, scr_insides_lines, need_translate, true);
            }
        }
    }
    return 0;
}

void colorfulPrint(std::wstring prompt, std::wstring text_color, std::wstring background_color) {
    constexpr std::wstring_view RESET = L"\033[0m";
    std::wstring massage = std::format(L"{}{}{}{}\n", text_color, background_color, prompt, RESET);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == NULL || hConsole == INVALID_HANDLE_VALUE) {
        return;
    }
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), massage.c_str(), massage.size(), nullptr, NULL);
}


SelectedItem select_from_file_or_manual(FileType type, bool allow_manual, bool allow_flags, bool force_manual, std::wstring path_choose_view_num) {
    SelectedItem result;
    result.cancelled = false;
    auto& lines = global_all_lines[type];
    if (!force_manual) {
        // Удаляем пустые строки (если есть)

        if (!lines.empty()) {
            auto chosen = advansed_chooser({
                .lines_to_choose = showfile(type,CURRENT_SETTINGS.showlines_num),
                .singleChoice = true,
                .title = L"Выберите номер:"
                })[0];
            if (chosen == EXIT_CODE) {
                result.cancelled = true;
                return result;
            }
            if (chosen >= 1 && chosen <= lines.size()) {
                result.id = lines[chosen-1].id;
            }
            else {
                result.cancelled = true;
                return result;
            }
        }
        else if (!allow_manual) {
            // Список пуст, а ручной ввод не разрешён – отмена
            result.cancelled = true;
            return result;
        }
    }

    // Если путь ещё не получен (список был пуст или force_manual), запускаем ручной ввод
    if (force_manual or (result.manual_path.empty() && allow_manual)) {
        // Здесь может быть выбор: ввести строку или выбрать файл на ПК
        // Для простоты – только ввод строки
        if (type == FileType::Link) path_choose_view_num = L"1";
        result.manual_path = choose_file_on_pc(path_choose_view_num,type,0);
        if (result.manual_path == L"") {
            result.cancelled = true;
            return result;
        }
    }

    if (allow_flags && !result.cancelled) {
        result.flags = choose_and_make_flags(lines[lines.size()-1].flags);
    }
    return result;
}

std::tuple<FileType,std::vector<int>> ask_lines_in_file_type(std::wstring prompt, bool single_line) {
    int type = advansed_chooser({
            .lines_to_choose = {L"Игры", L"Программы", L"Ссылки", L"Cкрипты", L"\033[31mГруппы\033[0m"},
            .singleChoice = true,
            .title = prompt.empty() ? L"Выберите тип(SCAPE для выхода):" : prompt
        })[0] - 1;//они 0-based, так games это 0
    if (type == -2) { //-2 так как если escape то возвращается -1
        return std::make_tuple(FileType::null, std::vector<int>{-1});
    }
    FileType file_type = static_cast<FileType>(type);
    //добавить доп опцию "добавить новую" ибо мне лень лазить по своей-же программе

    return std::make_tuple(file_type, advansed_chooser({
        .lines_to_choose = showfile(file_type,CURRENT_SETTINGS.showlines_num),//показываем согласно настройкам
        .singleChoice = single_line,
        .title = L"Выбирите линии: "
        }));
    //очень sus код
}



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