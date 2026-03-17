#include "settings.h"
#include "File_funcs.h"
#include "advanced_choice.h"
#include <locale>
#include <fstream>
#include <span>

using std::wifstream;
using std::wstring;
using std::vector;
static constexpr auto EXIT_CODE = -1;

const int path_choose_view = 1;
const int if_path_wrong = 2;
const int showlines = 3;
const int flags = 4;
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
static vector<wstring> flags_ch_menu(vector<wstring> settings) {
    vector<wstring> v_flags(settings.begin() + flags_start_indx, settings.end()); //массив флагов(пока -а) {1}
    //std::wcout v_flags[0];Sleep(10000);
    //if выбор в chooser = индекс - 1 меняем значение на другое
    int index = 0;
    wstring ask_create_scr;
    vector<wstring> flags_options;
    while (index != EXIT_CODE - 1) {
        ask_create_scr = make_flag_wstr(L"Предлагать создать скрипт: ", v_flags[0]);
        flags_options = { ask_create_scr };
        index = stoi(sett_menu_options_show(flags_options, true)) - 1;
        if (index < 0) { break; }
        change_bool(v_flags, index);
    }
    auto startPos = settings.end() - v_flags.size();
    std::copy(v_flags.begin(), v_flags.end(), startPos);
    return settings;
}

//настройки одного прохода... : зашёл -> изменил -> выкинуло на выбор того что настраивать
std::wstring prog_settings(bool change = false, short num_to_read = NULL) //mode - изменяем ли или нет, num - при вызове из функции определяет строку для чтения
{
    std::vector<wstring> settings = std::get<std::vector<wstring>>(readFile({ .file_type = "sett",.isVector = true}));
    if (change == true) // если изменяем настройки
    {
        //настройка настроек

        size_t main_menu_ch = stoi(sett_menu_options_show({ L"Вид выбора пути", L"Поиск неправильно указанного пути", L"Настройки показа", L"Настройки флагов" }));
        wstring sub_menu_ch;
        bool no_sub = false;
        switch (main_menu_ch) {
            case path_choose_view: sub_menu_ch = sett_menu_options_show({ L"Консоль-стандарт", L"Выбор по вин-путям" }); break;
            case if_path_wrong: sub_menu_ch = sett_menu_options_show({ L"Использовать поиск", L"Предлогать перевыбрать путь и запускать", L"ничего не делать(кто вообще это выберет? программа просто выдаст ошибку и закроется)" }); break;
            case showlines: sub_menu_ch = sett_menu_options_show({ L"Показывать имена(ваши)",L"Показывать название файла +.exe(или другое расширение) без ваших имен", L"Показывать пути(как раньше)" }); break;
            case flags: settings = flags_ch_menu(settings); no_sub = true; break;
            case EXIT_CODE: break;
        }
        while (settings.size() < 3) {
            settings.push_back(L"1");
        }
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












































const std::map<std::wstring, std::wstring> PATH_CHOOSE_VIEW = {
    {L"1", L"Консоль-страндарт"},
    {L"2", L"По вин-путям"}
};

const std::map<std::wstring, std::wstring> IF_PATH_WRONG = {
    {L"1", L"Использовать поиск"},
    {L"2", L"Перевыбор"},
    {L"3", L"Ничего не делать"}
};

const std::map<std::wstring, std::wstring> SHOWLINES = {
    {L"1", L"показывать имена"},
    {L"2", L"показывать название файла+.exe(или другое расширение)"},
    {L"3", L"показывать пути(как раньше)"}
};

ProgSettings CURRENT_SETTINGS;

void read_set() {
    std::vector<std::wstring> sett = std::get<std::vector<std::wstring>>(readFile({ .file_type = "sett", .isVector = true }));
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
}
