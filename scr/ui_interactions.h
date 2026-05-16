#pragma once
#include <vector>
#include <string>
#include <map>
#include "StartFuncs.h"

std::vector<std::wstring> make_massive_of_wstr();
void countdown(double seconds_to_wait, std::wstring wstring_to_show, int duration);
int additional_option_logic(std::map<short, std::pair<std::wstring, FileType>>::const_iterator it, int option, size_t orig_lines_count, PythonRuntime& python, short app_type);
std::wstring colorfulPrint(std::wstring prompt, std::wstring text_color, std::wstring bacground_color = PRINT_BACKGROUNDCOLOR::BLACK);
