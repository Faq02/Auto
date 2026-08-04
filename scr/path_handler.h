#pragma once
#include "app_config.h"

std::tuple<bool, std::wstring> Win_path_selector();
void If_prog_view_is_2_and_false();
std::wstring choose_file_on_pc(std::wstring& path_choose_view, FileType type, short app_type = NULL);
std::vector<std::wstring> split_lineOn_paths(std::wstring& line);
std::wstring_view extract_filename(std::wstring_view path);