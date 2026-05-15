#pragma once
#include <string>
#include <vector>
#include "app_config.h"

std::string getFileName(FileType type);
std::wstring read_url_from_url_file(const std::wstring& wpath);
int is_valid_file_type(const std::wstring& path);
struct LineEntry {
    std::wstring path;
    std::wstring name;
    std::wstring flags;
};
struct Group {
    std::vector<LineEntry> entries;
    std::wstring name;
    std::wstring flags;
};
LineEntry line_parser(FileType type, short line_number, std::wstring raw_line);
std::vector<LineEntry> file_parser(FileType type);
Group group_parser(const std::wstring& line);
std::vector<std::wstring> showfile(FileType type, std::wstring setting);
std::vector<std::wstring> show_groups(std::wstring setting);
std::vector<std::wstring> split(const std::wstring& line, wchar_t delim);
void sort_flags(std::vector<std::wstring>& flags);
std::wstring make_flags();
std::vector<std::wstring> translate_script_insides(std::vector<std::wstring>& not_translated_insides);