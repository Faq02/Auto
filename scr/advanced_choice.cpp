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
#include <variant>

#include "data_work.h"
#include "advanced_choice.h"

#pragma comment(lib, "winmm.lib")

const int LeftMClick = FROM_LEFT_1ST_BUTTON_PRESSED;
const int Enter_key = VK_RETURN;
const int Apostrophe_or_ё = VK_OEM_3;
const int Backspace_key = VK_BACK;
const int Zero_key = 48;
const int MIN_INPUTFIELD_WIDTH = 10;
const int RESIZE_RETURN_CODE = -2;

std::vector<std::string> hover_sound = { "sounds\\button1.wav","sounds\\button2.wav","sounds\\perehod.wav" };
std::string select_sound = "sounds\\passGood.wav";

static const std::vector<wchar_t> quick_keys = {
    L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9',
    L'q', L'w', L'e', L'r', L't', L'y', L'u', L'i', L'o', L'p',
    L'a', L's', L'd', L'f', L'g', L'h', L'j', L'k', L'l',
    L'z', L'x', L'c', L'v', L'b', L'n', L'm'
};





// TODO перенести всё в класс, чтобы не global у меня с ним уже ptsr предыдущей версии, когда многое сохранялось и приходилось вручную убирать
// Необходимая директива для автоматического связывания с библиотекой winmm.lib
struct TitleLayout {
    std::vector<std::wstring> title_lines;  // строки заголовка после split по '\n'
    int buffer = 0;            // ширина одной колонки (в символах)
    int columns = 1;           // количество колонок
    int height = 0;            // полная высота заголовка (в строках консоли)
};
struct ChildrenLine {
    std::vector<std::wstring> segments;
};



/// Настройки меню, передаваемые пользователем

struct DisplayLine {

    std::vector<std::wstring> segments; // линии, разбитые на куски
};

enum class ZoneType {
    None,
    MenuItem,      // пункт меню
    InputField,    // поле ввода
    ChildItem,
    // Button?
};

struct ActiveZone {
    ZoneType type;
    int index;      // для MenuItem — индекс пункта, для InputField — не используется
    int y_start;
    int y_end;
    int x_start;    // для будущих кнопок, пока 0
    int x_end;      // console_width, пока всё по ширине
};

struct VisibleItem {
    ZoneType type;          // MainItem / ChildItem / InputField

    int root_index;         // индекс основного пункта
    int child_index;        // -1 если не ребёнок посути -указатель на root_index которому принадлежит

    DisplayLine display;    // уже подготовленные сегменты

    int y_start;
    int y_end;
};
//работает с индексами, возращает 1-based
class AdvancedChooser {
public:
    ChoiceResult result;
    AdvancedChooser(const MenuOptions& optionS)
        : options(optionS),
        selected(options.preselected.begin(), options.preselected.end()),
        hover(options.prehovered),
        expanded_root(options.preexpanded),
        display_lines_withnums(options.lines_to_choose) 
        {
        init_console_state();
        update_console_size(); //1-ый раз обновляем, потом уже в основном цикле и там же придётся переподготавливать линии
        set_cursor_show(false); //goddaum можно не писать это в коде каждый раз
        calc_everything();
        ClearScreen();
    }
    void save_results(std::vector<int> vec_before_resize, std::vector<int> vec_childs = {}) {
        if (!vec_before_resize.empty()) {
            std::set<int> sel(vec_before_resize.begin(), vec_before_resize.end());
            selected = sel;
            if (!vec_childs.empty()) {
                std::set<int> meh(vec_childs.begin(), vec_childs.end());
                selected_children = meh;//ну прям полный питон код-вперёд пофиг на память...
            }
        }
        return;
    }


    std::variant<std::vector<int>, ChoiceResult> Run(bool children) {
        ProcessInputLoop();
        set_cursor_show(true);
        reset_console_state();
        system("cls");
        if (children) {
            result.roots = get_result(false);
            result.children = get_result(true);
            result.hovered = hover;
            result.key = special_key;
            return result;
        }
        return get_result(false);
    }



private:
    MenuOptions options;
    // Состояние меню
    std::set<int> selected;             // индексы выбранных пунктов (0-based)
    int hover = 0;                       // индекс пункта под курсором / выделением
    int child_hover = -1;
    int expanded_root = -1;
    bool input_mode = false;
    std::wstring input_line;  // буфер поля
    const int input_field_height = 3; // высота занимаемая полем ввода
    bool need_restart = false;         //рестар если изминение окна
    std::set<int> selected_children = {};
    int prev_DrawY = 0;
    ShortcutKeys special_key;
    bool field_already_active = false;

    // Вычисленные размеры и разбивка
    //std::vector<ActiveZone> active_zones;
    TitleLayout title_layout;            // параметры заголовка
    std::vector<std::wstring> child_lines;
    std::vector<std::wstring> display_lines_withnums; // строки меню с быстрыми клавишами (подготовленные)
    std::vector<DisplayLine> display_lines;
    std::map<int, std::vector<DisplayLine>> childrens;
    std::vector<VisibleItem> display_items;

