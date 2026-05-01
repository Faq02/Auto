
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
#include <map>
#include "advanced_choice.h"
#include "data_work.h"

#pragma comment(lib, "Winmm.lib")

std::vector<std::string> hover_sound = { "sounds\\button1.wav","sounds\\button2.wav","sounds\\perehod.wav" };
std::string select_sound = "sounds\\passGood.wav";

static const std::vector<wchar_t> quick_keys = {
    L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9',
    L'q', L'w', L'e', L'r', L't', L'y', L'u', L'i', L'o', L'p',
    L'a', L's', L'd', L'f', L'g', L'h', L'j', L'k', L'l',
    L'z', L'x', L'c', L'v', L'b', L'n', L'm'
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

    BrightRed = FOREGROUND_INTENSITY | FOREGROUND_RED,
    BrightGreen = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
    BrightBlue = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
    BrightYellow = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
    BrightWhite = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
};

enum class BackgroundColor {
    Black = 0,
    Red = BACKGROUND_RED,
    Green = BACKGROUND_GREEN,
    Blue = BACKGROUND_BLUE,
    Yellow = BACKGROUND_RED | BACKGROUND_GREEN,
    White = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
    BrightWhite = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
};

struct Title {
    std::vector<std::wstring> title_lines;
    int buffer;
    int one_title_line_splits;
    int height;
};


const int LeftMClick = FROM_LEFT_1ST_BUTTON_PRESSED;
const int Enter_key = VK_RETURN;
const int Apostrophe_or_ё = VK_OEM_3;
const int Backspace_key = VK_BACK;
const int Zero_key = 48;
const int MIN_INPUTFIELD_WIDTH = 10;
int menuStartY = 0;

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

static void set_background_color(HANDLE console_output, BackgroundColor background_color, TextColor color = TextColor::White) {
    SetConsoleTextAttribute(console_output, WORD(background_color) | WORD(color));
}
static void set_text_color(HANDLE console_output, TextColor color) {
    SetConsoleTextAttribute(console_output, (WORD)color);
}
static HANDLE start_console_output() { return GetStdHandle(STD_OUTPUT_HANDLE); }


std::vector<int> get_console_params() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return { csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1 };
}
auto cp = get_console_params();
int console_width = cp[0];
int max_console_lines = cp[1];
void fast_print(HANDLE hConsole, const std::wstring& text) {
    DWORD written;
    WriteConsoleW(hConsole, text.c_str(), (DWORD)text.length(), &written, NULL);
}
static std::vector<int> start_end_culc(int& max_console_lines, int display_lines_size, int& hover, int& currentY) {
    int can_show = max_console_lines - currentY; //доступное кол-во строк после currentY - отрисования заголовка
    if (can_show < 1) can_show = 1;
    int start = 0;
    if (display_lines_size > can_show) {
        start = hover - can_show + 1;
        if (start < 0) start = 0;
        if (start > display_lines_size - can_show) start = display_lines_size - can_show;
    }
    int end = start + can_show;
    if (end > display_lines_size) end = display_lines_size;
    return { start,end };
}
static void hide_cursor(CONSOLE_CURSOR_INFO cursorInfo, bool visible = false) {
    cursorInfo.bVisible = visible;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

int get_line_height(const std::wstring& line, int console_width) {
    int height = 1; // хотя бы одна строка
    int current_line_len = 0;
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == L'\n') {
            height++;
            current_line_len = 0;
        }
        else {
            current_line_len++;
            if (current_line_len > console_width) {
                height++;
                current_line_len = 1; // символ переносится
            }
        }
    }
    return height;
}


Title title_params;
static void culc_title_params(const std::wstring& title) {
    std::vector<std::wstring> title_lines = split(title, L'\n');
    int max_height = max_console_lines; // сколько строк доступно для заголовка
    int best_columns = 1;
    int best_height = INT_MAX;

    // Пробуем количество колонок от 1 до 5 (или пока ширина колонки >= 10)

    for (int cols = 1; cols <= 5; ++cols) {
        int col_width = console_width / cols;
        if (col_width < 10) break; // слишком узкие колонки – неудобно

        // Вычисляем высоту каждой колонки
        std::vector<int> col_height(cols, 0);
        for (size_t idx = 0; idx < title_lines.size(); ++idx) {
            int col = idx % cols;
            int len = title_lines[idx].length();
            int parts = (len + col_width - 1) / col_width; // ceil деление
            col_height[col] += parts;
        }
        int total_height = *std::max_element(col_height.begin(), col_height.end());

        if (total_height <= max_height) {
            best_columns = cols;
            best_height = total_height;
            break; // нашли оптимальное (чем меньше колонок, тем лучше)
        }
        if (total_height < best_height) {
            best_columns = cols;
            best_height = total_height;
        }
    }
    int buffer = console_width / best_columns;
    if (buffer < 1) buffer = 1;
    title_params.title_lines = title_lines;
    title_params.buffer = buffer;
    title_params.one_title_line_splits = best_columns;
}



