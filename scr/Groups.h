#pragma once
#include <string>

void group_add(std::wstring path_choose_view, bool from_changer); //now self-implemented not AI
void create_shortcut(int group_number, wchar_t hotkey_letter, const std::wstring& shortcut_path);
std::wstring GetProgramsMenuPath(); //AI trash
std::wstring GetOrCreateAppFolder(); //AI trash