    std::vector<int> lines_heights;       // высота каждой строки меню в строках консоли
    std::vector<int> lines_startY;        // смещение начала каждой строки меню от menuStartY
    int visible_range_start = 0;
    int visible_range_end = 0;
    int scroll_ofset = 0;
    int DrawY = 0;
    int menuStartY = 0;            // Y-координата начала области меню
    const int MARKER_WIDTH = 3;
    static constexpr int SCROLL_STEP = 3; //в видимых строках, а не консольных-нету половинчатости, или помещается или нет а также вполне может быть измененно пользователем

    // Параметры консоли
    int console_width;
    int console_height;
    const HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hConsoleInput;
    DWORD original_console_mode;

    // Звуки
    std::vector<std::string> hoverSoundPaths; ///< пути к случайным звукам наведения
    std::string selectSoundPath;              ///< путь к звуку выбора

    //временные значения
    size_t childs_size;
    //цвета
    menucolors MenuColors = options.menucolors;

    // Генератор случайных чисел для звуков наведения
    std::mt19937 rng;

    // Быстрые клавиши (статический набор)
    static const std::vector<wchar_t> QuickKeys;

    
    std::vector<int> get_result(bool children) {
        std::vector<int> res;
        if (children) { res.assign(selected_children.begin(), selected_children.end()); }
        else { res.assign(selected.begin(), selected.end()); }

        if (!need_restart) {
            for (int& val : res) {
                if (val >= 0) {
                    val++;  // 0-based -> 1-based
                }
            }
        }
        if (need_restart == true and children == false) res.push_back(RESIZE_RETURN_CODE);
        return res;
    }
    //помощь и консоль
    //звук текущего hover
    void PlayHoverSound() {
        if (hover_sound.empty()) return;

        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, hover_sound.size() - 1);

