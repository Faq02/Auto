#pragma once
#include <vector>
#include "StartFuncs.h"

std::vector<std::wstring> make_massive_of_wstr();
void countdown(double seconds_to_wait, std::wstring wstring_to_show, int duration);
int show_scripts(int state, std::vector<std::wstring> scr_insides_lines, bool need_translate = true, bool from_scr_start = false);
int additional_option_logic(std::map<short, std::pair<std::wstring, FileType>>::const_iterator& it, int option, size_t orig_lines_count, PythonRuntime& python, short app_type);
void colorfulPrint(std::wstring_view prompt, std::wstring_view text_color, std::wstring_view background_color = nullptr);
SelectedItem select_from_file_or_manual(FileType type, bool allow_manual, bool allow_flags, bool force_manual);