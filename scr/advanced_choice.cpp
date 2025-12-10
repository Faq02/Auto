#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <io.h>
#include <fcntl.h>
#include <mmsystem.h>
#include <conio.h>
#include <chrono>
#include <random>
#include <algorithm>
#include <wctype.h>
#include "advanced_choice.h"

#pragma comment(lib, "Winmm.lib")

std::vector<std::string> hover_sound = { "sounds\\button1.wav","sounds\\button2.wav","sounds\\perehod.wav" };
std::string select_sound = "sounds\\passGood.wav";

static const std::vector<wchar_t> quick_keys = {
    L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9',
    L'q', L'w', L'e', L'r', L't', L'y', L'u', L'i', L'o', L'p',
    L'a', L's', L'd', L'f', L'g', L'h', L'j', L'k', L'l',
    L'z', L'x', L'c', L'v', L'b', L'n', L'm'
};


const int LeftMClick = FROM_LEFT_1ST_BUTTON_PRESSED;
const int Enter_key = VK_RETURN;
const int Apostrophe_or_ё = VK_OEM_3;
const int Backspace_key = VK_BACK;
const int Zero_key = 48;

// Функция для подготовки строк с быстрыми клавишами
std::vector<std::wstring> PrepareDisplayLines(const std::vector<std::wstring>& original_lines) {
    std::vector<std::wstring> display_lines;

    for (int i = 0; i < (int)original_lines.size(); i++) {
        wchar_t quick_key = (i < (int)quick_keys.size()) ? quick_keys[i] : L' ';
        std::wstring key_display = (quick_key != L' ') ?
            std::wstring(L"") + quick_key + L"-" : L"    ";

        // Создаем строку для отображения один раз
        display_lines.push_back(key_display + original_lines[i]);
    }

    return display_lines;
}

// Функция для поиска индекса по быстрой клавише
int FindQuickKeyIndex(wchar_t key) {
    wchar_t lower_key = towlower(key);
    auto it = std::find(quick_keys.begin(), quick_keys.end(), lower_key);
    if (it != quick_keys.end()) {
        return std::distance(quick_keys.begin(), it);
    }
    return -1;
}


// Функция для воспроизведения случайного звука наведения
static void PlayHoverSound() {
    if (hover_sound.empty()) return;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, hover_sound.size() - 1);

    int sound_index = dis(gen);
    PlaySoundA(hover_sound[sound_index].c_str(), NULL, SND_FILENAME | SND_ASYNC);
}

// Функция для воспроизведения звука выбора
static void PlaySelectSound() {
    PlaySoundA(select_sound.c_str(), NULL, SND_FILENAME | SND_ASYNC);
}

static void GoToXY(int x, int y) {
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { (SHORT)x, (SHORT)y });
}

static void ClearScreen() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO cs;
    GetConsoleScreenBufferInfo(h, &cs);
    DWORD size = cs.dwSize.X * cs.dwSize.Y, written;
    FillConsoleOutputCharacter(h, L' ', size, { 0,0 }, &written);
    FillConsoleOutputAttribute(h, cs.wAttributes, size, { 0,0 }, &written);
    SetConsoleCursorPosition(h, { 0,0 });
}


