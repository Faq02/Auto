#pragma once
#include <variant>;
#include "app_config.h"

int writefile(std::wstring line, std::string file_path, std::string prog_name = "", bool show = true);
int writefile(std::vector<std::wstring> lines, std::string file_path, std::string prog_name = "", bool show = true);

struct ReadOptions {
    std::string file_path;
    bool for_full_read = false;
    bool for_py_code = false;
    bool isVector = false;
};
std::variant<std::wstring, std::vector<std::wstring>> readFile(ReadOptions options);
void make_txt_for_scripts(std::string directory_path);
std::wstring choose_line(short line_number, FileType type, bool raw = false);
int delete_lines_or_insert_or_add_one(FileType type, std::vector<int> numbers, bool insert, std::wstring line_to_insert, short line_number, bool show, bool add);
void mass_files_delete(std::vector<int> num_of_indxes, FileType type);