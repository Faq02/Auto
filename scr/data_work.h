#pragma once
#include "app_config.h"

std::string getFileName(FileType type);
std::wstring read_url_from_url_file(const std::wstring& wpath);
int is_valid_file_type(const std::wstring& path);

struct Group {
    std::vector<FileType> types;
    std::vector<int> IDs; 
};
struct FlagsContents {
    std::wstring Asadmin;
    std::wstring CloseAfter;
    std::wstring Separate;
    std::map<FileType, std::vector<int>> Children; //first-file_type second-vector ID детей
};
struct LineDisplayInfo {
    std::wstring_view name_or_path;
    std::wstring_view flags;
    bool has_flags = false;
};
struct ReplaceOptions {
    FileType type;
    LineEntry& old_line;
    LineEntry& new_line_entry;
    bool full = true;
    bool path = false;
    bool name = false;
    bool flags = false;
};
int find_line_id(const std::wstring& raw_line);
LineEntry line_parser(FileType type, short line_number, std::wstring raw_line);
std::vector<LineEntry> file_parser(FileType type);
void add_entries(FileType type, std::vector<LineEntry>& line_entries);
void delete_entries_by_IDs(FileType type, std::vector<int> IDs);
void replace_entity(ReplaceOptions options);
void delete_entries_by_positions(FileType type, const std::vector<int>& positions);
Group group_parser(const std::wstring& line);
std::wstring_view get_name_accords_sett(const LineEntry* line_entry_Rptr, const std::wstring& sett, FileType type);
std::vector<std::wstring> showfile(FileType type, std::wstring& setting);
std::vector<std::wstring> show_groups(std::wstring setting);
std::vector<std::wstring> split(const std::wstring& line, wchar_t delim);
FlagsContents flags_parser(const std::wstring& flags);
void sort_flags(std::vector<std::wstring>& flags);
std::wstring choose_and_make_flags(std::wstring already_in_use = L"", bool force_choldren = false);
std::vector<std::wstring> translate_script_insides(std::vector<std::wstring>& not_translated_insides);
int get_max_id(FileType type);
void load_all_files();
const LineEntry* get_entry_by_id(FileType type, int id);
std::map<int, std::vector<std::wstring>> prepare_children(FileType root_file_type, std::wstring& showlines_num);