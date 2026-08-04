#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <charconv>
#include <system_error>
#include <tuple>

#include "data_work.h"
#include "file_io.h"
#include "converter.h"
#include "StartFuncs.h"
#include "path_handler.h"
#include "app_config.h"
#include "advanced_choice.h"
#include "win_help.h"
#include "logger.h"
#include "settings.h"
#include <format>
#include "ui_interactions.h"
#include <unordered_map>
#include <unordered_set>
#include "Changer.h"

using std::wstring;

static constexpr auto EXIT_CODE = -1;



//возвращает указатель на entry в FileType vector<entries>
const LineEntry* get_entry_by_id(FileType type, int id) {
    auto itType = global_all_lines.find(type);
    if (itType == global_all_lines.end()) return nullptr;
    for (const auto& entry : itType->second) {
        if (id == entry.id) {
            return &entry;
        }
    }
    return nullptr;
}


//file_parser только без файла-выдаст массив указателей на все LineEntry внутри FileType type могут устареть и стать висячими ЧТЕНИЕ
std::vector<const LineEntry*> get_entries_Rptrs(FileType type) {
    //reserve?
    auto it = global_all_lines.find(type);
    if (it == global_all_lines.end()) {
        return {};
    }
    const auto& lines_vec = it->second;
    std::vector<const LineEntry*> entries;
    entries.reserve(lines_vec.size());
    for (const LineEntry& entry : lines_vec) {
        entries.push_back(&entry);
    }
    return entries;
}
//изминение и чтение
std::vector<LineEntry*> get_entries_Mptrs(FileType type) {
    auto it = global_all_lines.find(type);
    if (it == global_all_lines.end()) return {};

    std::vector<LineEntry*> entries;
    entries.reserve(it->second.size());

    for (LineEntry& entry : it->second) {
        entries.push_back(&entry);
    }
    return entries;
}





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

//возвращает id линии или -1 если нету. можно указать откуда начинать(уже пропустит сам 1 символ)
int find_line_id(const std::wstring& raw_line) {
    // Ищем '*' с конца (так как ID в конце)
    size_t pos = raw_line.rfind(L'*');
    if (pos == std::wstring::npos || pos + 1 >= raw_line.size()) {
        return -1;
    }

    int id = 0;
    for (size_t i = pos + 1; i < raw_line.size(); ++i) {
        wchar_t ch = raw_line[i];
        if (ch >= L'0' && ch <= L'9') {
            id = id * 10 + (ch - L'0');
        }
        else {
            return 0;  // Если после звездочки не число
        }
    }
    return id;
}

