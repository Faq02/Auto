#pragma once
#include <tuple>
#include <string>
std::tuple<bool, DWORD> IsProcessRunning(const std::wstring& processName);
int makeTaskAdmin();
bool ActivateProcessByPID(DWORD pid);