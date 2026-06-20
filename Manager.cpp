#include <string>
#include <vector>

#include "StartFuncs.h"
#include "Groups.h"
#include "python_script_maker.h"
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

std::vector<int> ask_lines_to_delete(std::string file_path) {
    std::vector<std::wstring> lines_to_show;
    if (file_path == FILE_NAMES.at(FileType::Group)) {
        lines_to_show = show_groups(CURRENT_SETTINGS.showlines_num);
    }
    else { lines_to_show = std::get<std::vector<std::wstring>>(readFile({ .file_path = file_path, .isVector = true })); }
    
    return advansed_chooser({
        .lines_to_choose = lines_to_show,
        .singleChoice = false,
        .title = L"Выберите номера удаляемых скриптов\n" });
}

class Manager {
private:
    std::string file_path;
    int mode;
    int prog_view;
    FileType type;
    PythonRuntime* python;
    bool from_start_files;
    short line_num;
public:
    void set_private(FileType Type, int Mode, const std::wstring prog_vie, bool from_start_fileS = false, short line_nuM = NULL, PythonRuntime* pythoN = nullptr) {
        file_path = getFileName(Type);
        mode = Mode;
        prog_view = stoi(prog_vie);
        type = Type;
        python = pythoN;
        from_start_files = from_start_fileS;
        line_num = line_nuM;
    }
    void loger() {
        log(L"Manager called\nfile_path:" + StringToWstring(file_path)
        + L"mode:" + std::to_wstring(mode) + L"\n"
        + L"prog_view:" + std::to_wstring(prog_view) + L"\n"
        + L"from_start_files:" + std::to_wstring(from_start_files) + L"\n"
        + L"line_num:" + std::to_wstring(line_num) + L"\n"
        );
    }

    void groups() {
        int choice;
        switch (mode) {
        case (int)MODE::ADD:
            group_add(std::to_wstring(prog_view));
            return;


        case (int)MODE::DEL:
            choice = advansed_chooser({
                .lines_to_choose = { L"всю группу", L"её некоторые внутренности" },
                .singleChoice = true,
                .title = L"" })[0];

            switch (choice)
            {
            case 1: { delete_lines_or_insert_or_add_one(type, ask_lines_to_delete(file_path), false, L"", NULL, false, false); break; }
            case 2: { group_del(); break; }
            }
            break;
        
        case (int)MODE::CHANGE:
            changer(type, nullptr);
            return;
        }

    }


    void add_script_logic() {
        std::wstring prog_name = input_word(L"Введите название скрипта");
        std::string pr_name = WstringTo_Utf8(prog_name);
        if (python != nullptr) {
            python_script_make(pr_name, false, python);
            return;
        }
        else {
            log(L"с питоном то проблемки...\n");
            return;
        }
    }

    void scripts() {
        std::wstring prog_name;
        std::string pr_name;
        switch (mode) {
        case (int)MODE::ADD:
            add_script_logic();
            return;

        case (int)MODE::DEL:
            mass_files_delete(ask_lines_to_delete(file_path), type);
            return;

        case (int)MODE::CHANGE:
            changer(type, python);
            return;

        case (int)MODE::ADDwithFLAGS:
            add_script_logic();
            add_with_flags_logic();
            break;
        }
    }


    int add_entry()
    {
        if (from_start_files) {
            auto pr_view = std::to_wstring(prog_view);
            std::wstring new_path = choose_file_on_pc(pr_view, type);
            if (!new_path.empty()) {
                delete_lines_or_insert_or_add_one(type, {}, true, new_path, line_num, false, false);
                startfiles(type, line_num, nullptr, "",false);
                return EXIT_CODE;
            }
            else { return 0; }
        }
        if (prog_view == (int)SETTINGS::path_choose_view::console_standart || type == FileType::Link)
        {
            std::wstring lime = input_line(L"Что записать в файл?\n");
            std::vector<std::wstring> lines = split_lineOn_paths(lime);
            writefile(lines, file_path);
        }
        else
        {
            auto result = Win_path_selector();
            if (std::get<0>(result))
                writefile(std::get<1>(result), file_path);
            else
                If_prog_view_is_2_and_false();
                return EXIT_CODE;
            
        }
    }
    void add_with_flags_logic() {
        if (type != FileType::Script) {
            if (add_entry() == EXIT_CODE) return;
        }
        if (type == FileType::Script) { make_txt_for_scripts("scripts\\"); }//переделываем txt файл после изменения
        std::vector<std::wstring> paths = get<std::vector<std::wstring>>(readFile({ .file_path = file_path,.for_full_read = false,.for_py_code = false,.isVector = true }));
        LineEntry entry;
        entry = line_parser(type, paths.size(), L"");
        std::wstring flags = make_flags();
        std::wstring name = entry.name;
        if (entry.name.empty()) { name = extract_filename(entry.path); } // нету имени - берём .exe
        delete_lines_or_insert_or_add_one(type, {}, true, (entry.path + L"\"" + name + L"\"" + flags), paths.size(), false, false);
    }
    void games_links_progs() {
        switch (mode) {
        case (int)MODE::ADD:add_entry(); break;
        case (int)MODE::DEL: delete_lines_or_insert_or_add_one(type, ask_lines_to_delete(getFileName(type)), false, L"", NULL, false, false); break;
        case (int)MODE::CHANGE: changer(type, python); break; //возможно стоило питон то убрать для такого варианта...
        case (int)MODE::ADDwithFLAGS: 
            add_with_flags_logic();
            break;
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


void manager(short mode, const FileType type, const std::wstring prog_view, bool from_start_files = false, short line_num = NULL, PythonRuntime* python = nullptr) {
    Manager Class;
    Class.set_private(type, mode, prog_view, from_start_files, line_num, python);
    Class.loger();
    Class.start();
}