LineEntry line_parser(FileType type, short line_number = NULL, wstring raw_line = L"") {
    LineEntry entry;
    //entry.path = choose_line(line_number, file_name)
    if (raw_line.empty()) {
        std::string file_name = getFileName(type);
        std::vector lines = std::get<std::vector<std::wstring>>(readFile({ .file_path = file_name, .for_full_read = true, .for_py_code = false, .isVector = true, }));
        raw_line = lines[line_number - 1];
    }

    //старая версия -2 прохода а с id - 3
    size_t first_sep = raw_line.find(L'\"');
    size_t second_sep = raw_line.find(L'\"', first_sep + 1);
        
    
    if (first_sep == std::wstring::npos) {
        entry.path = raw_line;
        return entry;
    }
    entry.path = raw_line.substr(0, first_sep);
    if (second_sep != std::wstring::npos) {//есть 2, значит и 1 тожк
        size_t id_sep = raw_line.rfind(L'*');//моментально
        entry.name = raw_line.substr(first_sep + 1, second_sep - first_sep - 1);
        entry.flags = raw_line.substr(second_sep + 1,id_sep-second_sep-1);
        entry.id = find_line_id(raw_line);//без 2 разделителя id не будет, выйдет, что каждая линия будет такой: C:/path""*id даже если не будет ничего, id будет
    }
    else if (first_sep != std::wstring::npos and second_sep == std::wstring::npos){//есть 1, нет 2
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
        result.push_back(line_parser(type,NULL, l));
    return result;
}

//ВАЖНО все указатели на global_all_lines после этого слетят. добавляет элементы по FileType.
void add_entries(FileType type, std::vector<LineEntry>& line_entries) {
    auto& entries = global_all_lines[type];
    entries.reserve(entries.size() + line_entries.size());
    for (const auto& entry : line_entries) {
        global_all_lines[type].push_back(entry);
    }
    save_file(type);
}
//вот тут точно ВСЁ связанное не с id слетит
void delete_entries_by_IDs(FileType type, std::vector<int> IDs) {
    std::unordered_set<decltype(LineEntry::id)> id_set(IDs.begin(), IDs.end());
    auto& entities = global_all_lines[type];
    
    std::erase_if(entities, [&id_set](const LineEntry& entry) {
        return id_set.contains(entry.id);
        });
    
    save_file(type);
}
//только, чтобы не смущать пользователя

void delete_entries_by_positions(FileType type, const std::vector<int>& positions) {
    auto& entries = global_all_lines[type];

    // Сортируем позиции в обратном порядке (чтобы не сбивать индексы)
    std::vector<int> sorted_positions = positions;
    std::sort(sorted_positions.begin(), sorted_positions.end(), std::greater<int>());

    for (int pos : sorted_positions) {
        // pos — 1-based
        if (pos >= 1 && pos <= static_cast<int>(entries.size())) {
            entries.erase(entries.begin() + (pos - 1));
        }
    }
    save_file(type);
}

//предпологает что new_line_entry имеет старый id \
ты сам можешь сделать массив указателей и изменить только нужный...
void replace_entity(ReplaceOptions options) {
    
    auto& lines = global_all_lines[options.type];
    auto it = std::find_if(lines.begin(), lines.end(),
        [&options](const auto& entry) { return entry.id == options.old_line.id; });

    // Если строка не найдена, ничего не делаем
    if (it == lines.end()) return;
    if (options.full) {
        *it = options.new_line_entry; // Полная замена объекта
    }
    else {
        if (options.path)  it->path = options.new_line_entry.path;
        if (options.name)  it->name = options.new_line_entry.name;
        if (options.flags) it->flags = options.new_line_entry.flags;
    }
    save_file(options.type);
}




Group group_parser(const std::wstring& line_entry_path) {
    log(L"вошли в group_parser");
    std::vector<wstring> parts = split(line_entry_path, L'|');
    Group group;
    std::vector<FileType> file_types = {};
    std::vector<int> IDs = {};
    std::vector<std::wstring> slach = {};
    for (size_t i = 0; i < parts.size(); i++)
    {
        if (i == parts.size() - 1)
        {
            // последняя часть — это мета группы
            // "имя группы" + флаги
            break;//и забили хер, так как имя и флаги есть в LineEntry
        }
        else
        {
            if (parts[i].empty()) continue;
            slach = split(parts[i], L'-');
            file_types.push_back(static_cast<FileType>(stoi(slach[0]))); //опасненько
            IDs.push_back(stoi(slach[1]));
        }
    }
    group.IDs = IDs;
    group.types = file_types;
    return group;
}

//выдаёт имя линии, если есть, если нету .exe имя или если showlines_num == 3 то путь
std::wstring_view get_name_accords_sett(const LineEntry* line_entry_Rptr, const std::wstring& sett, FileType type) {
    if (line_entry_Rptr == nullptr) {
        log(L"\nget_name_accords_sett get empty ptr to LineEntry\n");
        return L""; // Возвращает пустой string_view
    }

    std::wstring_view display;
    int setting = stoi(sett);
    bool gr_or_lnk = ((type == FileType::Link) || type == FileType::Group);

    switch (setting)
    {
    case 1:
        if (!line_entry_Rptr->name.empty()) {
            display = line_entry_Rptr->name;
        }
        else if (!gr_or_lnk) {
            display = extract_filename(line_entry_Rptr->path);
        }
        else {
            display = line_entry_Rptr->path;
        }
        break;
    case 2:
        display = extract_filename(line_entry_Rptr->path);
        break;
    case 3:
        display = line_entry_Rptr->path;
        break;
    }

    return display;
}


std::vector<wstring> showfile(FileType type, wstring& setting)
{
    const auto& lines = global_all_lines[type];

    std::vector<std::wstring> result;
    result.reserve(lines.size());
    for (const auto& entry : lines) {
        result.push_back(std::wstring(get_name_accords_sett(&entry, setting, type)));
    }
    return result;
}
std::vector<std::wstring> show_groups(std::wstring setting) {
    std::vector<std::wstring> result;
    result.reserve(global_all_lines[FileType::Group].size());

    std::wstring line_to_show;

    for (const auto& line : global_all_lines[FileType::Group]) {
        Group group = group_parser(line.path);

        if (!line.name.empty()) {
            result.push_back(line.name);
            continue;
        }

        for (size_t i = 0; i < group.IDs.size(); ++i) {
            int current_id = group.IDs[i];
            FileType current_type = group.types[i];

            const LineEntry* entry = get_entry_by_id(current_type, current_id);
            if (entry == nullptr) continue;

            line_to_show += get_name_accords_sett(entry, setting, current_type);

            if (!entry->flags.empty()) {
                line_to_show += L" Флаги:";
                line_to_show += entry->flags;
                line_to_show += L"|";
            }
            else {
                line_to_show += L"|";
            }
        }

        if (!line_to_show.empty()) {
            result.push_back(std::move(line_to_show));
        }
        line_to_show.clear();
    }
    return result;
}
//для 1-based вывода advanced_chooser
enum class FLAGS {
    Asadmin = 1, CloseAfter = 2, Children = 3, Separate = 4
};


std::map<std::wstring, int> flagPriority = {
    {Flags::Asadmin, 90},
    {Flags::Separate, 80},
    {Flags::CloseAfter, 50}
};

const std::vector<std::wstring> flaggs = {
        L"Asadmin",
        L"CloseAfter",
        L"Children",
        L"Separate"
};

FlagsContents flags_parser(const std::wstring& flags) {
    std::vector<std::wstring> splited_by_colon = split(flags, L':'); //{L"Asadmin",L"CloseAfter",L"Children{2-1,22}{0-2}"}
    std::vector<int> all_firsts_braces = {};
    std::vector<int> all_end_braces = {}; //предпологается что равен all_firsts_braces по size
    std::wstring line = L"";
    FlagsContents flags_contents;
    int counter = 0;
    for (size_t i = 0; i < splited_by_colon.size(); ++i) {
        line = splited_by_colon[i];
        if (line.starts_with(flaggs[0])) {
            flags_contents.Asadmin = splited_by_colon[i];
        }
        else if (line.starts_with(flaggs[1])) {
            flags_contents.CloseAfter = splited_by_colon[i];
        }
        else if (line.starts_with(flaggs[2])) {
            size_t pos = 8; // после "Children"

            while (pos < line.size() && line[pos] == L'{') {
                size_t end = line.find(L'}', pos);
                if (end == std::wstring::npos) break;

                // Извлекаем содержимое { ... }
                std::wstring content = line.substr(pos + 1, end - pos - 1);
                // content = "2-1,2"

                // Находим разделитель тип-ID
                size_t dash = content.find(L'-');
                if (dash != std::wstring::npos) {
                    // Парсим тип (число до '-')
                    std::wstring type_str = content.substr(0, dash);
                    int type_int = std::stoi(type_str);
                    FileType file_type = static_cast<FileType>(type_int);

                    // Парсим ID (числа после '-')
                    std::wstring ids_str = content.substr(dash + 1);
                    std::vector<int> children_IDs;

                    if (!ids_str.empty()) {
                        auto id_parts = split(ids_str, L',');
                        for (const auto& id_str : id_parts) {
                            children_IDs.push_back(std::stoi(id_str));
                        }
                    }

                    if (!children_IDs.empty()) {
                        flags_contents.Children[file_type] = children_IDs;
                    }
                }
                pos = end + 1;
            }
        }
        else if (line.starts_with(flaggs[3])) {
            flags_contents.Separate = splited_by_colon[i];
        }
    }
    return flags_contents;
}
//в убывающем
void sort_flags(std::vector<std::wstring>& flags) {
    std::sort(flags.begin(), flags.end(), [](const std::wstring& a, const std::wstring& b) {
        // Если флага нет в карте, даем ему очень маленький вес (в конец)
        int priorityA = flagPriority.count(a) ? flagPriority[a] : 0;
        int priorityB = flagPriority.count(b) ? flagPriority[b] : 0;
        return priorityA > priorityB;
        });
}
//добавить выбор-пояснить каждый, после выбора которого вылезет текст с объяснением каждого флага, пока не выйдет пользователь
//возвращает строку с флагами, например: "Asadmin:CloseAfter"
//по ссылке изменяе
static std::wstring add_childs(std::wstring prompt = L"") {
    //план: someflags:children{FileType-1,2,3}{FileType-5} где FileType-число
    std::wstring childs_flag = L"Children";
    while (true) {
        auto [file_type, chosen] = ask_lines_in_file_type(prompt);
        if (file_type == FileType::null) break;
        if (chosen.size() == 0 or chosen[0] == EXIT_CODE) {
            //TODO перенести эту часть(выбор) в UI goddaym или сделать какую-то кастомную функцию когда пользователь ничего не выбрал по типу ASCII арта, так как это бывает часто
            colorfulPrint(L"Вы ничего не выбрали", PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
            Sleep(1500);
            continue;
        }
        std::vector<int> ids = {};
        for (int i = 0; i < chosen.size(); ++i) {
            ids.push_back(global_all_lines[file_type][chosen[i]-1].id);
            //ids.push_back(find_line_id(choose_line(chosen[i], file_type, true)));
        }
        //сборка флага:
        childs_flag += L"{" + std::to_wstring((int)file_type) + L"-";
            //std::format(L"{{{}-", file_type); //пример с Games:childs{0-
        int c = 0;
        for (auto& id : ids) {
            c++;
            if (c != ids.size()) {
                childs_flag += std::to_wstring(id) + L",";
                continue;
            }
            childs_flag += std::to_wstring(id) + L"}";
                
        }

    }
    return childs_flag;
}

static std::vector<int> flags_to_vec(const FlagsContents& fcontent, bool force_children = false) {
    std::vector<int> result;

    if (!fcontent.Asadmin.empty()) result.push_back((int)FLAGS::Asadmin);
    if (!fcontent.CloseAfter.empty()) result.push_back((int)FLAGS::CloseAfter);
    if (!fcontent.Separate.empty()) result.push_back((int)FLAGS::Separate);
    if (!fcontent.Children.empty() or force_children) result.push_back((int)FLAGS::Children);
    return result;
}



/*принимает флаги которые уже есть и показывает какие можно добавить TODO-кинуть в UI iteractions, TODO в title писть что уже есть
* возвращает новые флаги
*/ 
std::wstring choose_and_make_flags(std::wstring already_in_use, bool force_children) {
    std::vector<int> preselected = {}; //0-based
    std::vector<std::wstring> flagos_to_show = {
        L"\033[36mAS ADMIN\033[0m -- запускает всё что с флагом от администратора",
        L"\033[36mCLOSE AFTER\033[0m -- закрывает программу auto после запуска. для групп закрывает только после запуска всей группы",
        L"\033[36mCHILDREN\033[0m -- дополнительные строки для запуска, появляются при наводке, могут быть откуда угодно и чем угодно. после выбора будет запущено их составление",
        L"\033[36mSEPARATE\033[0m -- заставляет программу стартовать отдельно(может пригодиться для discord/vortex)"
        };
    
    std::wstring force_childs_additional_prompt = L""; 

    for (size_t i = 0; i < flaggs.size(); ++i) {
        if (already_in_use.find(flaggs[i]) != std::wstring::npos) { //сломается, если будет примерно такой флаг: childs_pass или что-то содержащее другой внутри
            preselected.push_back(i);
        }
    }
    FlagsContents in_used_parsed = flags_parser(already_in_use);

    std::vector<int> new_flags = flags_to_vec(in_used_parsed, true);
    if (in_used_parsed.Children.empty()) {
        force_childs_additional_prompt = L"У вас нету дочерних, добавляйте или escape";
        std::erase(preselected, (int)FLAGS::Children - 1);
    }
    if (!force_children) {
        new_flags = advansed_chooser({
            .lines_to_choose = flagos_to_show,
            .singleChoice = false,
            .title = L"Выберите флаги: ",
            .preselected = preselected,
            });
    }
    //дальше код который надо будет оставить здесь в data_work, а тот что выше убрать в UI_iteractions или нет это же advansed_chooser который всего 1 вызов, хоть и занимает много
    wstring flags = L"";
    for (auto& flag_num : new_flags) {
         
        std::wstring separator = flags.empty() ? L"" : L":";
        std::wstring childs = L"";
        int action;
        bool end = false;
        std::vector<int>::iterator it;
        FlagsContents flags_inUse_parsed = flags_parser(already_in_use);
        switch (flag_num) {
        case (int)FLAGS::Asadmin:
            flags += separator + L"Asadmin"; break;
        case (int)FLAGS::CloseAfter:
            flags += separator + L"CloseAfter"; break;
        case (int)FLAGS::Children:
            it = std::find(preselected.begin(), preselected.end(), (int)FLAGS::Children-1);
            if (it != preselected.end()) { 
                action = advansed_chooser({ //TODO добавить показ в title
                    .lines_to_choose = {L"Изменить", L"Пересоздать"},
                    .singleChoice = true,
                    .title = L"У вас уже были дочерние, выберите действие:"
                    })[0];
                switch (action) {
                case -1://escape
                    end = true;
                    break;
                case 1://change
                    childs = change_childs(flags_inUse_parsed);
                    if (!childs.empty()) flags += separator + childs;
                    end = true;
                    break;
                case 2://remade
                    break;
                }
            }
            if (end) break;
            childs = add_childs(force_childs_additional_prompt);
            if (!childs.empty()) flags += separator + childs;
            break;
        case (int)FLAGS::Separate:
            flags += separator + L"Separate"; break;
        
        
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

//считает id для новой линии по FileType
int get_max_id(FileType type) {
    int max_id = 0;
    const auto& entries = global_all_lines[type];
    for (const auto& entry : entries) {
        if (entry.id > max_id) max_id = entry.id;
    }
    return max_id;
}

void load_all_files() {
    for (FileType type : {FileType::Game, FileType::Link, FileType::Program, FileType::Script, FileType::Group}) {
        global_all_lines[type] = file_parser(type);
    }
}

//из file_entry читает нужные линии по ID
//static int read_lines_by_IDs_fast(FileType           type,
//     const std::vector<int>&                   IDs,
//     std::wstring&                             showlines_num,
//     const std::unordered_map<int, LineEntry>& file_lookup,
//      std::vector<std::wstring>&               out_lines)
//{
//    out_lines.reserve(out_lines.size() + IDs.size());
//
//    for (const auto& id : IDs) {
//        // поиск id в хеш-таблице.
//        auto it = file_lookup.find(id);
//        if (it != file_lookup.end()) {
//            out_lines.push_back(get_name_accords_sett(it->second, showlines_num, type));
//        }
//    }
//}





std::map<int, std::vector<std::wstring>> prepare_children(
    FileType root_file_type,
    std::wstring& showlines_num)
{
    std::map<int, std::vector<std::wstring>> children;
    int line_counter = 1; //1-based


    

    // Проходим по ВСЕМ записям корневого типа (в порядке ID или как удобно)
    // Но нам нужен порядок, как в файле. Для этого нам нужен вектор.
    // Поэтому либо храним еще вектор, либо используем file_parser для порядка.
    // Проще: загружаем все записи в вектор для корневого типа.
    

    for (const auto& line_entry : global_all_lines[root_file_type]) {
        std::vector<std::wstring> line_children;

        if (!line_entry.flags.empty()) {
            FlagsContents flags_contents = flags_parser(line_entry.flags);
            for (const auto& [file_type, children_IDs] : flags_contents.Children) {
                
                for (int id : children_IDs) {
                    std::wstring display = L"";
                    display = get_name_accords_sett(
                        get_entry_by_id(file_type, id),
                        showlines_num,
                        file_type
                    );
                    line_children.push_back(display);
                    
                }
            }
        }

        if (!line_children.empty()) {
            children[line_counter] = std::move(line_children);
        }
        line_counter++;
    }

    return children;
}

