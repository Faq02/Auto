#pragma once
#pragma once
#include <string>
#include <map>
#include <vector>

std::wstring prog_settings(bool change, short num_to_read);
extern const std::map<std::wstring, std::wstring> PATH_CHOOSE_VIEW;
extern const std::map<std::wstring, std::wstring> IF_PATH_WRONG;
extern const std::map<std::wstring, std::wstring> SHOWLINES;

struct ProgSettings {
    std::wstring path_choose_view;
    std::wstring path_view_num;

    std::wstring if_path_wrong;
    std::wstring if_wrong_num;

    std::wstring showlines;
    std::wstring showlines_num;
};

extern ProgSettings CURRENT_SETTINGS;

void read_set();
