#include <string>
#include <variant>
#include <vector>
#include <locale> // для codecvt_byname
#include <fstream>
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <unordered_map>
#include "file_io.h"
#include "app_config.h"
#include "data_work.h"
#include "logger.h"
#include "converter.h"

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

void make_txt_for_scripts(std::string directory_path){
    std::wstring file_contents;
    std::string file_name = FILE_NAMES.at(FileType::Script);



    // 1. Читаем старые ID из существующего файла (если есть)
    std::unordered_map<std::wstring, int> old_ids; // путь -> id
    auto old_lines = get<std::vector<std::wstring>> (readFile({ .file_path = file_name, .for_full_read = true, .isVector = true }));
    if (!old_lines.empty()) {
        for (const auto& line : old_lines) {
            int id = find_line_id(line);
            if (id != 0) {
                // Извлекаем путь без ID
                size_t pos = line.find(L'\"');
                if (pos != std::wstring::npos) {
                    std::wstring path = line.substr(0, pos);
                    old_ids[path] = id;
                }
            }
        }
    }

    int max_id = get_max_id(FileType::Script);


    std::vector<std::wstring> current_scripts;
    for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
        if (entry.is_regular_file()) {
            std::wstring relativePath = L"scripts\\" + entry.path().filename().wstring();
            std::wstring full_scrPath = std::filesystem::absolute(relativePath).wstring();
            current_scripts.push_back(full_scrPath);
        }
    }

    //Сорт
    std::sort(current_scripts.begin(), current_scripts.end());

    // 4. Формируем новые строки с ID
    for (const auto& path : current_scripts) {
        int id;
        auto it = old_ids.find(path);
        if (it != old_ids.end()) {
            id = it->second;  // Старый ID сохраняем
        }
        else {
            id = ++max_id;    // Новый скрипт — новый ID
        }
        file_contents += path + L"\"" L"\"" L"*" + std::to_wstring(id) + L"\n";
    }

    
    if (!file_contents.empty()) {
        file_contents.pop_back();
    }
    if (remove(file_name.c_str()) == 0) { ; } //удаляем старый файл
    else { std::wcout << L"ошибка удаления файла с путями скриптов для перезаписи прости"; }
    writefile(file_contents, file_name, "", false);
}

//перезапись по сути
void save_file(FileType type) {
    //удаляем старый
    std::wofstream fout(FILE_NAMES.at(type));

    // удалось ли открыть файл
    if (!fout.is_open()) {
        log(L"Ошибка при открытии/создании файла: " + StringToWstring(FILE_NAMES.at(type)));
        return;
    }
    fout.imbue(std::locale(fout.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    for (const auto& line_entry : global_all_lines[type]) {
        fout << line_entry.path+L"\""+line_entry.name+L"\""+line_entry.flags+L"*"+std::to_wstring(line_entry.id) << std::endl;
    }
    fout.close();
}
void mass_files_delete(std::vector<int> human_nums, FileType type) {
    std::wstring filename = L"";
    for (char i = 0; i < human_nums.size(); i++) {
        filename = global_all_lines[type][human_nums[i]].path;
        if (fs::remove(filename) == false) {
            std::wcerr << L"Ошибка при удалении файла: " << filename << L"\n";
        }
    }
    //перезапись txt файла для скриптов
    if (type == FileType::Script) { make_txt_for_scripts("scripts\\"); }
}
//автономно выбирает 1 линию(из txt файла) ПРИНИМАЕТ не индекс, а человеческое без raw = true читает до первого "
template <typename TStream>
static bool prepareFileStream(TStream& stream) {

    if (!stream.is_open()) {
        std::wcerr << L"Ошибка открытия файла." << L"\n";
        return false;
    }
    stream.imbue(std::locale(stream.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    return true;
}
//фунция-танк для штурма маленького домика...
int delete_lines_or_insert_or_add_one(FileType type, std::vector<int> numbers = {}, bool insert = false,
    std::wstring line_to_insert_or_add = L"", short line_number = NULL, bool show = true, bool add = false
) {

    std::string file_name = getFileName(type);
    std::wifstream fread(file_name);
    if (!prepareFileStream(fread)) return 1;
    std::wofstream fwrit("temp.txt");
    if (!prepareFileStream(fwrit)) return 1;
    short int lin_num = 1;
    std::wstring line;
    //логика
    if (insert == false && add == false) {
        while (getline(fread, line)) {
            if (std::find(numbers.begin(), numbers.end(), lin_num) == numbers.end()) {
                fwrit << line << std::endl;
            }
            lin_num++;
        }
    }
    else {
        while (getline(fread, line)) {
            if (lin_num == line_number) {
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
        log(L"ошибка удаления старого файла\n");
        return 1;
    }
    if (rename("temp.txt", file_name.c_str()) != 0) {
        log(L"ошибка переименовывания нового файла\n");
        return 1;
    }
    if (show == true) {
        std::wcout << L"Готово\nНовый файл:\n" << L"\n";
        std::wcout << std::get<std::wstring>(readFile({ .file_path = file_name,  .for_py_code = false, .isVector = false }));
    }
    return 0;
}


//максимально быстро на winAPI с ранним выходом