#pragma once
#include <string>
#include "StartFuncs.h"
#include "app_config.h"
void manager(short mode, const FileType type, const std::wstring prog_view, bool from_start_files = false, short line_num = NULL, PythonRuntime* python = nullptr);