void DrawMenu(const std::vector<std::wstring>& display_lines,
    const std::set<int>& selected,
    int hover,
    bool showCheckboxes,
    const std::wstring& title = L"",
    short newLines = NULL,
    bool from_input_field = false,
    std::wstring num_while_inputing = L""
    )
{
    HANDLE console_output = start_console_output();
    int currentY = 0;
    
    //отрисовка кнопки: убрать заголовок или любой другой на нужной линии 
    

    // Отрисовка заголовка, если он есть

    //std::vector<std::wstring> title_lines = split(title, L'\n');
    //int max_height = max_console_lines; // сколько строк доступно для заголовка
    //int best_columns = 1;
    //int best_height = INT_MAX;

    // Пробуем количество колонок от 1 до 5 (или пока ширина колонки >= 10)
    
    

    // Устанавливаем итоговые значения
    int one_title_line_splits = title_params.one_title_line_splits;

    
    

    if (!title.empty()) {
        int repit = 1;
        //int Y_to_back = 0;
        for (std::wstring title_line : title_params.title_lines) {
            if (repit > 1 and repit <= one_title_line_splits) { GoToXY(title_params.buffer * (repit - 1), currentY); } //по x туда сколько повторений
            else { GoToXY(0, currentY); }
            set_text_color(console_output, TextColor::BrightGreen);

            // Разбиваем длинные строки заголовка
            for (size_t i = 0; i < title_line.length(); i += title_params.buffer) {
                std::wstring part = title_line.substr(i, title_params.buffer);
                if (i > 0) {
                    if (repit == 1) { currentY++; GoToXY(0, currentY); } //1 столбик
                    if (repit > 1) { GoToXY(title_params.buffer * (repit - 1), currentY + 1); currentY++; } //остальные

                }
                std::wcout << part;
            }
            if (repit < one_title_line_splits) { repit++; continue; }

            repit -= one_title_line_splits - 1;
            /*GoToXY(0, currentY);
            std::wcout << part;*/
            currentY++;
            if (currentY > max_console_lines - 7) { 
                GoToXY(0, currentY); std::wcout << L"и ещё несколько скрыто..."; 
                currentY++;
                set_text_color(console_output, TextColor::White); 
                break; 
            }
        }
        set_text_color(console_output, TextColor::White);
        menuStartY = currentY;
        if (showCheckboxes) menuStartY += 3;
        title_params.height = currentY;
    }
    if (showCheckboxes) {
        std::wstring chosen_nums;
        for (int i : selected) {
            if (!chosen_nums.empty()) chosen_nums += L" ";
            chosen_nums += std::to_wstring(i + 1);
        }

        if (from_input_field && !num_while_inputing.empty()) {
            if (!chosen_nums.empty() && !num_while_inputing.empty()) chosen_nums += L" ";
            chosen_nums += num_while_inputing;
        }

        // Определяем ширину поля
        const int MIN_INPUTFIELD_WIDTH = 10; // Минимальная ширина внутренней части
        int content_length = static_cast<int>(chosen_nums.length());
        int field_width = (std::max)(MIN_INPUTFIELD_WIDTH, content_length);

        std::wstring upper_line = L"\n " + std::wstring(field_width, L'_');
        std::wstring middle_line = L"\n|" + chosen_nums;

        // Добавляем подчеркивания для заполнения оставшегося места
        if (content_length < field_width) {
            middle_line += std::wstring(field_width - content_length, L'_');
        }
        middle_line += L"|";

        std::wcout << upper_line << middle_line;
        currentY += 3;
    }
    
    // Рисуем ТОЛЬКО видимую часть
    std::vector<int> culc = start_end_culc(max_console_lines, static_cast<int>(display_lines.size()), hover, currentY);
    for (int i = culc[0]; i < culc[1]; ++i) { //рисуем что поместится
        bool sel = selected.count(i);
        std::wstring marker = L"";

        if (showCheckboxes) {
            marker = sel ? L"[x] " : L"[ ] ";
        }
        else {
            marker = (i == hover) ? L"-> " : L"   ";
        }

        std::wstring line = marker + display_lines[i];
        std::wstring result;
        for (auto ch : line) {
            if (ch == L'\n') {
                // Рассчитываем сколько нужно пробелов до конца строки
                int current_pos = result.size() % console_width;
                int spaces_needed = console_width - current_pos;
                if (spaces_needed > 0 && spaces_needed <= console_width) {
                    result += std::wstring(spaces_needed, L' ');
                }
            }
            else {
                result += ch;
            }
        }
        line = result;




        // Разбиваем строку на части по ширине консоли
        for (size_t pos = 0; pos < line.length(); pos += console_width) {
            GoToXY(0, currentY);

            // Подсветка
            if (i == hover) {
                set_background_color(console_output, BackgroundColor::Blue);
            }
            else if (sel && showCheckboxes) {
                set_text_color(console_output, TextColor::Gray); //я хз что это
            }
            else {
                set_text_color(console_output, TextColor::White);
            }
            std::wstring part = line.substr(pos, console_width);
            int spaces = console_width - part.length();
            fast_print(console_output, part + std::wstring(spaces, L' '));

            currentY++;
        }
    }

    // Сброс цвета
    set_text_color(console_output, TextColor::White);
}






