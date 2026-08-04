#include "settings.h"

#include "advanced_choice.h"
#include <locale>
#include <fstream>
#include <span>
#include "data_work.h"
#include "file_io.h"

using std::wifstream;
using std::wstring;
using std::vector;
static constexpr auto EXIT_CODE = -1;

const int path_choose_view = 1;
const int if_path_wrong = 2;
const int showlines = 3;
const int flags = 4;
const int have_task_admin = 5; // 0-false(нету задачи с asadmin), 1-true
const int flags_start_indx = 3;

//вызывает функцию advansed_chooser c нужным текстом
static wstring sett_menu_options_show(vector<wstring> lines_to_choose, bool flags = false) {
    wstring for_flags = flags ? L"ДЛЯ ВЫХОДА НАЖМИТЕ ESC\n" : L"";
    return std::to_wstring(advansed_chooser({
            .lines_to_choose = lines_to_choose,
            .singleChoice = true,
            .title = L"Что настраиваем ? : \n" + for_flags})[0]);
}
//+переделать в лямбды
static wstring make_flag_wstr(wstring name, wstring its_bool) {
    const wchar_t* labels[] = { L"false", L"true" };
    return name + labels[stoi(its_bool)];
}
static void change_bool(vector<wstring>& old_flags, int index) {
    old_flags[index] = (stoi(old_flags[index]) == true) ? std::to_wstring(false) : std::to_wstring(true);
}
//+
//возвращает настройки с флагами измененными или нет...(сначало запоминает, а потом меняет и возвращает)
//static vector<wstring> flags_ch_menu(vector<wstring> settings) {
//    vector<wstring> v_flags(settings.begin() + flags_start_indx, settings.end()); //массив флагов(пока -а) {1}
//    //std::wcout v_flags[0];Sleep(10000);
//    //if выбор в input_word = индекс - 1 меняем значение на другое
//    int index = 0;
//    wstring ask_create_scr;
//    vector<wstring> flags_options;
//    while (index != EXIT_CODE - 1) {
//        ask_create_scr = make_flag_wstr(L"Предлагать создать скрипт: ", v_flags[0]);
//        flags_options = { ask_create_scr };
//        index = stoi(sett_menu_options_show(flags_options, true)) - 1;
//        if (index < 0) { break; }
//        change_bool(v_flags, index);
//    }
//    auto startPos = settings.end() - v_flags.size();
//    std::copy(v_flags.begin(), v_flags.end(), startPos);
//    return settings;
//}



const std::map<std::wstring, std::wstring> PATH_CHOOSE_VIEW = {
    {L"1", L"Консоль-страндарт"},
    {L"2", L"По вин-путям"}
};

const std::map<std::wstring, std::wstring> IF_PATH_WRONG = {
    {L"1", L"Использовать поиск"},
    {L"2", L"Перевыбор"},
    {L"3", L"Создание"},
    {L"4", L"Ничего не делать"}
};

const std::map<std::wstring, std::wstring> SHOWLINES = {
    {L"1", L"показывать имена"},
    {L"2", L"показывать название файла+.exe(или другое расширение)"},
    {L"3", L"показывать пути(как раньше)"}
};


enum class settings_pos {
    path_choose_view = 1,
    if_path_wrong = 2,
    showlines = 3,
    colors = 4,
    have_task_admin = 5
};

menucolors parse_menucolors_setting(std::wstring wstr_set) {
    menucolors result;
    std::vector<std::wstring> splited = split(wstr_set, L':');
    // 8:7:7:16:8
    int values[5] = { 8, 7, 7, 16, 8 }; //сначала стандарт
    for (size_t i = 0; i < (std::min)(splited.size(), size_t(5)); ++i) {
        values[i] = std::stoi(splited[i]);
    }

    // Явное и безопасное присвоение через static_cast
    result.field_base_c = static_cast<TextColor>(values[0]);
    result.field_active_c = static_cast<TextColor>(values[1]);
    result.lines_c = static_cast<TextColor>(values[2]);
    result.hover_c = static_cast<BackgroundColor>(values[3]);
    result.chosen_c = static_cast<TextColor>(values[4]);

    return result;
}
//огр. char = 256 или 128? линий настроек
std::wstring read_setting(char one_based_set_pos) {
    std::vector<wstring> settings = std::get<std::vector<wstring>>(readFile({ .file_path = getFileName(FileType::Settings),.isVector = true }));
    return settings[one_based_set_pos - 1];
}

