#pragma once 
#include <string>
#include <iostream>
#include <windows.h>
#include <vector>
#include <tuple>
#include <variant>

std::tuple<bool, std::wstring> Win_path_selector();
void If_prog_view_is_2_and_false();
std::wstring chooser(const std::wstring& what_choice);
void countdown(double seconds_to_wait, std::wstring wstring_to_show, int duration);
std::wstring extract_filename(const std::wstring& path);
std::vector<std::wstring> make_massive_of_wstr();
int writefile(std::wstring line, std::string file_type, std::string prog_name, bool show);
int writefile(std::vector<std::wstring> lines, std::string file_type, std::string prog_name, bool show);
std::variant<std::wstring, std::vector<std::wstring>> readFile(std::string file_type, bool isVector);
void make_txt_for_scripts(std::string directory_path);
std::wstring choose_line(short line_number, std::string file_type);
int startfiles(int line_number, const std::string& file_type);
int delete_lines_or_insert_one(std::string file_type, std::vector<int> numbers, bool insert, std::wstring line_to_insert, short line_number, bool show);
void files_checker();
std::wstring prog_settings(bool change, short num_to_read);
void func_linker(short mode, const std::string& file_type, const std::wstring prog_view, bool from_start_files, short line_num);