#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>
#include <locale>
#include <codecvt>
#include <fcntl.h>
#include <io.h>
#include <map>
#include <functional>
#include <shellapi.h>
#include <limits>
#include <vector>
#include <sstream>
#include <ShlObj.h>
#include <comutil.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shlwapi.h>
#include <tuple>
#include <filesystem>
#include <cctype>
#include <variant>
#include <thread>
#include <iomanip>
#include "File_funcs.h"
#include "Groups.h"
#include "python_script_maker.h"
#include "advanced_choice.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "comsuppw.lib")

using std::string;
using std::wstring;
using std::wofstream;
using std::wifstream;
namespace fs = std::filesystem;


std::wstring chooser(const std::wstring& what_choice)
{
    std::wcout << what_choice << L'\n';
    wstring choice;
    //choice.reserve(4); -- было нужно для числовых значений, но есть использование как строки
    std::wcin >> choice;
    std::wcin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    return choice;
}


void countdown(double seconds_to_wait, std::wstring wstring_to_show, int duration) {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    std::wcout << std::fixed << std::setprecision(duration);
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - start;
        double remaining = seconds_to_wait - elapsed.count();

        if (remaining <= 0.0) {
            GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
            cursorInfo.bVisible = TRUE;
            SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
            break;
        }
        std::wcout << L"\r" << wstring_to_show << remaining;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}




std::wstring extract_filename(const std::wstring& path) {
    const size_t last_slash = path.find_last_of(L"\\/");
    return (last_slash == std::wstring::npos) ? path : path.substr(last_slash + 1);
}

std::vector<std::wstring> make_massive_of_wstr() {
    std::vector<std::wstring> wideStrings;
    std::wstring line;

    // Считываем всю строку до Enter
    if (std::getline(std::wcin, line)) {
        std::wistringstream iss(line);
        std::wstring tempStr;

        // Разбиваем строку на слова
        while (iss >> tempStr) {
            wideStrings.push_back(tempStr);
        }
    }
    return wideStrings;
}


static string file_chooser(string file_type, string prog_name="")
{
    string file_name = "progpaths.txt";
    if (file_type == "game")
    {
        file_name = "gamespaths.txt";
    }
    else if (file_type == "link")
    {
        file_name = "linkspath.txt";
    }
    else if (file_type == "group"){ file_name = "groups.txt"; }
    else if (file_type == "sett") { file_name = "settings.txt"; }
    else if (file_type == "script") 
    {
        file_name = "scripts\\script_" + prog_name + ".py";
    }
    else if (file_type == "scripts") { file_name = "scripts.txt"; }
    return file_name;
}

std::tuple<bool, std::wstring> Win_path_selector()
{
    OPENFILENAME ofn;
    wchar_t filePath[MAX_PATH] = L"";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = L"*.exe *.bat *.cmd *.txt *.doc *.url *.py\0*.exe;*.bat;*.cmd;*.txt;*.doc;*.url;*.py\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"Что записать в файл?";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileName(&ofn) == TRUE) {
        wstring tmp = std::wstring(filePath);
        return std::make_tuple(true,tmp);
    }
    else { return std::make_tuple(false, L""); }
        // Пользователь отменил выбор или произошла ошибка
        //DWORD error = CommDlgExtendedError();
}

void If_prog_view_is_2_and_false()
{
    DWORD error = CommDlgExtendedError();
    if (error) {
        std::wcerr << L"Ошибка: " << error << L"\n";
    }
    else {
        std::wcout << L"Выбор файла отменен" << L"\n";
    }
}