void DrawMenu(const std::vector<std::wstring>& display_lines, const std::set<int>& selected, int hover, bool showCheckboxes, const std::wstring& title = L"", short newLines = NULL, bool from_input_field = false, std::wstring num_while_inputing = L"")
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int console_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;

    int currentY = 0;


    // Отрисовка заголовка, если он есть
    if (!title.empty()) {
        std::wstringstream title_stream(title);
        std::wstring title_line;
        while (std::getline(title_stream, title_line, L'\n')) {
            GoToXY(0, currentY);
            SetConsoleTextAttribute(out, FOREGROUND_INTENSITY | FOREGROUND_GREEN);

            // Разбиваем длинные строки заголовка
            for (size_t i = 0; i < title_line.length(); i += console_width) {
                std::wstring part = title_line.substr(i, console_width);
                if (i > 0) {
                    currentY++;
                    GoToXY(0, currentY);
                }
                std::wcout << part;
            }

            currentY++;
            SetConsoleTextAttribute(out, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
    }
    if (showCheckboxes) {
        std::wstring chosen_nums;
        for (int i : selected) {
            if (!chosen_nums.empty()) chosen_nums += L" ";
            chosen_nums += std::to_wstring(i + 1);
        }

        std::wstring content = chosen_nums;
        if (from_input_field && !num_while_inputing.empty()) {
            if (!content.empty() && !num_while_inputing.empty()) content += L" ";
            content += num_while_inputing;
        }

        // Определяем ширину поля
        const int MIN_WIDTH = 10; // Минимальная ширина внутренней части
        int content_length = static_cast<int>(content.length());
        int field_width = (std::max)(MIN_WIDTH, content_length);


        std::wstring upper_line = L"\n " + std::wstring(field_width, L'_');
        std::wstring middle_line = L"\n|" + content;

        // Добавляем подчеркивания для заполнения оставшегося места
        if (content_length < field_width) {
            middle_line += std::wstring(field_width - content_length, L'_');
        }
        middle_line += L"|";

        std::wcout << upper_line << middle_line;
        currentY += 3;
    }

    for (int i = 0; i < (int)display_lines.size(); i++) {
        bool sel = selected.count(i);
        std::wstring marker = L"";

        if (showCheckboxes) {
            marker = sel ? L"[x] " : L"[ ] ";
        }
        else {
            marker = (i == hover) ? L"-> " : L"   ";
        }

        std::wstring line = marker + display_lines[i];

        // Разбиваем строку на части по ширине консоли
        for (size_t pos = 0; pos < line.length(); pos += console_width) {
            GoToXY(0, currentY);

            // Подсветка
            if (i == hover) {
                SetConsoleTextAttribute(out, BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
            else if (sel && showCheckboxes) {
                SetConsoleTextAttribute(out, FOREGROUND_INTENSITY);
            }
            else {
                SetConsoleTextAttribute(out, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }

            std::wstring part = line.substr(pos, console_width);

            std::wcout << part;

            // Очистка остатка строки
            int spaces = console_width - part.length();
            for (int j = 0; j < spaces; j++) {
                std::wcout << L" ";
            }

            currentY++;
        }
    }

    // Сброс цвета
    SetConsoleTextAttribute(out, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}


std::vector<int> advansed_chooser(MenuOptions options) {
    _setmode(_fileno(stdout), _O_U16TEXT);
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int console_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;

    std::vector<std::wstring> display_lines = PrepareDisplayLines(options.lines_to_choose);

    std::vector<int> line_heights;
    std::vector<int> start_positions;
    int current_position = 0;

    for (const auto& line : display_lines) {
        // Вычисляем сколько строк займет эта строка
        int height = (line.length() + console_width - 1) / console_width; // Округление вверх
        if (height < 1) height = 1;

        line_heights.push_back(height);
        start_positions.push_back(current_position);
        current_position += height;
    }
    bool from_scripts = options.lines_to_choose[0] == L"добавить Клик по координатам" && options.singleChoice;
    bool from_groups_make = options.title.length() >= 6 &&
        options.title[0] == L'Р' && options.title[1] == L'е' &&
        options.title[2] == L'ж' && options.title[3] == L'и' &&
        options.title[4] == L'м' && options.title[5] == L':';
    // Скрытие курсора
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

    // Настройка ввода
    HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hin, &mode);
    mode |= ENABLE_EXTENDED_FLAGS;
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    mode |= ENABLE_MOUSE_INPUT;
    SetConsoleMode(hin, mode);

    int newLineCount = 0;
    // Вычисляем высоту заголовка
    if (!options.title.empty()) {
        newLineCount = 0;
        for (wchar_t c : options.title) {
            if (c == L'\n') {
                newLineCount++;
            }
        }
        std::vector<std::wstring> title_lines;
        std::wstringstream title_stream(options.title);
        std::wstring line;
        while (std::getline(title_stream, line, L'\n')) {
            title_lines.push_back(line);
        }
        int additional_lines = 0;
        for (const auto& title_line : title_lines) {
            if (title_line.length() > console_width) {
                additional_lines += (title_line.length() - 1) / console_width;
            }
        }
        newLineCount += additional_lines;
    }
    int line_of_input_field = 0;
    if (!options.singleChoice) {
        newLineCount += 3;
    }


    std::set<int> selected;
    int hover = 0;
    int lastHover = -1;

    ClearScreen();
    selected.clear();
    // Передаем подготовленные строки в DrawMenu

    DrawMenu(display_lines, selected, hover, !options.singleChoice, options.title, newLineCount,false);

    INPUT_RECORD rec;
    DWORD read;

    // Функция для нахождения элемента меню по координате Y мыши
    auto findMenuIndexByMouseY = [&](int mouseY) -> int {
        // Вычитаем высоту заголовка
        int adjustedY = mouseY - newLineCount;

        if (adjustedY < 0 && adjustedY != -2) return -1;
        // Ищем в каком диапазоне находится Y
        for (int i = 0; i < display_lines.size(); i++) {
            if (adjustedY == -2 && !options.singleChoice) {
                return i;
            }
            if (adjustedY >= start_positions[i] && adjustedY < start_positions[i] + line_heights[i]) {
                return i;
            }
        }

        return -1;
        };
    bool input_mode = false;
    if (!options.singleChoice) { input_mode = true; }
    //selected.clear(); // очистка, потому что при вызовах почему-то сохраняются выбранные...
    std::wstring input_line = L"";
    int cursorX_pos = 0;
    WCHAR char_input;
    bool again = false;
    while (true) {
        ReadConsoleInput(hin, &rec, 1, &read);
        
        // Воспроизведение звука при изменении наведения
        if (hover != lastHover) {
            PlayHoverSound();
            lastHover = hover;
        }
        
        if (!options.singleChoice && input_mode) {
            if (again &&
                (!input_line.empty() && std::all_of(input_line.begin(), input_line.end(), ::iswdigit))) {
                again = false;
                DrawMenu(display_lines, selected, hover, !options.singleChoice, options.title, newLineCount, true, input_line);
            }
            if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
                auto& k = rec.Event.KeyEvent;
                WCHAR char_input = k.uChar.UnicodeChar;
                int vk = k.wVirtualKeyCode;
                if (char_input >= L'0' && char_input <= L'9') {
                    cursorX_pos++;
                    GoToXY(cursorX_pos, newLineCount - 2);
                    input_line += char_input;
                    DrawMenu(display_lines, selected, hover, !options.singleChoice, options.title, newLineCount, true, input_line);
                    continue;
                }
                if (vk == VK_SPACE) {
                    if (!input_line.empty()) {
                        selected.insert(std::stoi(input_line) - 1);
                    }
                    input_line = L"";
                    cursorX_pos++;
                    DrawMenu(display_lines, selected, hover, !options.singleChoice, options.title, newLineCount, true);
                    continue;
                }
                if (vk == Backspace_key) {
                    if (!input_line.empty()) {
                        input_line.pop_back();
                    }
                    DrawMenu(display_lines, selected, hover, !options.singleChoice, options.title, newLineCount, true, input_line);
                    continue;
                }
                else if (rec.EventType == MOUSE_EVENT || (rec.Event.KeyEvent.wVirtualKeyCode == VK_UP || rec.Event.KeyEvent.wVirtualKeyCode == VK_DOWN)) {
                    auto& m = rec.Event.MouseEvent;
                    if (m.dwButtonState == LeftMClick || (rec.Event.KeyEvent.wVirtualKeyCode == VK_UP || rec.Event.KeyEvent.wVirtualKeyCode == VK_DOWN)) {
                        input_mode = false;
                        if (!input_line.empty()) {
                            selected.insert(std::stoi(input_line) - 1);
                        }
                        DrawMenu(display_lines, selected, hover, !options.singleChoice, options.title, newLineCount, false);
                    }
                    continue;
                }
                if (char_input == Enter_key) {
                    if (!input_line.empty()) {
                        selected.insert(std::stoi(input_line) - 1);
                    }
                    break;
                }
            }
        }
        static bool leftButtonPressed = false;
        if (rec.EventType == MOUSE_EVENT) {
            auto& m = rec.Event.MouseEvent;
            int mouseY = m.dwMousePosition.Y;

            // Находим индекс элемента меню по координате Y
            int index = findMenuIndexByMouseY(mouseY);

            if (index != -1) {
                hover = index;

                // Обрабатываем только клики, а не движение с зажатой кнопкой
                // dwEventFlags == 0 означает клик (не движение)
                if ((m.dwEventFlags == 0 || m.dwEventFlags == DOUBLE_CLICK) &&
                    (m.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {

                    if (options.singleChoice) {
                        PlaySelectSound();
                        ClearScreen();
                        cursorInfo.bVisible = TRUE;
                        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
                        return { index + 1 };
                    }
                    else {
                        if (selected.count(index))
                            selected.erase(index);
                        else
                            selected.insert(index);
                    }
                }

                DrawMenu(display_lines, selected, hover, !options.singleChoice,
                    options.title, newLineCount);
            }
        }

        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
            auto& k = rec.Event.KeyEvent;
            auto& keboard_input = k.wVirtualKeyCode;

            if (keboard_input == Zero_key && from_scripts) {
                ClearScreen();
                return { 0 };
            }

            // Обработка быстрых клавиш
            int quick_index = FindQuickKeyIndex(k.uChar.UnicodeChar);
            if (quick_index != -1 && quick_index < (int)options.lines_to_choose.size()) {
                if (options.singleChoice) {
                    PlaySelectSound();
                    ClearScreen();
                    cursorInfo.bVisible = TRUE;
                    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
                    return { quick_index + 1 };
                }
                else {
                    PlayHoverSound;
                    input_mode = true;
                    again = true;
                }
                continue;
            }
            if (from_groups_make) {
                if (keboard_input == Apostrophe_or_ё && options.singleChoice == true) {
                    cursorInfo.bVisible = TRUE;
                    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
                    return { 5 };
                }
                if (keboard_input == Zero_key) {
                    cursorInfo.bVisible = TRUE;
                    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
                    return { 0 };
                }
            }

            if (keboard_input == Enter_key) {
                if (options.singleChoice) {
                    PlaySelectSound();
                    ClearScreen();
                    cursorInfo.bVisible = TRUE;
                    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
                    return { hover + 1 };
                }
                else {
                    PlaySelectSound();
                    break;
                }
            }

            // Обработка стрелок
            if (keboard_input == VK_UP) {
                hover = (hover > 0) ? hover - 1 : options.lines_to_choose.size() - 1;
                DrawMenu(display_lines, selected, hover, !options.singleChoice, options.title, newLineCount);
            }
            else if (keboard_input == VK_DOWN) {
                hover = (hover < (int)options.lines_to_choose.size() - 1) ? hover + 1 : 0;
                DrawMenu(display_lines, selected, hover, !options.singleChoice, options.title, newLineCount);
            }

            // Обработка Escape
            if (keboard_input == VK_ESCAPE) {
                ClearScreen();
                cursorInfo.bVisible = TRUE;
                SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
                return options.singleChoice ? std::vector<int>{ -1 } : std::vector<int>{};
            }
        }
    }

    ClearScreen();

    // Преобразуем set в vector для возврата
    std::vector<int> result(selected.begin(), selected.end());
    for (int i = 0; i < result.size(); ++i) {
        result[i]++;
    }

    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    return result;
}
//ЗАМЕНА ТУПЫХ КОНСТАНТ ВИНДОВС!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// 
// auto& k = rec.Event.KeyEvent;
//auto& keboard_input = k.wVirtualKeyCode; ТОЖЕ ВСёёёёёёёёёёёёё замЕНИТЬ!!!!!!!
//#include <windows.h>
//#include <iostream>
//
//// Определение своих, более понятных констант
//const int LeftMouseClick = FROM_LEFT_1ST_BUTTON_PRESSED;
//const int EnterKey = VK_RETURN;
//const int MyCustomReturn = VK_RETURN; // Еще один вариант имени
