#include <iostream>
#include <windows.h>
#include <string>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <tlhelp32.h>
#include <cstdlib>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <cstdint>
#include "File_funcs.h"
#include "StartFuncs.h"
#include "File_search.h"

namespace fs = std::filesystem;

static std::wstring where_are_you_go_lnk(const std::wstring& lnkPath) {
    if (!PathFileExistsW(lnkPath.c_str()) || !PathMatchSpecW(lnkPath.c_str(), L"*.lnk")) {
        return lnkPath;
    }

    CoInitialize(NULL);
    IShellLinkW* pShellLink = NULL;
    IPersistFile* pPersistFile = NULL;
    std::wstring resolvedPath = lnkPath;

    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
        IID_IShellLinkW, (LPVOID*)&pShellLink);

    if (SUCCEEDED(hr)) {
        hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
        if (SUCCEEDED(hr)) {
            hr = pPersistFile->Load(lnkPath.c_str(), STGM_READ);
            if (SUCCEEDED(hr)) {
                wchar_t tempPath[MAX_PATH];
                hr = pShellLink->GetPath(tempPath, MAX_PATH, NULL, SLGP_SHORTPATH);
                if (SUCCEEDED(hr)) {
                    resolvedPath = tempPath;
                }
            }
            pPersistFile->Release();
        }
        pShellLink->Release();
    }
    CoUninitialize();
    return resolvedPath;
}

static std::wstring shell_error_control(int cod_error) {
    switch (cod_error) {
    case 2:  return L"Файл не найден";
    case 3:  return L"Путь не найден";
    case 5:  return L"Доступ запрещен";
    case 8:  return L"Недостаточно памяти";
    case 11: return L"Неправильный .exe";
    case 33: return L"Приложение уже запущено";
    default: return L"Неизвестная ошибка: " + std::to_wstring(cod_error);
    }
}

static bool IsProcessRunning(const std::wstring& processName) {
    PROCESSENTRY32W entry = { sizeof(PROCESSENTRY32W) };
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (snapshot == INVALID_HANDLE_VALUE) return false;

    bool exists = false;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, processName.c_str()) == 0) {
                exists = true;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return exists;
}

static HINSTANCE RunFile(std::wstring path, const std::string& file_type = "", int line_num = NULL) {
    // Проверяем расширение файла
    if (path.length() >= 4) {
        std::wstring extension = path.substr(path.length() - 4);
        std::wstring start = path.substr(0, 5);

        if (start == L"https" || extension == L".py" || extension == L".url") {
            return ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
    }
    //проверка существования+поиск если надо(по настройке(можно просто предложить переделать путь))
    if (!fs::exists(path)) {
        std::wstring mode = prog_settings(false, 2);
        std::wstring foundPath;
        std::wstring pr_view = prog_settings(false, 1);
        if (mode == L"1") {
            foundPath = search(path);
        }
        else if (mode == L"2") {
            std::wcout << L"Такого файла нету, согласно настройкам выберите(или введите) сами: " << std::endl;
            func_linker(1, file_type, pr_view, true, line_num);
            return (HINSTANCE)1;
        }
    }

    if (path.length() >= 4 && path.substr(path.length() - 4) == L".exe") {
        size_t lastSlash = path.find_last_of(L"\\/");
        std::wstring exeName = (lastSlash != std::wstring::npos) ?
            path.substr(lastSlash + 1) : path;
        if (IsProcessRunning(exeName)) {
            return (HINSTANCE)33; // ALREADY_RUNNING
        }
    }

    size_t lastSlash = path.find_last_of(L"\\/");
    std::wstring work_dir = (lastSlash != std::wstring::npos) ?
        path.substr(0, lastSlash) : L"";

    return ShellExecuteW(NULL, L"open", path.c_str(), NULL,
        work_dir.empty() ? NULL : work_dir.c_str(), SW_SHOWNORMAL);
}

int startfiles(int line_number, const std::string& file_type) {
    std::wcout << L"Начало запуска..." << std::endl;

    std::wstring path = choose_line(line_number, file_type);
    if (path.empty()) {
        std::wcout << L"Путь не найден!" << std::endl;
        return 1;
    }

    if (file_type == "group") {
        size_t start = 0;
        for (size_t i = 0; i <= path.length(); i++) {
            if (i == path.length() || path[i] == L'|') {
                std::wstring one_el = path.substr(start, i - start);
                if (one_el.empty()) {
                    start = i + 1;
                    continue;
                }

                // Проверяем начало строки
                if (one_el.length() >= 5 && one_el.substr(0, 5) != L"https") {
                    one_el = where_are_you_go_lnk(one_el);
                }

                bool isRunning = false;
                if (one_el.length() >= 4 && one_el.substr(one_el.length() - 4) == L".exe") {
                    size_t lastSlash = one_el.find_last_of(L"\\/");
                    std::wstring exeName = (lastSlash != std::wstring::npos) ?
                        one_el.substr(lastSlash + 1) : one_el;
                    isRunning = IsProcessRunning(exeName);
                }

                std::wcout << L"Запускаем... " << one_el << std::endl;

                if (isRunning) {
                    size_t lastSlash = one_el.find_last_of(L"\\/");
                    std::wstring exeName = (lastSlash != std::wstring::npos) ?
                        one_el.substr(lastSlash + 1) : one_el;
                    std::wcout << L"\033[31mПриложение\033[0m " << exeName
                        << L" \033[31mуже запущено\033[0m \nЗапуск отменён\n" << std::endl;
                    start = i + 1;
                    continue;
                }

                HINSTANCE result = RunFile(one_el);
                if (reinterpret_cast<INT_PTR>(result) <= 32) {
                    std::wcout << L"Файл не запущен, ошибка: \n"
                        << shell_error_control(static_cast<int>(reinterpret_cast<INT_PTR>(result))) << std::endl;
                }
                else {
                    std::wcout << L"\033[32mФайл успешно запущен!\033[0m" << std::endl;
                }

                start = i + 1;
            }
        }
        return 0;
    }

    /*if (file_type != "link" && GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        
        std::wcout << L"Файл не найден!\n";
        return 1;
    }*/    
    HINSTANCE result = RunFile(path, file_type, line_number);
    if (reinterpret_cast<INT_PTR>(result) == 1) { return 0; }
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        std::wcout << L"Файл не запущен, ошибка: "
            << shell_error_control(static_cast<int>(reinterpret_cast<INT_PTR>(result))) << std::endl;
        return 1;
    }

    std::wcout << L"\033[32mФайл успешно запущен!\033[0m" << std::endl;
    return 0;
}