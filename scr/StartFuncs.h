#pragma once
#include "app_config.h"

int startfiles(FileType type, int line_number, std::string codem, bool from_admin);
int startfilesN(FileType type, const LineEntry& line_entry, const std::string& codem, bool from_admin);
std::wstring where_are_you_go_lnk(const std::wstring& lnkPath);
