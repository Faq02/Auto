#include <iostream>
#include <string>
#include <locale>
#include <vector>
#include <algorithm>

#include "advanced_choice.h"
#include "win_help.h"
#include "ui_interactions.h"
#include "file_io.h"
#include "data_work.h"
#include "settings.h"
#include "fq-start.h"


void save(std::string& name, std::vector<std::wstring>& current_script) {
    writefile(current_script, name, "", false);
    return;
}
//получает набор заданий
int maker(std::string sname) {
    sname = "scripts\\" + sname;
    fq_maker maker;
    enum class Commands { clickByCoords,Save, execute_fastkeys, sendtext, execute_fastkey,WAIT, start, check, show };
    
	const std::vector<std::wstring> lines_to_choose = {
            L"добавить Клик по координатам",
            L"Закончить",
            L"добавить комбинацию клавиш",
            L"добавить ввод с клавиатуры",
            L"нажать кнопку",
            L"ЗАДЕРЖКА",
            L"запуск чего-то",
            L"проверка",
            L"Показать",
    };
    
    std::vector<std::wstring> current_script = {};
    

    ChoiceResult choice_result;
	while (true) {
        choice_result = advansed_chooserC({
            .lines_to_choose = lines_to_choose,
            .singleChoice = true,
            .title = L"выбери команду для добавления",
            .children = {{
                    7, {L"   └─Из имеющихся", L"   └─Выбор на ПК"} //дочерние для запуска чего-либо
                    }}
            });
        int choice_root = choice_result.roots[0] - 1;
        std::wstring cursor_pos = L"";
        std::vector<std::wstring> fastkeys_split_by_space = {};
        std::wstring fq_faskkeys = L"";
        std::wstring path_to_start = L"";
        bool end = false;
        if (choice_result.roots[0] == -2 or choice_result.roots.empty()) break;
        switch (choice_root) {
        case (int)Commands::clickByCoords:
            cursor_pos = maker.get_cursor_pos();
            if (cursor_pos.empty()) {
                colorfulPrint(L"Получить координаты не удалось", PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
                Sleep(3000);
                break;
            }
            current_script.push_back(L"CLICK " + cursor_pos);
            break;
        case (int)Commands::Save:
            save(sname, current_script);
            end = true;
            break;
        case (int)Commands::execute_fastkeys:
            fastkeys_split_by_space = split(input_line(L"Введите комбинацию через пробел"), L' ');
            fq_faskkeys = L"";
            for (size_t i = 0; i < fastkeys_split_by_space.size(); ++i) {
                if (i != fastkeys_split_by_space.size() - 1) {
                    fq_faskkeys += fastkeys_split_by_space[i] + L'+';
                }
                else fq_faskkeys += fastkeys_split_by_space[i];
            }

            if(!fq_faskkeys.empty()) current_script.push_back(L"fastkeys " + fq_faskkeys);
            break;
        case (int)Commands::sendtext:
            current_script.push_back(L"sendtext " + L'\"' + input_line(L"Введите текст: ") + L'\"');
            break;
        case (int)Commands::execute_fastkey:
            current_script.push_back(L"press_key " + input_word(L"Введите клавишу: "));
            break;
        case (int)Commands::WAIT:
            current_script.push_back(L"WAIT " + input_word(L"Введите время ожидания в секундах(для дробной через точку)\n например 2.4: "));
            break;
        case (int)Commands::start:
        {
            FileType type;
            if (!choice_result.children.empty()) {
                if (choice_result.children[0] == 2) {
                    
                    type = FileType::null;
                    SelectedItem selected_item = select_from_file_or_manual(type, true, false, true, CURRENT_SETTINGS.path_view_num);
                    if (!selected_item.manual_path.empty()) {
                        path_to_start = selected_item.manual_path;
                    }
                }
                else {
                    auto [file_type, id] = ask_lines_in_file_type(L"", true);
                    if(file_type != FileType::null) path_to_start = get_entry_by_id(file_type, id[0])->path;

                }
                if (!path_to_start.empty()) current_script.push_back(L"start " + std::wstring(L"\"") + path_to_start + L'\"');
                break;
            }
            break;
        }
        case (int)Commands::check:
            save(sname, current_script);
            tokinizer(sname);
            //delete?
            break;
        case (int)Commands::show:
            for (auto& line : current_script) {//пока самый простой вариант без перевода обратно
                std::wcout << line << std::endl;
            }
            system("pause");
            break;
        }
        if (end == true) {
            break;
        }
	}
}