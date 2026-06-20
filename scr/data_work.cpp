#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include "data_work.h"
#include "file_io.h"
#include "converter.h"
#include "StartFuncs.h"
#include "path_handler.h"
#include "app_config.h"
#include "advanced_choice.h"
#include "win_help.h"
#include "logger.h"

using std::wstring;

static constexpr auto EXIT_CODE = -1;


std::wstring read_url_from_url_file(const std::wstring& wpath) {
    // Открываем файл .url для чтения
    auto lines = std::get<std::vector<std::wstring>>(readFile({ .file_path = WstringTo_Utf8(wpath),.for_py_code = false, .isVector = true }));

    std::wstring line;
    for (size_t i = 0; i < lines.size(); ++i) {
        // Ищем строку с URL=
        line = lines[i];
        if (line.find(L"URL=") == 0) {
            std::wstring url_wstr = line.substr(4);
            return url_wstr;
        }
    }

    return L"";
}

int is_valid_file_type(const wstring& path) {
    // Сначала проверяем, является ли путь URL
    if (path.find(L"http://") == 0 ||
        path.find(L"https://") == 0 ||
        path.find(L"ftp://") == 0) {
        return 3;
    }

    // Проверяем расширение файла
    size_t dot_pos = path.find_last_of(L'.');
    if (dot_pos == wstring::npos) return 0;

    wstring ext = path.substr(dot_pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), std::towlower);

    // Обработка .lnk файлов
    if (ext == L".lnk") {
        wstring resolved_path = where_are_you_go_lnk(path);

        // Если функция вернула тот же путь (не удалось разрешить)
        if (resolved_path == path) {
            return 2; // Считаем ярлыком программы по умолчанию
        }

        // Рекурсивно проверяем разрешенный путь
        return is_valid_file_type(resolved_path);
    }

    // Обработка .url файлов
    if (ext == L".url") {
        // Пытаемся прочитать URL из .url файла
        wstring url_content = read_url_from_url_file(path);

        if (url_content.empty()) {
            return 2; // По умолчанию считаем программой
        }

        // Если URL - веб-ссылка
        if (url_content.find(L"http://") == 0 ||
            url_content.find(L"https://") == 0 ||
            url_content.find(L"ftp://") == 0) {
            return 3;
        }

        // Если URL ведет на локальный файл
        return 2;
    }

    // Проверка остальных расширений (ваш текущий код)
    if (ext == L".exe" || ext == L".msi" || ext == L".bat" ||
        ext == L".cmd" || ext == L".apk" || ext == L".ipa" ||
        ext == L".iso" || ext == L".rom") {
        return 2;
    }

    if (ext == L".py" || ext == L".js" || ext == L".vbs" ||
        ext == L".ps1" || ext == L".sh" || ext == L".bat") {
        return 4;
    }

    return 0;
}

std::vector<std::wstring> split(const std::wstring& line, wchar_t delim) {
    log(L"called split, line:" + line + L"\ndelim:" + delim + L"\n");
    std::vector<std::wstring> result;
    size_t start = 0;
    size_t end = line.find(delim);
    while (end != std::wstring::npos) {
        result.push_back(line.substr(start, end - start));
        start = end + 1;
        end = line.find(delim, start);
    }
    result.push_back(line.substr(start));
    return result;
}




LineEntry line_parser(FileType type, short line_number = NULL, wstring raw_line = L"") {
    LineEntry entry;
    //entry.path = choose_line(line_number, file_name)
    if (raw_line.empty()) {
        std::string file_name = getFileName(type);
        std::vector lines = std::get<std::vector<std::wstring>>(readFile({ .file_path = file_name, .for_full_read = true, .for_py_code = false, .isVector = true, }));
        raw_line = lines[line_number - 1];


        /*wifstream fin;
        //fin.open(file_name);
        //fin.imbue(std::locale(fin.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
        //wstring line;
        //short int line_count = 0;
        //while (std::getline(fin, line)) {
        //    line_count++;
        //    if (line_count == line_number) {
        //        raw_line = line;
        //        break;
        //    }
        //}
        //fin.close();*/
    }

    size_t first_sep = raw_line.find(L'\"');
    size_t second_sep = raw_line.find(L'\"', first_sep + 1);
    if (first_sep == std::wstring::npos) {
        entry.path = raw_line;
        return entry;
    }
    entry.path = raw_line.substr(0, first_sep);
    if (second_sep != std::wstring::npos) {
        entry.name = raw_line.substr(first_sep + 1, second_sep - first_sep - 1);
        entry.flags = raw_line.substr(second_sep + 1);
    }
    else {
        entry.name = raw_line.substr(first_sep + 1);
    }
    return entry;
}

std::vector<LineEntry> file_parser(FileType type) {
    std::string file_name = getFileName(type);
    auto lines = get<std::vector<std::wstring>>(readFile({ .file_path = file_name,.for_full_read = true, .for_py_code = false, .isVector = true, }));
    if (lines.empty() || (lines.size() == 1 && lines[0].empty())) return {};
    std::vector<LineEntry> result;

    for (auto& l : lines)
        result.push_back(line_parser(FileType::null,NULL, l));
    return result;
}