        int sound_index = dis(gen);
        PlaySoundA(hover_sound[sound_index].c_str(), NULL, SND_FILENAME | SND_ASYNC);
    }
    // Функция для воспроизведения звука выбора
    void PlaySelectSound() {
        PlaySoundA(select_sound.c_str(), NULL, SND_FILENAME | SND_ASYNC);
    }

    //считает зоны для мышки - последний вызов после отрисовки, когда display_items уже готов
    //void BuildActiveZones() {
    //    active_zones.clear();

    //    // Зона поля ввода (если есть)
    //    if (!options.singleChoice) {
    //        ActiveZone input_zone;
    //        input_zone.type = ZoneType::InputField;
    //        input_zone.index = -1;
    //        input_zone.y_start = title_layout.height;  // сразу после заголовка
    //        input_zone.y_end = input_zone.y_start + input_field_height;
    //        input_zone.x_start = 0;
    //        input_zone.x_end = console_width;
    //        active_zones.push_back(input_zone);
    //    }

    //    // Зоны пунктов меню
    //    
    //    int cnt = 0;
    //    for (const auto& line : display_items) {
    //        ActiveZone item_zone;
    //        line.type == ZoneType::MenuItem ? item_zone.type = ZoneType::MenuItem : item_zone.type = ZoneType::ChildItem;
    //        item_zone.index = cnt;
    //        item_zone.y_start = line.y_start;
    //        item_zone.y_end = line.y_end;
    //        item_zone.x_start = 0;
    //        item_zone.x_end = console_width;
    //        active_zones.push_back(item_zone);

    //        cnt++;
    //    }
    //}

    void calc_everything() {
        prepare_lines();
        calculate_title_layout();
        calculate_lines_hights();
    }

    void update_console_size() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        console_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        console_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }

    void init_console_state() {
        hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
        DWORD console_mode;
        GetConsoleMode(hConsoleInput, &console_mode);
        original_console_mode = console_mode;
        console_mode |= ENABLE_EXTENDED_FLAGS;
        console_mode &= ~ENABLE_QUICK_EDIT_MODE;
        console_mode |= ENABLE_MOUSE_INPUT;
        console_mode |= ENABLE_WINDOW_INPUT;
        SetConsoleMode(hConsoleInput, console_mode);
    }

    void reset_console_state() { SetConsoleMode(hConsoleInput, original_console_mode); }

    void set_cursor_show(bool show) {
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
        cursorInfo.bVisible = show;
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    }

    //ставит цифру или букву перед линией по индексу
    void add_keys_to_mainLines() {
        std::vector<std::wstring> original = display_lines_withnums; //жаль что копирование, но есть ли выход? мб TODO
        display_lines_withnums.clear();
        if (!options.singleChoice) {
            for (int i = 0; i < (int)original.size(); i++) {
                display_lines_withnums.push_back(std::to_wstring((i+1)) + L"-" + original[i]);
            }
        }
        else {
            for (int i = 0; i < (int)original.size(); i++) {
                wchar_t quick_key = (i < (int)quick_keys.size()) ? quick_keys[i] : L' ';
                std::wstring key_display = (quick_key != L' ') ?
                    std::wstring(L"") + quick_key + L"-" : L"    ";

                display_lines_withnums.push_back(key_display + original[i]);
            }
        }

    }
    void add_spec_marker_forChilds() {
        //изначально segments - вектора с 1 линией, поэтому ничего страшного
        //записываем чистые линии без номеров
        //возврат к тому, что почти было
        for (const auto& [id, segments_vec] : options.children) {
            auto& line_vec = childrens[id];
            line_vec.reserve(segments_vec.size());
            for (auto& segment : segments_vec) {

                line_vec.push_back(DisplayLine{ {segment} });
            }
        }
    }

    int FindQuickKeyIndex(wchar_t key) {
        wchar_t lower_key = towlower(key);
        auto it = std::find(quick_keys.begin(), quick_keys.end(), lower_key);
        if (it != quick_keys.end()) {
            return std::distance(quick_keys.begin(), it);
        }
        return -1;
    }

    void swap_menu_colors() {
        auto old_base = MenuColors.lines_c;
        MenuColors.lines_c = MenuColors.chosen_c; // затемнено, когда поле ввода активно
        MenuColors.field_base_c = MenuColors.field_active_c;
        MenuColors.chosen_c = old_base;
    }

    

    //подготовка
    //расчитывает высоту и высоту начала каждого пункта меню
    void calculate_lines_hights() {
        int currentY = menuStartY; // начинаем после заголовка и поля ввода

        for (size_t i = 0; i < display_lines.size(); ++i) {
            lines_startY.push_back(currentY);
            int height = (int)display_lines[i].segments.size();
            lines_heights.push_back(height);
            currentY += height; // следующий пункт начнётся здесь
        }
    }


    std::vector<std::wstring> wrap_to_console(std::wstring& line, bool with_child) {
        std::vector<std::wstring> result = {};
        std::wstring child_marker = L"";
        if (with_child) {
            child_marker = L" >";
        }
        line = std::wstring(MARKER_WIDTH, L' ') + line + child_marker; //добавление места под маркер для первой линии



        std::wstring current_segment;
        current_segment.reserve(console_width); //выделение памяти под строку, для быстроты вроде


        for (size_t i = 0; i < line.size(); ++i) {
            wchar_t ch = line[i];

            if (ch == L'\n') {
                // Добиваем текущий сегмент пробелами до console_width
                current_segment.append(console_width - current_segment.size(), L' ');
                result.push_back(std::move(current_segment));
                current_segment.clear();
                current_segment.reserve(console_width);

                current_segment = std::wstring(MARKER_WIDTH, L' ');
            }
            else {
                current_segment += ch;

                // Если сегмент заполнился — пушим и начинаем новый
                //TODO тут 2 раза 3 строки повторюется, впринципе пофиг, можно сделать выйгрыш в 4, зачем?
                if ((int)current_segment.size() == console_width) {
                    result.push_back(std::move(current_segment));
                    current_segment.clear();
                    current_segment.reserve(console_width);
                    current_segment = std::wstring(MARKER_WIDTH, L' ');
                }
            }
        }

        // хвост (если остался незавершённый сегмент)
        if (!current_segment.empty()) {
            current_segment.append(console_width - current_segment.size(), L' ');
            result.push_back(std::move(current_segment));
        }
        return result;
        /*if (disline) display_lines.push_back(std::move(dline));
        else {

            for (auto& [key, value] : childrens) {
                for (auto& segment : value) {
                    segment = chline;
                }
            }

        }*/
    }

    void print_childrens() {
        ClearScreen();
        for (auto [key, value] : childrens) {
            //std::wcout << key << L'\n';
            for (auto& segment : value) {
                for (auto& line : segment.segments) {
                    std::wcout << line << L'\n';

                }

            }
        }
        system("pause");
    }

    //подготавливает display_lines для отображения
    void prepare_lines() {
        add_keys_to_mainLines();
        add_spec_marker_forChilds();
        //print_childrens();
        std::vector<std::wstring> lines = {};
        int cnt = 0;
        bool has_child = false;
        for (auto& raw : display_lines_withnums) {
            DisplayLine d;
            cnt++;
            for (const auto& [id, value] : childrens) {
                if (cnt == id) {
                    d.segments = wrap_to_console(raw, true);
                    display_lines.push_back(std::move(d));
                    has_child = true;
                    break;
                }

            }
            if (has_child) { has_child = false; continue; }
            d.segments = wrap_to_console(raw, false);
            display_lines.push_back(std::move(d));
        }

        for (auto& [id, child_vec] : childrens) {

            for (auto& child : child_vec) {
                DisplayLine wrapped;
                for (auto& raw : child.segments) {
                    auto segs = wrap_to_console(raw, false);
                    wrapped.segments.insert(wrapped.segments.end(), segs.begin(), segs.end());
                }
                child = std::move(wrapped);
            }
        }

        //print_childrens();
        lines = display_lines_withnums;



        //add_spec_marker_forChilds();
        std::wstring line = L"";
        DisplayLine dline;


    }


    void build_visible_items() {
        display_items.clear();
        int currentY = menuStartY;
        int root_cnt = 0;

        for (const auto& vec_of_one_line : display_lines) {
            VisibleItem root_item{};   // важно: заново
            root_item.root_index = root_cnt;
            root_item.child_index = -1;
            root_item.type = ZoneType::MenuItem;
            root_item.display = vec_of_one_line;
            root_item.y_start = currentY;
            currentY += (int)vec_of_one_line.segments.size();
            root_item.y_end = currentY;

            display_items.push_back(root_item);

            if (root_cnt == expanded_root) {
                auto it = childrens.find(root_cnt + 1);
                if (it != childrens.end()) {
                    int child_cnt = 0;
                    for (const auto& child : it->second) {
                        VisibleItem child_item{};
                        child_item.root_index = root_cnt;
                        child_item.child_index = child_cnt;
                        child_item.type = ZoneType::ChildItem;
                        child_item.display = child;
                        child_item.y_start = currentY;
                        currentY += (int)child.segments.size();
                        child_item.y_end = currentY;

                        display_items.push_back(child_item);
                        ++child_cnt;
                    }
                }
            }

            ++root_cnt;
        }
    }

    void calculate_title_layout() {
        std::vector<std::wstring> title_lines = split(options.title, L'\n');
        int max_height = console_height; // сколько строк доступно для заголовка
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
        //ух какой костыль, но должен сработать, а так-же он шлёт всю предыдущую логику нахрен
        int new_lines_in_end = 0;
        int end = options.title.size() - 1;
        while (end > 0 and options.title[end] == L'\n') {
            ++new_lines_in_end;
            --end;
        }
        int buffer = console_width / best_columns;
        if (buffer < 1) buffer = 1;
        title_layout.title_lines = title_lines;
        title_layout.buffer = buffer;
        title_layout.columns = best_columns;
        title_layout.height = best_height+new_lines_in_end;
        //menuStartY = title_layout.height;//здесь кончается часть которую не надо трогать мышкой
        
    }


    int GetVisibleIndex() const {
        for (int i = 0; i < (int)display_items.size(); ++i) {
            const auto& item = display_items[i];
            if (item.type == ZoneType::MenuItem && item.root_index == hover) {
                // Если это родитель и у него нет раскрытых детей, или hover не на дочернем — возвращаем родителя
                if (child_hover == -1) return i;
            }
            if (item.type == ZoneType::ChildItem && item.root_index == hover && item.child_index == child_hover) {
                return i;
            }
        }
        // fallback: если не нашли (например, детей скрыли, а child_hover ещё не сброшен) – ищем родителя
        for (int i = 0; i < (int)display_items.size(); ++i) {
            if (display_items[i].type == ZoneType::MenuItem && display_items[i].root_index == hover) {
                return i;
            }
        }
        return 0; // аварийно
    }

    int culc_start_from_start() {

        int available_rows = (console_height - menuStartY);
        if (available_rows < 1) available_rows = 1;


        int start = visible_range_start;
        int center = start;
        if (start > 0) center = start;
        int total_height = display_items[start].display.segments.size();
        for (int i = center - 1; i >= 0; --i) {
            int h = display_items[i].display.segments.size();
            if (total_height + h <= available_rows) {
                total_height += h;
                start = i;
            }
            else break;
        }
        return start;
    }


    const std::pair<int, int> calculate_visible_Range() {
        int available_rows = (console_height - menuStartY);
        if (available_rows < 1) available_rows = 1;

        // Находим фактический индекс выделенного элемента в display_items
        //проходим по display_items


        //if (scroll_ofset > hover and scroll_ofset < display_items.size()-1) hover = scroll_ofset;
        //if (hover > scroll_ofset) scroll_ofset = hover;
        //int center = GetVisibleIndex();



        // Идём вниз
        if (scroll_ofset > display_items.size()) {
            scroll_ofset = display_items.size() - 2;
        }
        int start = scroll_ofset;
        int used = 0;

        int end = start;

        for (int i = start; i < display_items.size(); ++i) {
            int h = display_items[i].display.segments.size();

            if (used + h > available_rows)
                break;

            used += h;
            end = i;
        }


        /*if (hover > end) {
            scroll_ofset = hover;
        }*/
        //if (hover > scroll_ofset) hover = end;

        //if (hover > end) {
        //    //не на ховер а на кол-во строк этого элемента

        //    start += (int)display_items[hover].display.segments.size();

        //    end += hover - end;
        //}
        /*std::wcout
            << L"display_items.size="
            << display_items.size()
            << L" hover="
            << hover
            << L" scroll="
            << scroll_ofset
            << L" start="
            << start
            << L" end="
            << end
            << L"\n";*/
        return { start, end + 1 };
    }

    int get_line_height(const std::wstring& line) {
        int height = 0;
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

    //отрисовка(переделанная с упором на производительность и читаемость)
    void ClearScreen() {
        CONSOLE_SCREEN_BUFFER_INFO cs;
        GetConsoleScreenBufferInfo(hConsoleOutput, &cs);
        DWORD size = cs.dwSize.X * cs.dwSize.Y, written;
        FillConsoleOutputCharacter(hConsoleOutput, L' ', size, { 0,0 }, &written);
        FillConsoleOutputAttribute(hConsoleOutput, cs.wAttributes, size, { 0,0 }, &written);
        SetConsoleCursorPosition(hConsoleOutput, { 0,0 });
    }
    void set_background_color(HANDLE console_output, BackgroundColor background_color, TextColor color = TextColor::White) {
        SetConsoleTextAttribute(console_output, WORD(background_color) | WORD(color));
    }
    void set_text_color(HANDLE console_output, TextColor color) {
        SetConsoleTextAttribute(console_output, (WORD)color);
    }
    void fast_print(HANDLE hConsole, const std::wstring& text) {
        DWORD written;
        WriteConsoleW(hConsole, text.c_str(), (DWORD)text.length(), &written, NULL);
    }
    void GoToXY(int x, int y) {
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { (SHORT)x, (SHORT)y });
    }

    void DrawTitle() { //тяжёлый для чтения, но рабочий отрисовывает как человек читает- 1строка 2строка\n 3строка 4строка
        int repit = 1;
        //int Y_to_back = 0;
        for (std::wstring title_line : title_layout.title_lines) {
            if (repit > 1 and repit <= title_layout.columns) { GoToXY(title_layout.buffer * (repit - 1), DrawY); } //по x туда сколько повторений
            else { GoToXY(0, DrawY); }
            set_text_color(hConsoleOutput, TextColor::BrightGreen);

            // Разбиваем длинные строки заголовка
            for (size_t i = 0; i < title_line.length(); i += title_layout.buffer) {
                std::wstring part = title_line.substr(i, title_layout.buffer);
                if (i > 0) {
                    if (repit == 1) { DrawY++; GoToXY(0, DrawY); } //1 столбик
                    if (repit > 1) { GoToXY(title_layout.buffer * (repit - 1), DrawY + 1); DrawY++; } //остальные

                }
                std::wcout << part; //а чё не fastprint?

            }
            if (repit < title_layout.columns) { repit++; continue; }

            repit -= title_layout.columns - 1;
            /*GoToXY(0, currentY);
            std::wcout << part;*/
            DrawY++;
            if (DrawY > console_height - 7) {
                GoToXY(0, DrawY); std::wcout << L"и ещё несколько скрыто...";
                DrawY++;
                set_text_color(hConsoleOutput, TextColor::White);
                break;
            }
        }
        menuStartY = DrawY;
        set_text_color(hConsoleOutput, TextColor::White);
        //if (!options.singleChoice) DrawY += 3; 
        //title_layout.height = DrawY;
    }

    void DrawInputField() {
        set_text_color(hConsoleOutput, MenuColors.field_base_c);
        std::wstring chosen_nums;
        for (int i : selected) {
            if (!chosen_nums.empty()) chosen_nums += L" ";
            chosen_nums += std::to_wstring(i + 1);
        }

        if (input_mode && !input_line.empty()) {
            if (!chosen_nums.empty() && !input_line.empty()) chosen_nums += L" ";
            chosen_nums += input_line;
        }

        // опред. ширины поля
        const int MIN_INPUTFIELD_WIDTH = 10; // Минимальная ширина поля ввода, менять её я не буду, поэтому и выносить отсюда тоже не хочу
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
        DrawY += input_field_height;
        menuStartY += 3; //размер поля ввода
        set_text_color(hConsoleOutput, TextColor::White); //возвращаем на всякий
    }

    void DrawVisibleLines() {
        for (int i = visible_range_start; i < visible_range_end; ++i) { //рисуем что поместится

            int printed_lines = 0;
            std::wstring marker = L"";
            VisibleItem& item = display_items[i];
            const DisplayLine& line = display_items[i].display;

            //маркер
            bool is_hover =
                (item.type == ZoneType::MenuItem &&
                    item.root_index == hover)

                || (item.type == ZoneType::ChildItem &&
                    item.root_index == expanded_root &&
                    item.child_index == child_hover);
            marker = (is_hover) ? L"-> " : L"   ";
            bool sel = (options.childrenMultiplyChoice and options.singleChoice) ? selected_children.count(item.child_index) : selected.count(item.root_index);
            if (!options.singleChoice or (options.childrenMultiplyChoice and item.type == ZoneType::ChildItem)) marker = sel ? L"[x]" : L"[ ]";


            //цвет
            if (is_hover) set_background_color(hConsoleOutput, MenuColors.hover_c);
            else if (sel && !options.singleChoice) set_text_color(hConsoleOutput, MenuColors.chosen_c); //покрас выбранных в серый
            else set_text_color(hConsoleOutput, MenuColors.lines_c);


            for (size_t s = 0; s < line.segments.size(); ++s) {
                GoToXY(0, DrawY);
                if (s == 0) {
                    // Первый - заменяем первые 3 символа маркером
                    std::wstring output = marker + line.segments[0].substr(MARKER_WIDTH);
                    fast_print(hConsoleOutput, output);
                }
                else {
                    fast_print(hConsoleOutput, line.segments[s]);
                }
                ++DrawY;
            }
            set_text_color(hConsoleOutput, MenuColors.lines_c);
        }
        //надо либо подсветку как-то граничить или само заполнение...
        if (DrawY < prev_DrawY) {
            for (int y = DrawY; y < prev_DrawY; ++y) {
                GoToXY(0, y);
                fast_print(hConsoleOutput, std::wstring(console_width, L' '));
            }
        }
        prev_DrawY = DrawY;
        // 
        // старый вариант
        // 
        //int max_y = min(console_height, menuStartY + console_height); // можно просто до console_height
        //set_text_color(hConsoleOutput, MenuColors.base_color);
        //while (DrawY < max_y) {
        //    GoToXY(0, DrawY);
        //    fast_print(hConsoleOutput, std::wstring(console_width, L' '));
        //    ++DrawY;
        //}
    }

    void Draw() {
        DrawY = 0;
        DrawTitle();
        if (!options.singleChoice) DrawInputField();
        build_visible_items();
        auto [start, end] = calculate_visible_Range();
        visible_range_start = start;
        visible_range_end = end;
        DrawVisibleLines();
        
    }


    void check_if_its_expanded() {

        if (hover != expanded_root) {
            expanded_root = hover;

            if (!childrens.contains(hover + 1)) {
                expanded_root = -1;
            }

            child_hover = -1;
        }
    }

    void ScrollUp(int steps = SCROLL_STEP) {
        if (scroll_ofset > 0) {
            scroll_ofset = (std::max)(0, scroll_ofset - steps);
            Draw();
        }
    }

    void ScrollDown(int steps = SCROLL_STEP) {
        int max_scroll = (std::max)(0, (int)display_items.size() - 3);
        if (scroll_ofset < max_scroll) {
            scroll_ofset = (std::min)(max_scroll, scroll_ofset + steps);
            Draw();
        }
    }

    void empty_lines() {

        ClearScreen();
        set_text_color(hConsoleOutput, TextColor::Red);

        int centerY = (console_height / 2) - 5;
        

        const wchar_t* Empty[] = {
        L"_______   _     _    __  ________    ____       \n",
        L"|||||||   \\\\   //   ((|  ||||||||  //   \\\\      \n",
        L"||   ||    \\\\ //  ((        ||    ||     ||     \n",
        L"||   ||     //    ((        ||    ||     ||     \n",
        L"||   ||    //       ((|     ||     \\\\___//      \n",
        };
        int centerX = (console_width / 2) - 23; //23-наугад, вроде примерно оно
        GoToXY(0, centerY);
        for (auto line : Empty) {
            
            std::wcout << std::wstring(centerX, L' ') << line;
        }
        GoToXY(centerX, centerY + 6);
        std::wcout << L"у вас нету того из чего выбирать.";
        GoToXY(centerX+7, centerY + 7);
        std::wcout << L"для продолжения нажмите любую кнопку ";
        _getch();
        set_text_color(hConsoleOutput, MenuColors.lines_c);
    }



    bool HandleMouseEvent(const MOUSE_EVENT_RECORD& mouse) {

        //колёсико
        if (mouse.dwEventFlags == MOUSE_WHEELED) {
            int wheel_delta = (short)HIWORD(mouse.dwButtonState);
            int steps = abs(wheel_delta) / WHEEL_DELTA * SCROLL_STEP; //зачем? wheel_delta всегда 120, поитогу этот код это 1*SCROLL_STEP... upd не всегда иногда отличные от этого

            if (wheel_delta > 0) {
                ScrollUp(steps);
            }
            else {
                ScrollDown(steps);
            }
            return false;
        }


        int mouseY = mouse.dwMousePosition.Y;
        int mouseX = mouse.dwMousePosition.X;
        int in_bacground_lines = 0;
        if (scroll_ofset > 0) {
            for (int i = 0; i < visible_range_start; ++i) {
                const auto& item = display_items[i];
                in_bacground_lines += (item.y_end - item.y_start);
            }
        }
        mouseY += in_bacground_lines;

        int inputFieldY = title_layout.height;
        if (!options.singleChoice and mouseY >= inputFieldY and mouseY < inputFieldY + input_field_height) {
            // Мышь над полем
            // подсветка
            if (mouse.dwEventFlags == MOUSE_MOVED and field_already_active == false) {
                input_mode = true;
                swap_menu_colors();
                Draw();
                field_already_active = true;
            }
            //нажатие
            if ((mouse.dwEventFlags == 0 || mouse.dwEventFlags == DOUBLE_CLICK) &&
                (mouse.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {
                Draw();
            }
            return false;
        }
        if (field_already_active) {
            swap_menu_colors();
            field_already_active = false;
        }
        input_mode = false;
        // 2. Ищем среди всех видимых пунктов (основных и дочерних)
        for (int i = visible_range_start; i < visible_range_end; ++i) {
            const auto& item = display_items[i];
            /*std::wcout
                << L"i=" << i
                << L" y=" << item.y_start
                << L"-" << item.y_end
                << L" mouse=" << mouseY << L'\n';*/

            if (mouseX >= 0 && mouseX < console_width &&
                mouseY >= item.y_start && mouseY < item.y_end) {
                Draw();
                // Определяем, какой это элемент
                if (item.type == ZoneType::MenuItem) {
                    hover = item.root_index;
                    selected_children.clear(); //очистка при наведении на другое
                    child_hover = -1;
                    check_if_its_expanded();
                }
                else if (item.type == ZoneType::ChildItem) {
                    // Важно: не меняем hover, только child_hover
                    child_hover = item.child_index;
                    // expanded_root уже должен быть корнем
                }

                // Обработка клика
                if ((mouse.dwEventFlags == 0 || mouse.dwEventFlags == DOUBLE_CLICK) &&
                    (mouse.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {
                    if (item.type == ZoneType::MenuItem) {
                        if (options.singleChoice) {
                            PlaySelectSound();
                            selected = { item.root_index };
                            return true;
                        }
                        else {
                            if (selected.count(item.root_index))
                                selected.erase(item.root_index);
                            else
                                selected.insert(item.root_index);
                            Draw();
                        }
                    }

                    if (item.type == ZoneType::ChildItem) {
                        if (!options.childrenMultiplyChoice) {
                            PlaySelectSound();
                            selected = { hover };
                            selected_children = { child_hover };
                            return true;
                        }
                        else {
                            if (selected_children.count(child_hover))
                                selected_children.erase(child_hover);
                            else
                                selected_children.insert(child_hover);
                            Draw();
                        }
                    }
                }
                // Перерисовка при движении или клике
                Draw();
                return false;
            }
        }

        // Мышь вне всех зон
        //Sleep(3000);
        return false;
    }
    //вернёт true если надо выйти из основного цикла
    bool HandleKeyboardEvent(const KEY_EVENT_RECORD& key) {
        auto& keyboard_input = key.wVirtualKeyCode;
        int quick_index = FindQuickKeyIndex(key.uChar.UnicodeChar);


        //опять режимы
        //if (keyboard_input == Zero_key){//&& from_scripts) {
        //    ClearScreen();
        //    selected = { 0 };
        //    return true;
        //}


        //if(!options.singleChoice) EnterInputMode();


        if (input_mode) {
            WCHAR char_input = key.uChar.UnicodeChar;
            //должен быть от 0 до max_quick_index
            if (char_input >= L'0' && char_input <= L'9') {

                input_line += char_input;

                Draw();
                return false;
            }
            if (keyboard_input == VK_SPACE) {
                if (!input_line.empty()) {
                    selected.insert(std::stoi(input_line) - 1);//-1 так как теперь это индекс
                }
                input_line.clear();
                Draw();
                return false;
            }
            if (keyboard_input == Backspace_key) {
                if (!input_line.empty()) {
                    input_line.pop_back();
                }
                Draw();
                return false;
            }
            //TODO в обработке мышки или где-то когда MOUSE_EVENT проверять input_mode
            if (keyboard_input == VK_UP || keyboard_input == VK_DOWN) {
                if (!input_line.empty()) {
                    int num = std::stoi(input_line) - 1;
                    if (num >= 0 && num < (int)options.lines_to_choose.size()) {
                        selected.insert(num);
                    }
                }
                input_mode = false;
                Draw();
                return false;
                // Теперь обрабатываем стрелку как обычно (не возвращаем true)
                // Просто позволяем коду ниже обработать VK_UP/VK_DOWN
            }
            if (char_input == Enter_key) {
                if (!input_line.empty()) {
                    selected.insert(std::stoi(input_line) - 1);
                }
                return true;
            }
            return false;
        }


        // Обработка быстрых клавиш
        if (quick_index != -1 && quick_index < (int)options.lines_to_choose.size()) {
            if (options.singleChoice) {
                PlaySelectSound();
                selected = { quick_index };
                return true;
            }
            else {
                PlayHoverSound();
                input_mode = true;
                HandleKeyboardEvent(key);//перевызов для хз, но надо 100%, чувствую
                //again = true; под вопросом, так как не помню уже зачем сделал этот again, но он точно был для чего-то нужен
            }
        }
        //if (options.from_group_make) { //тоже хрень какая-то
        //    if (keyboard_input == Apostrophe_or_ё && options.singleChoice == true) {
        //        selected = { 7 };
        //        return true;
        //    }
        //    if (keyboard_input == Zero_key) {
        //        selected = { 0 };
        //        return true;
        //    }
        //}

        /*for (auto& [id, child_vec] : childrens) {
            if (hover + 1 == id) {
                expanded_root = hover;
                break;
            }
            expanded_root = -1;
        }*/

        if (keyboard_input == Enter_key) {
            if (options.singleChoice) {
                PlaySelectSound();
                selected = { hover };
                if (child_hover != -1 and !options.childrenMultiplyChoice) {
                    selected_children = { child_hover };
                }
                return true;
            }
            else {
                PlaySelectSound();
                /*if (options.from_changer_and_del &&
                    (selected.find(options.lines_to_choose.size()) != selected.end() ||
                        selected.find(options.lines_to_choose.size() - 1) != selected.end() ||
                        selected.find(options.lines_to_choose.size() - 2) != selected.end())) {
                    return true;
                }*/
                return true;
            }
        }

        if (keyboard_input == VK_SPACE) {
            if (options.childrenMultiplyChoice) {
                if (child_hover != -1) {
                    if (selected_children.count(child_hover))
                        selected_children.erase(child_hover);
                    else
                        selected_children.insert(child_hover);
                    Draw();
                }
            }
        }

        // Обработка стрелок
        if (keyboard_input == VK_UP) {

            //should work не уверен насчет индексов
            //если предыдущий по лайнс - дочерний- hover прыгает на кол-во дочерних, которое определяется по их root
            if (child_hover != -1) {
                if (child_hover > 0) {
                    child_hover--;
                }
                else {
                    child_hover = -1;
                }
                Draw();
                return false;
            }
            else if (hover > 0) {
                hover--;

                if (hover < scroll_ofset) {
                    scroll_ofset = hover; //подъём по одному наверх: hover = start(=scroll_ofset)
                }

                if (hover < scroll_ofset) scroll_ofset--;
                selected_children.clear();
                check_if_its_expanded();
                if (expanded_root == hover && childrens.contains(hover + 1)) {
                    child_hover = childrens[hover + 1].size() - 1; //childrens[hover + 1] map - темный лес, поэтому [hover + 1] это поиск в
                    //std::map<int, std::vector<DisplayLine>> childrens элемента std::vector<DisplayLine> по int, последний содержит человеческий 1-based индекс, поэтому +1
                }
                Draw();
                return false;
            }




            //hover = (hover > 0) ? hover - 1 : 0;//options.lines_to_choose.size() - 1;
            ////scroll_offset--;
            //check_if_its_expanded();
            //ClearScreen();
            //std::wcout << hover << L"   " << expanded_root;
            //system("pause");
            //Draw();
            //return false;
        }
        else if (keyboard_input == VK_DOWN) {


            if (child_hover == -1) {
                if (expanded_root == hover &&
                    childrens.contains(hover + 1)) {
                    child_hover = 0;
                }
                else {
                    if (hover != (int)options.lines_to_choose.size() - 1) {
                        hover++;
                        if (hover < scroll_ofset) hover = scroll_ofset;
                        if (hover > visible_range_end) scroll_ofset = hover; //здесь резкий page up после конца
                        selected_children.clear();
                        check_if_its_expanded();
                        auto [start, end] = calculate_visible_Range();
                        visible_range_start = start;
                        visible_range_end = end;
                    }
                    else {
                        hover = 0; //здесь возвращение в начало если что, если когда-то будешь менять
                        scroll_ofset = 0;
                    }
                }
                Draw();
                return false;
            }
            else {
                int child_count = childrens[expanded_root + 1].size();

                if (child_hover + 1 < child_count) {
                    child_hover++;
                }
                else {
                    child_hover = -1;
                    hover++;
                    check_if_its_expanded();
                }
                Draw();
                return false;
            }
            return false; //лишнее
        }

        //page up
        else if (keyboard_input == VK_PRIOR) {
            visible_range_end = visible_range_start;
            scroll_ofset = culc_start_from_start();
            hover = scroll_ofset;
            Draw();
            return false;
            /*
            план:
            1-узнать сколько элементов поместиться вверх после текущего старта
            2-установить старт на новый минимум
            3-энд выйдет новым стартом
            */
        }
        //page down
        else if (keyboard_input == VK_NEXT) {
            scroll_ofset = visible_range_end - 1;
            hover = scroll_ofset;
            Draw();
            return false;
        }
        else if (keyboard_input == Backspace_key) {
            input_mode = true;
            HandleKeyboardEvent(key);
            return false;
        }

        if (keyboard_input >= VK_F1 && keyboard_input <= VK_F6) { //пока только до 6
            special_key = static_cast<ShortcutKeys>(static_cast<int>(ShortcutKeys::F1) + (keyboard_input - VK_F1));
            return true;
        }
        // Обработка Escape
        if (keyboard_input == VK_ESCAPE) {
            selected = { -1 };
            return true;
        }

        return false;

    }


    //основа
    void ProcessInputLoop() {
        INPUT_RECORD rec;
        DWORD read;
        int last_hover = -1;
        if (options.lines_to_choose.empty()) {
            empty_lines();
            selected = { -1 };
            return;
        }
        Draw();
        while (true) {
            //надо также ловить обновление экраа и выщывать update_condole_size()
            ReadConsoleInput(hConsoleInput, &rec, 1, &read);
            if (read > 0) {
                if (rec.EventType == WINDOW_BUFFER_SIZE_EVENT) {
                    need_restart = true;
                    break;
                }
                if (hover != last_hover) {
                    PlayHoverSound();
                    last_hover = hover;
                }
                if (rec.EventType == MOUSE_EVENT) {
                    if (HandleMouseEvent(rec.Event.MouseEvent)) return;
                }
                if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
                    if (HandleKeyboardEvent(rec.Event.KeyEvent)) return;
                }
            }
        }
    }

};
//это в .h


static std::variant<std::vector<int>, ChoiceResult> standart(MenuOptions& options, bool children = false) {
    ChoiceResult result;
    bool resize = false;
    std::vector<int> root_res;
    while (true) {
        AdvancedChooser chooser(options);
        if (resize) {
            if (!options.singleChoice) chooser.save_results(root_res);
        }
        if (children) { result = std::get<ChoiceResult>(chooser.Run(true)); }
        else { root_res = std::get<std::vector<int>>(chooser.Run(false)); }
        resize = false;
        auto new_end = std::remove((children ? result.roots : root_res).begin(), (children ? result.roots : root_res).end(), RESIZE_RETURN_CODE);
        if (new_end != (children ? result.roots : root_res).end()) {
            resize = true;
            (children ? result.roots : root_res).erase(new_end, (children ? result.roots : root_res).end());//нашли-удалили
            Sleep(100); // даём время на stabilisation
            // Очищаем очередь событий изменения размера
            HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
            INPUT_RECORD dummy;
            DWORD read;
            while (PeekConsoleInput(hIn, &dummy, 1, &read) && read > 0) {
                if (dummy.EventType == WINDOW_BUFFER_SIZE_EVENT) {
                    ReadConsoleInput(hIn, &dummy, 1, &read); // поглощаем событие
                }
                else {
                    break; // другие события не трогаем, они пригодятся новому экземпляру
                }
            }
            continue;
        }
        //вывод результатов если надо
        /*for (auto& res : result) {
             std::wcout << res << L'\n';
        }*/
        break; //не нашли-вышли
    }
    if (children) return result;
    return root_res;

}

std::vector<int> advansed_chooser(MenuOptions options) {
    return std::get<std::vector<int>>(standart(options, false));
}
//версия с выводом дочерних
ChoiceResult advansed_chooserC(MenuOptions options) {
    return std::get<ChoiceResult>(standart(options, true));
}