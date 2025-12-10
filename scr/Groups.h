#pragma once
#include <string>
#include "File_funcs.h"

std::wstring group_add(std::wstring prog_view);
int group_del();
void create_shortcut(int group_number, wchar_t hotkey_letter, const std::wstring& shortcut_path);
std::wstring GetProgramsMenuPath();
std::wstring GetOrCreateAppFolder();