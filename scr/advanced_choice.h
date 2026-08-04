#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <io.h>
#include <fcntl.h>
#include <mmsystem.h>
#include <map>

#pragma comment(lib, "Winmm.lib")

enum class ShortcutKeys {
    F1 = 1,
    F2 = 2,
    F3 = 3,
    F4 = 4,
    F5 = 5,
    F6 = 6
};
enum class TextColor {
    Black = 0,
    Gray = FOREGROUND_INTENSITY,
    Red = FOREGROUND_RED,
    Green = FOREGROUND_GREEN,
    Blue = FOREGROUND_BLUE,
    Yellow = FOREGROUND_RED | FOREGROUND_GREEN,
    Magenta = FOREGROUND_RED | FOREGROUND_BLUE,
    Cyan = FOREGROUND_GREEN | FOREGROUND_BLUE,
    White = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,

    BrightCyan = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
    BrightMagenta = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
    BrightRed = FOREGROUND_INTENSITY | FOREGROUND_RED,
    BrightGreen = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
    BrightBlue = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
    BrightYellow = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
    BrightWhite = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
};

enum class BackgroundColor {
    Black = 0,                      // 0
    Blue = BACKGROUND_BLUE,         // 16
    Green = BACKGROUND_GREEN,       // 32
    Cyan = BACKGROUND_BLUE | BACKGROUND_GREEN,  // 48 (Aqua)
    Red = BACKGROUND_RED,           // 64
    Magenta = BACKGROUND_RED | BACKGROUND_BLUE, // 80 (Purple)
    Yellow = BACKGROUND_RED | BACKGROUND_GREEN, // 96
    White = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
    DarkGray = BACKGROUND_INTENSITY,            // 128
    LightBlue = BACKGROUND_INTENSITY | BACKGROUND_BLUE,         // 144
    LightGreen = BACKGROUND_INTENSITY | BACKGROUND_GREEN,       // 160
    LightCyan = BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE, // 176
    LightRed = BACKGROUND_INTENSITY | BACKGROUND_RED,           // 192
    LightMagenta = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE, // 208
    LightYellow = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN, // 224
    BrightWhite = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE // 240
};
struct menucolors {
    TextColor field_base_c   = TextColor::Gray;
    TextColor field_active_c = TextColor::White;
    TextColor lines_c        = TextColor::White;
    BackgroundColor hover_c  = BackgroundColor::Blue;
    TextColor chosen_c       = TextColor::Gray;
};



// Структура параметров меню
struct MenuOptions {
    std::vector<std::wstring> lines_to_choose; ///< Строки пунктов меню
    bool singleChoice = true;                 ///< true – выбор одного, false – множественный
    std::wstring title;                       ///< Заголовок меню (может быть пустым)
    std::map<int, std::vector<std::wstring>> children = {};
    bool childrenMultiplyChoice = false;
    std::vector<int> preselected= {};
    int prehovered = 0;
    int preexpanded = -1;
    menucolors menucolors;
    
};
struct ChoiceResult {
    std::vector<int> roots;
    std::vector<int> children;
    int hovered;
    ShortcutKeys key;
};


std::vector<int> advansed_chooser(MenuOptions options);
ChoiceResult advansed_chooserC(MenuOptions options);



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