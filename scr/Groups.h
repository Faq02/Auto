#pragma once
#include <string>

std::wstring group_add(std::wstring prog_view, bool from_changer = false);
int group_del();
void create_shortcut(int group_number, wchar_t hotkey_letter, const std::wstring& shortcut_path);
std::wstring GetProgramsMenuPath();
std::wstring GetOrCreateAppFolder();