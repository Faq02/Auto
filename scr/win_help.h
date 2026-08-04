#pragma once
#include <tuple>
#include <string>
#include <wtypes.h>

std::tuple<bool, DWORD> IsProcessRunning(const std::wstring& processName);
int makeTaskAdmin();
bool ActivateProcessByPID(DWORD pid);

void win_click_on_pos(int x, int y, bool Right = false);
bool SendShortcut(const std::vector<std::wstring>& keys);
bool SendText(const std::wstring& text);
class fq_maker {
public:
	std::wstring get_cursor_pos();
};