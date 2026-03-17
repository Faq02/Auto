#pragma once

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <io.h>
#include <fcntl.h>
#include <mmsystem.h>

#pragma comment(lib, "Winmm.lib")

// Структура параметров меню
struct MenuOptions {
    std::vector<std::wstring> lines_to_choose;
    bool singleChoice = false;
    std::wstring title = L"";
};



std::vector<int> advansed_chooser(MenuOptions options);
