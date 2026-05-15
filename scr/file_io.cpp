#include <string>
#include <variant>
#include <vector>
#include <locale> // для codecvt_byname
#include <fstream>
#include <iostream>
#include <filesystem>
#include <windows.h>
#include "file_io.h"
#include "app_config.h"
#include "data_work.h"

namespace fs = std::filesystem;

int writefile(std::vector<std::wstring> lines, std::string file_path, std::string prog_name, bool show) { //дозаписывает 
    std::wofstream fout(file_path, std::ios::app);
    fout.imbue(std::locale(fout.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    for (short i = 0; i < lines.size(); i++)
    {
        std::wstring line = lines[i];
        fout << line << std::endl;
    }
    if (show) { std::wcout << L"Добавлено " << lines.size() << L" путей в файл" << std::endl; }
    fout.close();
    return 0;
}
int writefile(std::wstring line, std::string file_path, std::string prog_name, bool show) {
    return writefile(std::vector<std::wstring>{line}, file_path, prog_name, show);
}

std::variant<std::wstring, std::vector<std::wstring>> readFile(ReadOptions options) {
   
    std::wifstream fin(options.file_path);
    fin.imbue(std::locale(fin.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    std::wstring line;


    size_t pos;
    if (options.isVector) {
        std::vector<std::wstring> lines;
        while (std::getline(fin, line)) {
            pos = line.find(L'\"');
            if (pos != std::wstring::npos and options.for_full_read != true) { lines.push_back(line.substr(0, pos)); }
            else { lines.push_back(line); }
        }
        fin.close();
        return lines;
    }
    else {
        short int line_cnt = 0;
        std::wstring content;
        while (std::getline(fin, line)) {
            line_cnt++;
            std::wstring res;
            if (options.for_py_code == false) {
                res = std::to_wstring(line_cnt) + L" " + res;
            }
            else {
                res = line; // для питона просто читаем код скрипта
            }

            content += res + L"\n";
        }
        fin.close();
        return content;
    }
}

void make_txt_for_scripts(std::string directory_path) {
    short c = 0;
    std::wstring file_contents;
    std::string file_name = (FILE_NAMES.at(FileType::Script));
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

    writefile(file_contents, file_name, "", false);
}

//автономно выбирает 1 линию(из txt файла)
std::wstring choose_line(short line_number, FileType type, bool raw) { //добавить определение из какого файла брать
    std::string file_name = getFileName(type);
    std::wifstream fin;
    fin.open(file_name);
    fin.imbue(std::locale(fin.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    std::wstring line;
    std::wstring res_line;
    short int line_count = 0;
    int pos;
    while (std::getline(fin, line)) {
        line_count++;
        if (line_count == line_number) {
            pos = line.find(L'\"');
            if (pos != std::wstring::npos and raw == false) { res_line = line.substr(0, pos); break; } //выбираем только путь
            else if (pos != std::wstring::npos and raw == true) { res_line = line; break; }
            res_line = line;
            break;
        }
    }
    fin.close();
    return res_line;
}


int delete_lines_or_insert_or_add_one(FileType type, std::vector<int> numbers = {}, bool insert = false, std::wstring line_to_insert_or_add = L"", short line_number = NULL, bool show = true, bool add = false)
{
    
    std::string file_name = getFileName(type);
    std::wifstream fread;
    fread.open(file_name);
    fread.imbue(std::locale(fread.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001"))); //получает локаль файла и меняет на широкоформатную...
    if (fread.is_open() == false)
    {
        std::wcerr << "Ошибка открытия файла." << L"\n"; //добавить какой файл в ошибку
        return 1;
    }
    std::wofstream fwrit("temp.txt");
    fwrit.imbue(std::locale(fwrit.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    if (fwrit.is_open() == false)
    {
        std::wcerr << "Ошибка открытия файла." << L"\n"; //тоже самое
        return 1;
    }
    short int lin_num = 1;
    std::wstring line;
    if (insert == false && add == false)
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
                if (add) {
                    fwrit << line << L"\n" << line_to_insert_or_add;
                }
                else {
                    fwrit << line_to_insert_or_add << std::endl;
                }
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
        std::wcout << std::get<std::wstring>(readFile({ .file_path = file_name,  .for_py_code = false, .isVector = false }));
    }
    return 0;
}

void files_checker() {
    std::vector<std::string> files = { "progpaths.txt", "gamespaths.txt", "linkspath.txt", "groups.txt", "scripts.txt", "settings.txt" };
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
                        delete_lines_or_insert_or_add_one(FileType::Settings, {}, true, L"1", 1, false); //функция замены или удаления где true-замена int 1 - позиция строки 
                        break;
                    }
                    file.close(); //закрытие на случай не сработаного условия
                    break;
                }
            }
            if (no_need_full_rewrite == false) {
                if (remove(files[i].c_str()) == 0) { ; }
                else { std::wcout << L"ошибка удаления пустого файла настроек"; }
                std::wofstream fout(files[i], std::ios::app);
                fout.imbue(std::locale(fout.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
                fout << L"1" << std::endl;
                fout.close();
            }
        }


        if (!std::filesystem::exists(files[i])) { // нету
            std::ofstream(files[i].c_str()).close(); //создание пустого файла
            if (files[i] == "settings.txt") { //стандартная запись для настроек(жизненно важно)
                std::wofstream fout(files[i], std::ios::app);
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

void mass_files_delete(std::vector<int> num_of_indxes, FileType type) {
    std::wstring filename = L"";
    for (char i = 0; i < num_of_indxes.size(); i++) {
        filename = choose_line(num_of_indxes[i] + 1, type);
        if (fs::remove(filename) != 0) {
            std::wcerr << L"Ошибка при удалении файла: " << filename << L"\n";
        }
    }
    //перезапись txt файла для скриптов
    if (type == FileType::Script) { make_txt_for_scripts("scripts\\"); }
}