Group group_parser(const std::wstring& line) {
    log(L"вошли в group_parser");
    std::vector<wstring> parts = split(line, L'|');
    Group group;

    for (size_t i = 0; i < parts.size(); i++)
    {
        if (i == parts.size() - 1)
        {
            // последняя часть — это мета группы
            // "имя группы" + флаги
            auto meta = split(parts[i], L'"');
            if (meta.size() > 1) group.name = meta[1];
            if (meta.size() > 2) group.flags = meta[2];
        }
        else
        {
            if (parts[i].empty()) continue;
            group.entries.push_back(
                line_parser(FileType::null, 0, parts[i])
            );
        }
    }
    return group;
}


std::vector<wstring> showfile(FileType type, wstring setting)
{
    std::vector<LineEntry> file_entry = file_parser(type);
    if (file_entry.empty()) return {};
    std::vector<wstring> result;

    bool gr_or_lnk = ((type == FileType::Link) || type == FileType::Group);
    int sett = stoi(setting);

    int count = 1;

    for (const auto& entry : file_entry)
    {
        wstring display;

        switch (sett)
        {
        case 1:
            if (!entry.name.empty()) { display = entry.name; }
            else if (!gr_or_lnk) { display = extract_filename(entry.path); }
            else { display = entry.path; }
            break;
        case 2:
            display = extract_filename(entry.path);
            break;

        case 3:
            display = entry.path;
            break;
        }
        result.push_back(display);
    }

    return result;
}
std::vector<wstring> show_groups(wstring setting)
{
    std::vector<wstring> result = {};

    auto lines = std::get<std::vector<wstring>>(
        readFile({ .file_path = getFileName(FileType::Group), .for_full_read = true, .isVector = true })
    );
    /*for (auto l : lines) {
        std::wcout << l << L'\n';
    }*/
    //Sleep(10000);
    std::wstring line_to_show;
    int sett = stoi(setting);
    for (int i = 0; i < lines.size(); i++)
    {
        if (lines[i].empty()) continue;
        Group group = group_parser(lines[i]);
        for (const LineEntry& entry : group.entries) {
            if (entry.path.starts_with(L"https") or entry.path.starts_with(L"http")) { line_to_show += entry.path+L"|"; continue; }
            switch (sett)
            {
            case 1:
                if (!entry.name.empty()) { line_to_show += entry.name; }
                else line_to_show += extract_filename(entry.path);
                break;
            case 2:
                line_to_show += extract_filename(entry.path);
                break;

            case 3:
                line_to_show += entry.path;
                break;
            }
            if (!entry.flags.empty()) {line_to_show += L" Флаги:" + entry.flags + L"|"; }
            else { line_to_show += L"|"; }
        }
        if (!line_to_show.empty()) result.push_back(line_to_show);
        line_to_show.clear();
    }
    return result;
}

enum class FLAGS {
    Asadmin = 1, CloseAfter = 2
};

std::map<std::wstring, int> flagPriority = {
    {Flags::Asadmin, 90},
    {Flags::CloseAfter, 50}
};

//в убывающем
void sort_flags(std::vector<std::wstring>& flags) {
    std::sort(flags.begin(), flags.end(), [](const std::wstring& a, const std::wstring& b) {
        // Если флага нет в карте, даем ему очень большой вес (в конец)
        int priorityA = flagPriority.count(a) ? flagPriority[a] : 0;
        int priorityB = flagPriority.count(b) ? flagPriority[b] : 0;
        return priorityA > priorityB;
        });
}
//добавить выбор-пояснить каждый, после выбора которого вылезет текст с объяснением каждого флага, пока не выйдет пользователь
//возвращает строку с флагами, например: "Asadmin:CloseAfter"
std::wstring make_flags() {
    std::vector<std::wstring> flagos = { L"Asadmin", L"CloseAfter"};
    //предпологается флаги идут с другим разделителем - : например "Asadmin:CloseAfter будет значить запуск задачи и закрытие программы(console) после завершения запуска
    wstring flags = L"";
    int choise;
    bool end = false;


    auto addFlag = [&](const std::wstring& flagName) {
        if (!flags.empty()) flags += L":";
        flags += flagName;
        for (int i = flagos.size() - 1; i >= 0; --i) {
            if (flagos[i] == flagName)
                flagos.erase(flagos.begin() + i);
        }
        };
    int delete_count = 0;
    while (!flagos.empty()) {
        choise = advansed_chooser({ .lines_to_choose = flagos,.singleChoice = true,.title = L"Выбери флаг для добавления:\n" })[0];
        if (choise == EXIT_CODE) break;
        int idx = choise - 1;
        if (idx < 0 || idx >= flagos.size()) continue;

        std::wstring selected = flagos[idx];
        if (selected == L"Asadmin") {
            makeTaskAdmin();
            addFlag(L"Asadmin");
        }
        else if (selected == L"CloseAfter") {
            addFlag(L"CloseAfter");
        }
    }
    return flags;
}

std::vector<std::wstring> translate_script_insides(std::vector<std::wstring>& not_translated_insides) {
    std::vector<std::wstring> translated_lines = {};
    for (const auto& line : not_translated_insides) {
        if (line.size() < 5) {
            continue;
        }

        size_t pos = line.find(L"(");
        if (pos != std::wstring::npos) {
            std::wstring func_name = line.substr(0, pos);
            auto it = sorted_dict.find(func_name);
            //std::wcout << it->second.first << line.substr(pos) << std::endl;
            //std::wcout << line.substr(pos); Sleep(3000);
            if (line.substr(pos) == L"()") { translated_lines.push_back(L"клик на предпологаемом месте мышки"); }
            else { translated_lines.push_back(it->second + line.substr(pos)); }
        }
    }
    return translated_lines;
}