std::wstring to_ansi(TextColor color) {
    switch (color) {
    case TextColor::Black:        return L"\033[30m";
    case TextColor::Gray:         return L"\033[90m"; // Яркий черный (серый)
    case TextColor::Red:          return L"\033[31m";
    case TextColor::Green:        return L"\033[32m";
    case TextColor::Blue:         return L"\033[34m";
    case TextColor::Yellow:       return L"\033[33m";
    case TextColor::Magenta:      return L"\033[35m";
    case TextColor::Cyan:         return L"\033[36m";
    case TextColor::White:        return L"\033[37m";

        // Яркие ANSI-цвета текста (коды 91-97)
    case TextColor::BrightRed:    return L"\033[91m";
    case TextColor::BrightGreen:  return L"\033[92m";
    case TextColor::BrightBlue:   return L"\033[94m";
    case TextColor::BrightYellow: return L"\033[93m";
    case TextColor::BrightWhite:  return L"\033[97m";

    default:                      return L"\033[0m"; // Сброс, если что-то пошло не так
    }
}

// Конвертер для фона (BackgroundColor -> ANSI std::wstring)
std::wstring to_ansi(BackgroundColor color) {
    switch (color) {
    case BackgroundColor::Black:       return L"\033[40m";
    case BackgroundColor::Red:         return L"\033[41m";
    case BackgroundColor::Green:       return L"\033[42m";
    case BackgroundColor::Blue:        return L"\033[44m";
    case BackgroundColor::Yellow:      return L"\033[43m";
    case BackgroundColor::White:       return L"\033[47m";

        // Яркий ANSI-фон (код 107)
    case BackgroundColor::BrightWhite: return L"\033[107m";

    default:                           return L"\033[0m";
    }
}

int ask_color(menucolors MenuColors, bool text = false) {
    if (text) {
        return advansed_chooserC({
            .lines_to_choose = {
                L"Чёрный( !!аккуратней!! , в консоли фон чёрный обычно)",
                L"Синий",
                L"Зелёный",
                L"Cyan(май инглишь кайнда бэд)",
                L"Красный",
                L"Magenta",
                L"Жёлтый",
                L"Белый",
                L"Серый",
                L"ярко синий",
                L"ярко зелёный",
                L"ярко cyan",
                L"ярко красный",
                L"ярко magenta",
                L"ярко жёлтый",
                L"ярко белый ?что?"
            },
            .singleChoice = true,
            .title = L"Выберите цвет: ",
            .menucolors = MenuColors
            }).roots[0] - 1;
    }
    int choice = advansed_chooserC({
            .lines_to_choose = {
                L"Чёрный",
                L"Синий",
                L"Зелёный",
                L"Cyan(май инглишь кайнда бэд)",
                L"Красный",
                L"Magenta",
                L"Жёлтый",
                L"Белый",
                L"Серый",
                L"ярко синий",
                L"ярко зелёный",
                L"ярко cyan",
                L"ярко красный",
                L"ярко magenta",
                L"ярко жёлтый",
                L"ярко белый ?что?"
            },
            .singleChoice = true,
            .title = L"Выберите цвет: ",
            .menucolors = MenuColors
        }).roots[0];
    if (choice == EXIT_CODE) return -1;
    if(choice == 1) return (int)BackgroundColor::Black;
    return (choice-1) * 16;
}

vector<wstring> colors_change(std::vector<std::wstring> settings) {
    menucolors menucolors = parse_menucolors_setting(read_setting((char)settings_pos::colors));
    while (true) {
        
        ChoiceResult choice = advansed_chooserC({
        .lines_to_choose = {L"Tекста линий", L"Активное поле ввода", L"Неактивное поле ввода", L"Подсветка", L"Выбранные / Неактивные линии"},
        .singleChoice = true,
        .title = L"Выберите какой элемент менять: ",
        .menucolors = menucolors,
            });

        if (choice.roots[0] == -1) {
            //save? saved auto
            break;
        }
        
        

        int color_num;
        switch (choice.roots[0]) {
        case 1:
            color_num = ask_color(menucolors, true);if (color_num == -2) break;
            menucolors.lines_c = (TextColor)color_num;break;
        case 2:
            color_num = ask_color(menucolors, true); if (color_num == -2) break;
            menucolors.field_active_c = (TextColor)color_num; break;
        case 3:
            color_num = ask_color(menucolors, true); if (color_num == -2) break;
            menucolors.field_base_c = (TextColor)color_num; break;
        case 4:
            color_num = ask_color(menucolors, false); if (color_num == -1) break;
            menucolors.hover_c = (BackgroundColor)color_num; break;
        case 5:
            color_num = ask_color(menucolors, true); if (color_num == -2) break;
            menucolors.chosen_c = (TextColor)color_num; break;
        }
    }
    std::wstring color_set = std::to_wstring((int)menucolors.field_base_c) + L":" +
                             std::to_wstring((int)menucolors.field_active_c) + L":" +
                             std::to_wstring((int)menucolors.lines_c) + L":" +
                             std::to_wstring((int)menucolors.hover_c) + L":" +
                             std::to_wstring((int)menucolors.chosen_c);
    settings[((int)settings_pos::colors)-1] = color_set;
    return settings;
}



