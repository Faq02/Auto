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



// Старая версия для обратной совместимости (опционально)
//inline std::vector<int> advansed_chooser(std::vector<std::wstring> lines,
//    bool singleChoice,
//    std::wstring title = L"") {
//    return advansed_chooser({
//        .lines = lines,
//        .singleChoice = singleChoice,
//        .title = title
//        });
//}