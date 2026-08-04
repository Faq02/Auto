#pragma once
#include <vector>
#include <string>

#include "app_config.h"

std::vector<std::wstring> make_massive_of_wstr();
void countdown(double seconds_to_wait, std::wstring wstring_to_show, int duration);
int show_scripts(int state, std::vector<std::wstring> scr_insides_lines, bool need_translate = true, bool from_scr_start = false);
int additional_option_logic(std::map<short, std::pair<std::wstring, FileType>>::const_iterator& it, int option, size_t orig_lines_count, short app_type);
void colorfulPrint(std::wstring prompt, std::wstring text_color, std::wstring background_color = L"");
SelectedItem select_from_file_or_manual(FileType type, bool allow_manual, bool allow_flags, bool force_manual, std::wstring path_choose_view_num);
std::tuple<FileType, std::vector<int>> ask_lines_in_file_type(std::wstring prompt = L"", bool single_line = false);
std::wstring input_word(const std::wstring& what_choice);
std::wstring input_line(const std::wstring& prompt);