//настройки одного прохода... : зашёл -> изменил -> выкинуло на выбор того что настраивать
std::wstring prog_settings(bool change = false, short num_to_read = NULL) //mode - изменяем ли или нет, num - при вызове из функции определяет строку для чтения
{

    std::vector<wstring> settings = std::get<std::vector<wstring>>(readFile({ .file_path = getFileName(FileType::Settings),.isVector = true}));
    if (change == true) // если изменяем настройки
    {
        //настройка настроек

        size_t main_menu_ch = stoi(sett_menu_options_show({ L"Вид выбора пути", L"Поиск неправильно указанного пути", L"Настройки показа", L"Настройки цветов(поддержка пока не везде)" }));
        wstring sub_menu_ch;
        bool no_sub = false;
        switch (main_menu_ch) {
            case (int)settings_pos::path_choose_view: sub_menu_ch = sett_menu_options_show({ L"Консоль-стандарт", L"Выбор по вин-путям" }); break;
            case (int)settings_pos::if_path_wrong: sub_menu_ch = sett_menu_options_show({ L"Использовать поиск", L"Предлогать перевыбрать путь(из имеющихся) и запускать",L"Предлогать создание нового", L"ничего не делать(кто вообще это выберет? программа просто выдаст ошибку и закроется)"}); break;
            case (int)settings_pos::showlines: sub_menu_ch = sett_menu_options_show({ L"Показывать имена(ваши)",L"Показывать название файла +.exe(или другое расширение) без ваших имен", L"Показывать пути(как раньше)" }); break;
            case (int)settings_pos::colors: settings = colors_change(settings); no_sub = true; break;
            case EXIT_CODE: break;
        }
        //while (settings.size() < 5) {
        //    settings.push_back(L"1"); //перенести в file_cheker(), заполняет недостоющие элементы если есть
        //    if (settings.size() == 5) { settings.pop_back(); settings.push_back(L"0"); } //TODO не работает
        //}
        if (no_sub == false) {
            settings[main_menu_ch - 1] = sub_menu_ch; //основное изменение-меняем выбранную настройку на значение выбора подкатегории
        }

        // Записываем обратно
        std::wofstream fout("settings.txt");
        fout.imbue(std::locale(fout.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
        for (size_t i = 0; i < settings.size(); ++i) {
            fout << settings[i];
            if (i != settings.size() - 1) {
                fout << L'\n';
            }
        }
        fout.close();
        return L"";
    }
    return settings[num_to_read-1];
}




ProgSettings CURRENT_SETTINGS;
//TODO реализовать добавление и чтение menucolors
void read_set() {
    std::vector<std::wstring> sett = std::get<std::vector<std::wstring>>(readFile({ .file_path = getFileName(FileType::Settings), .isVector = true }));
    auto it = PATH_CHOOSE_VIEW.find(sett[0]);
    if (it != PATH_CHOOSE_VIEW.end()) {
        CURRENT_SETTINGS.path_choose_view = it->second;
        CURRENT_SETTINGS.path_view_num = it->first;
    }
    auto it1 = IF_PATH_WRONG.find(sett[1]);
    if (it1 != IF_PATH_WRONG.end()) {
        CURRENT_SETTINGS.if_path_wrong = it1->second;
        CURRENT_SETTINGS.if_wrong_num = it1->first;
    }
    it1 = SHOWLINES.find(sett[2]);
    if (it1 != SHOWLINES.end()) {
        CURRENT_SETTINGS.showlines = it1->second;
        CURRENT_SETTINGS.showlines_num = it1->first;
    }
    CURRENT_SETTINGS.MenuColors = parse_menucolors_setting(sett[(int)settings_pos::colors - 1]); //должны быть всегда, да и вообще find(sett[2]) это архаизм этого приложения уже
}