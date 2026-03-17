#pragma once 
#include <string>
#include <iostream>
#include <windows.h>
#include <vector>
#include <tuple>
#include <variant>
#include "StartFuncs.h"

std::tuple<bool, std::wstring> Win_path_selector();
void If_prog_view_is_2_and_false();
std::wstring choose_file_on_pc(std::wstring& path_choose_view, short app_type);
std::wstring chooser(const std::wstring& what_choice);
void countdown(double seconds_to_wait, std::wstring wstring_to_show, int duration);
std::wstring extract_filename(const std::wstring& path);
std::vector<std::wstring> make_massive_of_wstr();
int is_valid_file_type(const std::wstring& path);
int writefile(std::wstring line, std::string file_type, std::string prog_name, bool show);
int writefile(std::vector<std::wstring> lines, std::string file_type, std::string prog_name,bool show);
struct ReadOptions {
    std::string file_type;
    bool for_full_read = false;
    bool for_py_code = false;
    bool isVector = false;
};
std::variant<std::wstring, std::vector<std::wstring>> readFile(ReadOptions options);

void make_txt_for_scripts(std::string directory_path);
std::wstring choose_line(short line_number, std::string file_type);
struct LineEntry {
    std::wstring path;
    std::wstring name;
    std::wstring flags;
};

LineEntry line_parser(short line_number, std::string file_type,std::wstring raw_line);
std::vector<LineEntry> file_parser(std::string file_type);
std::vector<std::wstring> showfile(std::string file_type, std::wstring setting);
std::wstring StringToWstring(const std::string& str);
int makeTaskAdmin(std::wstring name, std::wstring path);
int delete_lines_or_insert_or_add_one(std::string file_type, std::vector<int> numbers, bool insert, std::wstring line_to_insert, short line_number, bool show, bool add);
void files_checker();
std::string WstringTo_Utf8(const std::wstring& wstr);
void func_linker(short mode, const std::string& file_type, const std::wstring prog_view, bool from_start_files, short line_num, PythonRuntime* python);
