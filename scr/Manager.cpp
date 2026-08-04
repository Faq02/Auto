#include <string>
#include <vector>

#include "StartFuncs.h"
#include "Groups.h"
#include "advanced_choice.h"
#include "file_io.h"
#include "Changer.h"
#include "path_handler.h"
#include "converter.h"
#include "win_help.h"
#include "app_config.h"
#include "logger.h"
#include "data_work.h"
#include "settings.h"
#include "ui_interactions.h"
#include "ui_maker.h"



static constexpr auto EXIT_CODE = -1;
enum class MODE {
    ADD = 1, DEL = 2, CHANGE = 3, ADDwithFLAGS = 4
};

namespace SETTINGS {
    enum class path_choose_view {
        console_standart = 1,
        win_path = 2
    };
}

std::vector<int> ask_lines_to_delete(FileType type) {
    std::vector<std::wstring> lines_to_show;
    if (type == FileType::Group) {
        lines_to_show = show_groups(CURRENT_SETTINGS.showlines_num);
    }
    else { lines_to_show = showfile(type, CURRENT_SETTINGS.showlines_num); }
    
    return advansed_chooser({
        .lines_to_choose = lines_to_show,
        .singleChoice = false,
        .title = L"Выберите номера для удаления\n" });
}





/*
int max_id = 0;

for (auto& line : all_lines)
{
    max_id = std::max(max_id, line.id);
}


*/



class Manager {
private:
    int mode;
    int prog_view;
    FileType type;
    bool from_start_files;
public:
    void set_private(FileType Type, int Mode, const std::wstring prog_vie, bool from_start_fileS = false) {
        mode = Mode;
        prog_view = stoi(prog_vie);
        type = Type;
        from_start_files = from_start_fileS;
    }
    void loger() {
        log(L"Manager called\nmode:" + std::to_wstring(mode) + L"\n"
        + L"prog_view:" + std::to_wstring(prog_view) + L"\n"
        + L"from_start_files:" + std::to_wstring(from_start_files) + L"\n"
        );
    }

    void groups() {
        int choice;
        std::vector<int> positions_to_delete;
        switch (mode) {
        case (int)MODE::ADD:
            group_add(std::to_wstring(prog_view),false);
            return;


        case (int)MODE::DEL:
            //choice = advansed_chooser({ //возможно лишний выбор
            //    .lines_to_choose = { L"всю группу", L"её некоторые внутренности" },
            //    .singleChoice = true,
            //    .title = L"" })[0]; 
            positions_to_delete = ask_lines_to_delete(FileType::Group);
            delete_entries_by_positions(FileType::Group, positions_to_delete);
            break;
        
        case (int)MODE::CHANGE:
            changer(type);
            return;
        }

    }


    void add_script_logic() {
        std::wstring prog_name = input_word(L"Введите название скрипта");
        std::string pr_name = WstringTo_Utf8(prog_name);
        maker(pr_name + ".fq");
        return;
    }

    void scripts() {
        std::wstring prog_name;
        std::string pr_name;
        std::vector<int> positions;
        switch (mode) {
        case (int)MODE::ADD:
            add_script_logic();
            return;

        case (int)MODE::DEL:
            positions = ask_lines_to_delete(type);
            mass_files_delete(positions, type); //сначало файлы
            delete_entries_by_positions(type, positions);//затем txt подтягиваем
            return;

        case (int)MODE::CHANGE:
            changer(type);
            return;

        case (int)MODE::ADDwithFLAGS:
            add_script_logic();
            add_entry(true);
            break;
        }
    }


    int add_entry(bool allow_flags = false) {
        int new_id = get_max_id(type)+1;
        //это короче если путь не верный(при запуске) мы даём выбрать и создаём новый
        /*if (from_start_files) {
            auto pr_view = std::to_wstring(prog_view);
            std::wstring new_path = choose_file_on_pc(pr_view, type);
            if (!new_path.empty()) {
                new_path += standart_end + std::to_wstring(new_id);
                delete_lines_or_insert_or_add_one(type, {}, true, new_path, line_num, false, false);
                startfiles(type, line_num, nullptr, "",false);
                return EXIT_CODE;
            }
            else { return 0; }
        }*/
        if (type == FileType::Script and allow_flags == true) {
            LineEntry scr_entry = global_all_lines[type][global_all_lines[type].size()-1];
            scr_entry.flags = choose_and_make_flags(scr_entry.flags);
            return 0;
        }
        
        SelectedItem selected_item = select_from_file_or_manual(type, true, allow_flags, true, std::to_wstring(prog_view));

        if (selected_item.cancelled) return -1;
        if (!selected_item.manual_path.empty()) {
            LineEntry new_line;
            std::vector<std::wstring> manual_paths = split_lineOn_paths(selected_item.manual_path);
            std::vector<LineEntry> vec_entry = {};
            for (const auto& path : manual_paths) {
                new_line.path = selected_item.manual_path;
                if (!selected_item.flags.empty()) new_line.flags = selected_item.flags;
                new_line.name = L"";
                new_line.id = new_id;
                new_id++;
                vec_entry.push_back(new_line);
            }
            add_entries(type, vec_entry);
            return 0;
        }

        return 0;


        //if (prog_view == (int)SETTINGS::path_choose_view::console_standart || type == FileType::Link)
        //{
        //    std::wstring lime = input_line(L"Что записать в файл?\n");
        //    std::vector<std::wstring> lines = split_lineOn_paths(lime); //для  FileType::Link не работает-естественно
        //    int cnt = 0;
        //    for (auto& linne : lines) {
        //        linne += standart_end + std::to_wstring(new_id+cnt);
        //        ++cnt;//записываем подряд, поэтому обычный счёт пойдёт
        //    }
        //    writefile(lines, file_path);
        //}
        //else
        //{
        //    auto result = Win_path_selector();
        //    if (std::get<0>(result))
        //        writefile(std::get<1>(result) + standart_end + std::to_wstring(new_id), file_path);
        //    else
        //        If_prog_view_is_2_and_false();
        //        return EXIT_CODE;
        //    
        //}
    }
    void games_links_progs() {
        std::vector<int> positions = {};
        switch (mode) {
        case (int)MODE::ADD:add_entry(); break;
        case (int)MODE::DEL:
            positions = ask_lines_to_delete(type);
            delete_entries_by_positions(type, positions); break;
        case (int)MODE::CHANGE: changer(type); break; //возможно стоило питон то убрать для такого варианта...
        case (int)MODE::ADDwithFLAGS: add_entry(true); break;
        }
    }
    void start() {
        if (type == FileType::Script)
            return scripts();
        if (type == FileType::Group)
            return groups();
        return games_links_progs();
    }
};


void manager(short mode, const FileType type, const std::wstring prog_view, bool from_start_files = false) {
    Manager Class;
    Class.set_private(type, mode, prog_view, from_start_files);
    Class.loger();
    Class.start();
}