std::vector<int> advansed_chooser(MenuOptions options) {
    _setmode(_fileno(stdout), _O_U16TEXT);
    

    std::vector<std::wstring> display_lines = PrepareDisplayLines(options.lines_to_choose);

    std::vector<int> line_heights;
    std::vector<int> start_positions;
    int current_position = 0;

    for (const auto& line : display_lines) {
        // Вычисляем сколько строк займет эта строка
        int new_line_cnt = 0;
        for (wchar_t c : line) {
            if (c == L'\n') {
                new_line_cnt++;
                
            }
        }
        int height = ((line.length() + console_width - 1) / console_width) + new_line_cnt; // Округление вверх
        if (height < 1) height = 1;

        line_heights.push_back(height);
        start_positions.push_back(current_position);
        current_position += height;
    }
    bool from_scripts = options.lines_to_choose[0] == L"добавить Клик по координатам" && options.singleChoice;
    bool from_groups_make = (options.title.length() >= 6 && options.title.starts_with(L"Режим:"));
    bool from_changer_and_del_lines = (options.lines_to_choose.back() == L"Закончить" && !options.singleChoice);
    //ClearScreen();
    //std::wcout << options.lines_to_choose.back(); Sleep(3000);
    //std::wcout << from_changer_and_del_lines; Sleep(3000);
    // Скрытие курсора
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    hide_cursor(cursorInfo);

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
    culc_title_params(options.title);
    if (!options.title.empty()) newLineCount += title_params.height;
    int line_of_input_field = 0;
    if (!options.singleChoice) newLineCount += 3; //для поля ввода


    std::set<int> selected;
    int hover = 0;
    int lastHover = -1;
    ClearScreen();
    selected.clear();
    // Передаем подготовленные строки в DrawMenu

    DrawMenu(display_lines, selected, hover, !options.singleChoice, options.title, newLineCount, false);

    INPUT_RECORD rec;
    DWORD read;

    // Функция для нахождения элемента меню по координате Y мыши
    auto findMenuIndexByMouseY = [&](int mouseY, int menuStartY) -> int {
        for (int i = 0; i < display_lines.size(); i++) {
            int y_start = menuStartY + start_positions[i];
            int y_end = y_start + line_heights[i];
            if (mouseY >= y_start && mouseY < y_end) {
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
                    if (from_changer_and_del_lines && !input_line.empty()) {
                        try {
                            int input_num = std::stoi(input_line);
                            //далее костыльная обработка дополнительных линий из changer для поддержки срочного выхода из множественного выбора... помойму толком и не работает
                            if (input_num == options.lines_to_choose.size() ||
                                input_num == options.lines_to_choose.size() - 1 ||
                                input_num == options.lines_to_choose.size() - 2) {
                                if (!input_line.empty()) {
                                    selected.insert(std::stoi(input_line) - 1);
                                }
                                break; // Выход при вводе номера выхода
                            }
                        }
                        catch (const std::exception&) { ; }
                    }
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
                        if (from_changer_and_del_lines &&
                            (selected.find(options.lines_to_choose.size()) != selected.end() ||
                                selected.find(options.lines_to_choose.size() - 1) != selected.end() ||
                                selected.find(options.lines_to_choose.size() - 2) != selected.end())) {
                            break;
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
        //обработка мышки и её событий
        if (rec.EventType == MOUSE_EVENT) {
            auto& m = rec.Event.MouseEvent;
            int mouseY = m.dwMousePosition.Y;

            // Находим индекс элемента меню по координате Y
            int index = findMenuIndexByMouseY(mouseY, menuStartY);

            if (index != -1) {
                hover = index;

                // Обрабатываем только клики, а не движение с зажатой кнопкой
                // dwEventFlags == 0 означает клик (не движение)
                if ((m.dwEventFlags == 0 || m.dwEventFlags == DOUBLE_CLICK) &&
                    (m.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {

                    if (options.singleChoice) {
                        PlaySelectSound();
                        ClearScreen();
                        hide_cursor(cursorInfo, false); //возращаем курсор
                        menuStartY = 0;
                        return { index + 1 };
                    }
                    else {
                        if (selected.count(index))
                            selected.erase(index);
                        else
                            selected.insert(index);
                        if (from_changer_and_del_lines && (selected.find(options.lines_to_choose.size()) != selected.end() || // если конец
                            selected.find(options.lines_to_choose.size() - 1) != selected.end() || //если возвращение к выбору объекта
                            selected.find(options.lines_to_choose.size() - 2) != selected.end())) {//если возвращение к выбору режима
                            break;
                        }
                    }
                }

                DrawMenu(display_lines, selected, hover, !options.singleChoice,
                    options.title, newLineCount);
            }
        }
        //обработка нажатий на клавиши
        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
            auto& k = rec.Event.KeyEvent;
            auto& keboard_input = k.wVirtualKeyCode;

            if (keboard_input == Zero_key && from_scripts) {
                ClearScreen();
                menuStartY = 0;
                return { 0 };
            }

            // Обработка быстрых клавиш
            int quick_index = FindQuickKeyIndex(k.uChar.UnicodeChar);
            if (quick_index != -1 && quick_index < (int)options.lines_to_choose.size()) {
                if (options.singleChoice) {
                    PlaySelectSound();
                    ClearScreen();
                    hide_cursor(cursorInfo, false); //возращаем курсор
                    menuStartY = 0;
                    return { quick_index + 1 };
                }
                else {
                    PlayHoverSound;
                    input_mode = true;
                    again = true;
                }
                continue;
            }
            if (from_groups_make) { //тоже хрень какая-то
                if (keboard_input == Apostrophe_or_ё && options.singleChoice == true) {
                    hide_cursor(cursorInfo, false); //возращаем курсор
                    menuStartY = 0;
                    return { 7 };
                }
                if (keboard_input == Zero_key) {
                    hide_cursor(cursorInfo, false); //возращаем курсор
                    menuStartY = 0;
                    return { 0 };
                }
            }

            if (keboard_input == Enter_key) {
                if (options.singleChoice) {
                    PlaySelectSound();
                    ClearScreen();
                    hide_cursor(cursorInfo, false); //возращаем курсор
                    menuStartY = 0;
                    return { hover + 1 };
                }
                else {
                    PlaySelectSound();
                    if (from_changer_and_del_lines &&
                        (selected.find(options.lines_to_choose.size()) != selected.end() ||
                            selected.find(options.lines_to_choose.size() - 1) != selected.end() ||
                            selected.find(options.lines_to_choose.size() - 2) != selected.end())) {
                        break;
                    }
                    break;
                }
            }

            // Обработка стрелок
            if (keboard_input == VK_UP) {
                hover = (hover > 0) ? hover - 1 : 0;//options.lines_to_choose.size() - 1;
                //scroll_offset--;
                DrawMenu(display_lines, selected, hover, !options.singleChoice, options.title, newLineCount);
            }
            else if (keboard_input == VK_DOWN) {
                hover = (hover < (int)options.lines_to_choose.size() - 1) ? hover + 1 : 0;
                //scroll_offset++;
                DrawMenu(display_lines, selected, hover, !options.singleChoice, options.title, newLineCount);
            }

            // Обработка Escape
            if (keboard_input == VK_ESCAPE) {
                ClearScreen();
                hide_cursor(cursorInfo, false); //возращаем курсор
                menuStartY = 0;
                return std::vector<int>{ -1 };
            }
        }
    }

    ClearScreen();
    menuStartY = 0; //почему-то не всегда чистится
    // Преобразуем set в vector для возврата
    std::vector<int> result(selected.begin(), selected.end());
    for (int i = 0; i < result.size(); ++i) {
        result[i]++;
    }

    hide_cursor(cursorInfo, false); //возращаем курсор
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