int writefile(std::vector<std::wstring> lines, string file_type, string prog_name = "", bool show=true) { //дозаписывает 

    string file_name = file_chooser(file_type, prog_name);
    wofstream fout(file_name, std::ios::app);
    fout.imbue(std::locale(fout.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    short i = 0;
    for (short i = 0; i < lines.size(); i++)
    {
        std::wstring line = lines[i];
        /*
        if (line.substr(0, 5) != L"https" && file_type != "group")
        {
            if (line.size() > 1) {
                std::filesystem::path p(line);
                std::wstring extension = p.extension().wstring();
            }
            std::wstring extension = line;
            if (extension != L".exe" && extension != L".txt" && extension != L".doc" && extension != L".url" && extension != L".bat" && extension != L".cmd" && extension != L".py" && extension != L"1" && extension != L"2" && extension != L"3" && extension != L"4" && extension != L"\n1" && extension != L"\n2" && extension != L"\n3" && extension != L"\n4") {
                    line += L".lnk";
            }
        }
        */
        fout << line << std::endl;
    }
    if(show) { std::wcout << L"Добавлено " << lines.size() << L" путей в файл" << std::endl; }
    fout.close();
    return 0;
}
int writefile(std::wstring line, std::string file_type, string prog_name = "", bool show=true) {
    return writefile(std::vector<std::wstring>{line}, file_type, prog_name, show);
}




std::variant<std::wstring, std::vector<std::wstring>> readFile(std::string file_type, bool isVector = false) {

    std::string file_name = file_chooser(file_type);
    std::wifstream fin(file_name);
    fin.imbue(std::locale(fin.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    std::wstring line;
    if (isVector) {
        std::vector<std::wstring> lines;
        while (std::getline(fin, line)) {
            lines.push_back(line);
        }
        fin.close();
        return lines;
    }
    else {
        short int line_cnt = 0;
        std::wstring content;
        while (std::getline(fin, line)) {
            line_cnt++;          
            std::wstring res = std::to_wstring(line_cnt) + L" " + line;
            content += res + L"\n";
        }
        fin.close();
        return content;
    }
}

void make_txt_for_scripts(std::string directory_path) {
    short c = 0;
    std::wstring file_contents;
    string file_name = file_chooser("scripts");
    if (remove(file_name.c_str()) == 0) { ; } //удаляем старый файл
    else { std::wcout << L"ошибка удаления файла с путями скриптов для перезаписи прости"; }

    for (const auto& entry : std::filesystem::directory_iterator(directory_path)) { //собираем названия скриптов
        if (entry.is_regular_file()) { //проверка на обычный файл, не директорию
            c++;
            std::wstring relativePath = L"scripts\\" + entry.path().filename().wstring(); //создание неполного пути
            std::wstring full_scrPath = std::filesystem::absolute(relativePath).wstring(); //создание полного пути по неполному
            file_contents += full_scrPath + L"\n";
        }
    }

    // Удаляем последний символ если строка не пустая
    if (!file_contents.empty()) {
        file_contents.pop_back();
    }

    writefile(file_contents, "scripts", "", false);
}



//автономно выбирает 1 линию(из txt файла) по номеру
wstring choose_line(short line_number, string file_type) { //добавить определение из какого файла брать
    string file_name = file_chooser(file_type);
    wifstream fin;
    fin.open(file_name);
    fin.imbue(std::locale(fin.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    wstring line;
    wstring res_line;
    short int line_count = 0;
    while (std::getline(fin, line)) {
        line_count++;
        if (line_count == line_number) {
            res_line = line;
            break;
        }
    }
    fin.close();
    return res_line;
}

static std::wstring StringToWstring(const std::string& str) {
    if (str.empty()) return std::wstring();

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr_to(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr_to[0], size_needed);
    return wstr_to;
}


int delete_lines_or_insert_one(std::string file_type, std::vector<int> numbers = {}, bool insert = false, std::wstring line_to_insert = L"", short line_number = NULL, bool show = true)
{
    string file_name = file_chooser(file_type);
    wifstream fread;
    fread.open(file_name);
    fread.imbue(std::locale(fread.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001"))); //получает локаль файла и меняет на широкоформатную...
    if (fread.is_open() == false)
    {
        std::wcerr << "Ошибка открытия файла." << L"\n"; //добавить какой файл в ошибку
        return 1;
    }
    wofstream fwrit("temp.txt");
    fwrit.imbue(std::locale(fwrit.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    if (fwrit.is_open() == false)
    {
        std::wcerr << "Ошибка открытия файла." << L"\n"; //тоже самое
        return 1;
    }
    short int lin_num = 1;
    wstring line;
    if (insert == false)
    {
        while (getline(fread, line)) {
            if (std::find(numbers.begin(), numbers.end(), lin_num) == numbers.end()) {
                fwrit << line << std::endl;
            }
            lin_num++;
        }
    }
    else
    {
        while (getline(fread, line))
        {
            if (lin_num == line_number)
            {
                fwrit << line_to_insert << std::endl;
                lin_num++;
                continue;
            }
            fwrit << line << std::endl;
            lin_num++;
        }
    }
    fread.close();
    fwrit.close();
    if (remove(file_name.c_str()) != 0) {
        std::wcerr << "ошибка удаления старого файла" << L"\n";
        return 1;
    }
    if (rename("temp.txt", file_name.c_str()) != 0) {
        std::wcerr << "ошибка переименовывания нового файла" << L"\n";
        return 1;
    }
    if (show == true) {
        std::wcout << L"Готово\nНовый файл:\n" << L"\n";
        std::wcout << std::get<std::wstring>(readFile(file_type, false));
    }
    return 0;
}

static std::vector<std::tuple<std::wstring, bool>> isPythonInPath(const std::vector<std::string>& libraries) {
    char path[MAX_PATH] = "python.exe";
    std::vector<std::tuple<std::wstring, bool>> answer;
    BOOL found = PathFindOnPathA(path, NULL);
    if (found) {
        for (size_t i = 0; i < libraries.size(); i++) {
            std::string command = "python -c \"import " + libraries[i] + "\" 2>nul";
            int result = std::system(command.c_str());
            answer.push_back(std::make_tuple(StringToWstring(libraries[i]), result == 0));
        }
        return answer;
    }
    else {
        return { std::make_tuple(L"python_not_found", false) };
    }
}

void files_checker() {
    std::vector<std::string> files = { "progpaths.txt", "gamespaths.txt", "linkspath.txt", "groups.txt", "scripts.txt", "settings.txt"};
    wchar_t c;
    bool no_need_full_rewrite = false;
    //для txt файлов
    for (short i = 0; i < files.size(); ++i) {
        if (std::filesystem::exists(files[i]) && files[i] == "settings.txt") { //есть и имя настройки
            std::wifstream file(files[i]);
            file.imbue(std::locale(file.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
            while (file.get(c)) {

                if (!std::isspace(static_cast<unsigned char>(c))) { // Найден символ, который не является пробелом/enter-ом.
                    no_need_full_rewrite = true;
                    file.close();
                    break;
                }
            }
            file.close();
            if (no_need_full_rewrite == true) { //если не пустой проверяем не пустая ли 1 строка
                std::wifstream file(files[i]);
                file.imbue(std::locale(file.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
                wchar_t r;
                while (file.get(r)) {
                    if (std::isspace(static_cast<unsigned char>(r))) {
                        file.close();
                        delete_lines_or_insert_one("sett", {}, true, L"1", 1,false); //функция замены или удаления где true-замена int 1 - позиция строки 
                        break;
                    }
                    file.close(); //закрытие на случай не сработаного условия
                    break;
                }
            }
            if (no_need_full_rewrite == false) {
                if (remove(files[i].c_str()) == 0) { ; }
                else { std::wcout << L"ошибка удаления пустого файла настроек"; }
                wofstream fout(files[i], std::ios::app);
                fout.imbue(std::locale(fout.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
                fout << L"1" << std::endl;
                fout.close();
            }
        }


        if (!std::filesystem::exists(files[i])) { // нету
            std::ofstream(files[i].c_str()).close(); //создание пустого файла
            if (files[i] == "settings.txt") { //стандартная запись для настроек(жизненно важно)
                wofstream fout(files[i], std::ios::app);
                fout.imbue(std::locale(fout.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
                fout << L"1" << L"\n";
                fout.close();
            }
        }

    }
    std::wstring directory_path = L"scripts";
    if (!fs::exists(directory_path) or !fs::is_directory(directory_path)) {
        if (!CreateDirectory(directory_path.c_str(), NULL)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                std::cerr << "Ошибка при создании директории: " << GetLastError() << std::endl;
                return;
            }
        }
    }
    
}

void mass_files_delete(std::vector<int> num_of_indxes, string file_type) {
    std::wstring filename = L"";
    for (char i = 0; i < num_of_indxes.size(); i++) {
        filename = choose_line(num_of_indxes[i]+1, file_type);
        if (fs::remove(filename) != 0) {
            std::wcerr << L"Ошибка при удалении файла: " << filename << L"\n";
        }
    }
    //перезапись txt файла для скриптов
    if (file_type == "scripts") { make_txt_for_scripts("scripts\\"); }
}


std::wstring prog_settings(bool change = false, short num_to_read = NULL) //mode - изменяем ли или нет, num - при вызове из функции определяет строку для чтения
{
    wifstream fin("settings.txt");
    fin.imbue(std::locale(fin.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));

    wstring line;
    short int line_cnt = 0;
    wstring res;
    while (std::getline(fin, line)) {
        line_cnt++;
        if (line_cnt == num_to_read) {
            res = line;
        }
    }
    fin.close();
    if (change == true) // если изменяем настройки
    {
        //настройка настроек
        wstring cho;
        std::wstring chos = std::to_wstring(advansed_chooser({ 
            .lines_to_choose = { L"вид выбора пути",L"поиск неправильно указанного пути;" }, 
            .singleChoice = true, 
            .title = L"Что настраиваем ? : \n" })[0]);
            if (chos == L"1") {
                cho = std::to_wstring(advansed_chooser({ 
                    .lines_to_choose = { L"Консоль-стандарт", L"Выбор по вин-путям;" }, 
                    .singleChoice = true, 
                    .title = L"Что настраиваем?:\n" })[0]);
            }
            else if (chos == L"2") {
                cho = std::to_wstring(advansed_chooser({ 
                    .lines_to_choose = { L"Использовать поиск", L"Предлогать перевыбрать путь и запускать", L"ничего не делать(кто вообще это выберет? программа просто выдаст ошибку и закроется)" }, 
                    .singleChoice = true, 
                    .title = L"Что настраиваем?:\n" })[0]);
            } // 3 кстати реально НИЧЕГО не делает такого условия даже нет хахаххха
        if (std::stoi(chos) <= line_cnt) { delete_lines_or_insert_one("sett", {}, true, cho, std::stoi(chos), false); }
        else if (line_cnt + 1 == std::stoi(chos) - 1) { writefile(L"\n" + cho, "sett"); }
        else { writefile(cho, "sett"); }
        return cho;
    }
    return res;
}

bool isNotSpace(wchar_t c) {
    return !std::isspace(static_cast<wint_t>(c));
}

std::string coords_getter() {
    std::ifstream fin("coords.txt");
    std::string line = "";
    std::getline(fin, line);
    return line;
}


static std::string WstringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    // WC_ERR_INVALID_CHARS гарантирует корректную обработку ошибок
    int size_needed = WideCharToMultiByte(
        CP_UTF8,            // CodePage: UTF-8
        0,                  // dwFlags: 0 (или WC_ERR_INVALID_CHARS)
        wstr.c_str(),       // lpWideCharStr: Исходная wstring
        static_cast<int>(wstr.length()), // cchWideChar: Длина wstring
        nullptr,            // lpMultiByteStr: nullptr для получения размера
        0,                  // cbMultiByte: 0 для получения размера
        nullptr,            // lpDefaultChar: не используется с UTF-8
        nullptr             // lpUsedDefaultChar: не используется с UTF-8
    );

    std::string str_to(size_needed, 0);
    WideCharToMultiByte(
        CP_UTF8,
        0,
        wstr.c_str(),
        static_cast<int>(wstr.length()),
        &str_to[0],         // Буфер назначения
        size_needed,
        nullptr,
        nullptr
    );
    return str_to;
}

void func_linker(short mode, const string& file_type, const wstring prog_view, bool from_start_files = false, short line_num=NULL) {
    /*
    if (prog_view != L"2" && from_start_files == false)
    {
        showfile(file_type); //вывод текущего файла в консоль, нужно из-за чего-то связаного с выбором файла через win_path_selector
    }
    */
    if (mode == 1) { //1-добавить
        if (file_type == "group")
        {
            group_add(prog_view);
            return;
        }
        if (file_type == "scripts") {
            std::vector<std::string> required_libraries = { "pyautogui", "keyboard" };
            std::vector<std::wstring> missing_libraries;
            auto python_check_result = isPythonInPath(required_libraries);

            if (!python_check_result.empty()) {
                auto& first_result = python_check_result[0];
                if (std::get<0>(first_result) == L"python_not_found") {
                    std::wcout << L"\033[31mPython не установлен в PATH, создать скрипт основанный на нём невозможно\033[0m\n";
                    return;
                }

                bool all_libraries_available = true;
                for (const auto& result : python_check_result) {
                    auto library_name = std::get<0>(result);
                    auto is_available = std::get<1>(result);

                    if (!is_available) {
                        all_libraries_available = false;
                        missing_libraries.push_back(library_name);
                    }
                }

                if (!all_libraries_available) {
                    std::wcout << L"\033[33mНе все необходимые библиотеки установлены. Отсутствуют: \033[0m";
                    for (const auto& lib : missing_libraries) {
                        std::wcout << lib << L" ";
                    }
                    std::wcout << L"\n";
                    std::wcout << L"Выполните команду: pip install \'библиотека\' для установки...\n";
                    system("pause");
                    return;
                }
                std::wcout << L"\033[32mPython и все необходимые библиотеки доступны\033[0m\n";
            }

            std::wstring prog_name = chooser(L"Введите название скрипта");
            std::string pr_name = WstringToUtf8(prog_name);
            python_script_make(pr_name);
            return;
        }
        if ((prog_view == L"1" || file_type == "link") && (file_type != "group" || file_type != "scripts")) //если (вид записи в файл - консольный или тип файла - link) и тип файла != группе
        {
            /*
            реализовано добавление через пробел, но надо давать выбор - поэтому нужны ещё настройки(имеется ввиду добавлять через интер пока не 0)
            а также реализация первого через фун. massive maker
            */
            std::vector<std::wstring> lines;
            std::wstring lime;
            std::wcout << L"Что записать в файл?\n";
            std::getline(std::wcin, lime);
            auto it = std::find_if(lime.rbegin(), lime.rend(), isNotSpace); //узнаём 1 символ не пробел с конца строки
            lime.erase(it.base(), lime.end()); //режем по этот символ включ.

            int c = -1;
            int first = 0, second = 0;

            for (int i = 0; i < lime.size(); ++i) {
                c++;
                bool is_path_start = (std::isupper(lime[i]) && (i + 2 < lime.size()) && (lime.substr(i + 1, 2) == L":\\" || lime.substr(i + 1, 2) == L":/"));
                bool is_end = (c == lime.size() - 1);
                if ((is_path_start || is_end) && c != 0) {
                    first = second;
                    second = c;
                    if (is_end) {
                        second += 2;
                    }
                    if (first < second) {
                        lines.push_back(lime.substr(first, second - first - 1));
                    }
                }
            }
            

            if (from_start_files == true) {
                delete_lines_or_insert_one(file_type, {}, true, lime, line_num, false); //заменяем неправильную строку и запускаем правильную
                startfiles(line_num, file_type);
            }
            else {
                writefile(lines, file_type); //стандартный вызов-просто добавление строки в конец
            }
        }
        else
        {
            std::tuple<bool, std::wstring> result = Win_path_selector();
            if (from_start_files == true) {
                if (std::get<0>(result) == true) {
                    delete_lines_or_insert_one(file_type, {}, true, std::get<1>(result), line_num, false);
                    startfiles(line_num, file_type);
                }
                else { If_prog_view_is_2_and_false(); } // отмена поиска
            }
            else {
                if (std::get<0>(result) == true) { writefile(std::get<1>(result), file_type); return; }
                else { If_prog_view_is_2_and_false(); }
            }
        }
    }
    else if (mode == 2) { //удалить
        int choice = 0;
        if (file_type == "group")
        {
            choice = advansed_chooser({ 
                .lines_to_choose = { L"всю группу", L"её некоторые внутренности" }, 
                .singleChoice = true, 
                .title = L"" })[0];
            switch (choice)
            {
                case 1: { break; }
                case 2: { group_del(); break; }
            }

        }
        if (file_type == "scripts")
        {
            std::vector<int> numbers = advansed_chooser({ 
                .lines_to_choose = std::get<std::vector<std::wstring>>(readFile(file_type, true)), 
                .singleChoice = false, 
                .title = L"Выберите номера удаляемых скриптов\n" });
            mass_files_delete(numbers, file_type);
            return;
        }
        if(file_type != "group" || choice == 1) //посути объединение и групп и негрупп... хороший ход
        {
            //std::vector<short> numbers = massive_maker(L"Выберите номера удаляемых строк"); //создание массива чисел для след строки
            std::vector<int> numbers = advansed_chooser({ 
                .lines_to_choose = std::get<std::vector<std::wstring>>(readFile(file_type, true)), 
                .singleChoice = false, 
                .title = L"Выберите номера удаляемых строк\n" });
            delete_lines_or_insert_one(file_type, numbers); //если надо несколько строк удалить -- удаляем несколько
        }
        
    }
    //else if (mode == 3) {
        //создание скриптов
        /*showfile(file_type);
        std::vector<short> numbers = massive_maker(L"Выберите номер нужной вам программы");
        std::wstring path = choose_line(numbers[0], file_type);
        std::wstring prog_name = extract_filename(path);
        std::string pr_name = WstringToUtf8(prog_name);
        python_script_make(pr_name);*/
    //}
}

