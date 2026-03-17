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
#include <stdexcept>
#include <Python.h>
#include <cwctype>
#include <taskschd.h>
#include <comdef.h>
#include "File_funcs.h"
#include "StartFuncs.h"
#include "File_search.h"
#include "settings.h"

namespace fs = std::filesystem;

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

std::wstring where_are_you_go_lnk(const std::wstring& lnkPath) {
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


PythonRuntime::PythonRuntime() : m_initialized(false) {
    std::wstring pythonHome = L"./python_embed";
    Py_SetPythonHome(pythonHome.c_str());
    std::wstring pythonPath = pythonHome + L"/python310.zip;" +
        pythonHome + L"/DLLs;" +
        pythonHome + L"/lib;" +
        L"./python_libs";
    Py_SetPath(pythonPath.c_str());
    Py_Initialize();

    PyRun_SimpleString(
        "import sys\n"
        "sys.path.insert(0, './python_libs')\n"
    );

    m_initialized = true;
    std::wcout << L"[INFO] PythonRuntime создан.\n";
}

PythonRuntime::~PythonRuntime() {
    if (m_initialized) {
        Py_Finalize();
        std::wcout << L"[INFO] PythonRuntime уничтожен.\n";
    }
}

int PythonRuntime::execute(const std::string& code) {
    if (!m_initialized) return -1;
    return PyRun_SimpleString(code.c_str());
}

bool PythonRuntime::execute_safe(const std::string& code) {
    if (!m_initialized) {
        std::wcerr << L"[ERROR] Python не инициализирован!" << std::endl;
        return false;
    }

    if (code.empty()) {
        std::wcerr << L"[ERROR] Пустой код для выполнения!" << std::endl;
        return false;
    }

    std::wcout << L"[DEBUG] Выполнение Python кода (" << code.length() << L" байт)" << std::endl;

    std::string wrapped_code =
        "import sys\n"
        "import traceback\n"
        "try:\n"
        "    exec(r'''" + code + "''')\n"
        "    _success = True\n"
        "    print('\\033[32mВыполнение завершено успешно\\033[0m')\n"
        "except Exception as e:\n"
        "    print('\\033[31mPython Error:\\033[0m', e)\n"
        "    traceback.print_exc()\n"
        "    _success = False\n";

    int result = execute(wrapped_code);
    if (result != 0) {
        std::wcerr << L"[ERROR] Ошибка выполнения Python, код: " << result; Sleep(10000);
    }
    //Sleep(100000);
    return (result == 0);
}

static std::wstring GetCurrentWorkingDirectory() {
    wchar_t buffer[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, buffer);
    return std::wstring(buffer);
}

static bool SetCurrentWorkingDirectory(const std::wstring& dir) {
    return SetCurrentDirectoryW(dir.c_str()) != 0;
}

bool has_py_extension(const std::wstring& path) {
    if (path.length() < 3) return false;

    // Получаем последние 3 символа (нормализованные)
    std::wstring ext;
    for (auto it = path.rbegin(); it != path.rend() && ext.length() < 3; ++it) {
        wchar_t c = *it;
        if (c != L'\0' && c != L' ' && c != L'\r' && c != L'\n') {
            ext = std::towlower(c) + ext.c_str();
        }
    }

    return ext == L".py" || ext == L"yp."; // На случай обратного порядка
}



static HINSTANCE RunFile(std::wstring path, const std::string& file_type = "", int line_num = NULL, PythonRuntime* python = nullptr, std::string code_from_scr_make="") {
    // Проверяем расширение файла
    if (path.length() >= 4 || !code_from_scr_make.empty()) {
        std::wstring extension;
        std::wstring start;
        if (!path.empty()) {
            extension = path.substr(path.length() - 4);
            start = path.substr(0, 5);
        }
        if (start == L"https" || extension == L".url") {
            return ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
        if (python == nullptr) {
            std::wcout << L"с питоном то проблемки...\n"; Sleep(50000);
            return (HINSTANCE)1;
        }
        /*if (!code_from_scr_make.empty()) {
            std::wcout << L"не пусто1"; Sleep(5000);
        }
        else {
            std::wcout << L"пусто1"; Sleep(5000);
        }*/
        if ((path.ends_with(L".py")) or (!code_from_scr_make.empty())) {
            if (python != nullptr) {
                wchar_t app_path[MAX_PATH];
                GetModuleFileNameW(NULL, app_path, MAX_PATH);
                std::wstring app_dir = app_path;
                app_dir = app_dir.substr(0, app_dir.find_last_of(L"\\/"));

                wchar_t old_dir[MAX_PATH];
                GetCurrentDirectoryW(MAX_PATH, old_dir);
                SetCurrentDirectoryW(app_dir.c_str());

                // Выполняем скрипт
                if (code_from_scr_make != "") {
                    std::wcout << L"Запускаем... \n";
                    python->execute_safe(code_from_scr_make);
                    std::wcout << L"\033[32mСкрипт успешно запущен!\033[0m" << std::endl;
                    SetCurrentDirectoryW(old_dir);
                    return (HINSTANCE)1;
                }
                std::string spath = "1";
                spath += WstringTo_Utf8(path);
                std::wstring wcode = get<std::wstring>(readFile({ .file_type = spath,.for_py_code = true, .isVector = false, }));
                std::string code = WstringTo_Utf8(wcode);
                std::wcout << L"Запускаем... " << path << std::endl;
                python->execute_safe(code);
                std::wcout << L"\033[32mСкрипт успешно запущен!\033[0m" << std::endl;
                SetCurrentDirectoryW(old_dir);
                return (HINSTANCE)1;
            }
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
        else if (mode == L"2") {//ПОИТОГУ ЭТО НЕ РАБОТАЕТ СО СКРИПТАМИ
            std::wcout << L"Такого файла нету, согласно настройкам выберите(или введите) сами: " << std::endl;
            func_linker(1, file_type, pr_view, true, line_num, python);
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



static bool RunScheduledTask(const std::wstring& taskName) {
    // Формируем команду: schtasks /run /tn "ИмяЗадачи"
    // Обрати внимание на экранирование кавычек вокруг имени задачи.
    std::wstring command = L"schtasks /run /tn \"" + taskName + L"\"";

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    // Создаем процесс без создания окна (по желанию)
    if (CreateProcessW(
        NULL,               // Используем командную строку (следующий параметр)
        &command[0],        // Командная строка (модифицируемая)
        NULL,               // Атрибуты безопасности процесса
        NULL,               // Атрибуты безопасности потока
        FALSE,              // Наследование дескрипторов
        CREATE_NO_WINDOW,   // Флаг: не показывать окно консоли
        NULL,               // Переменные окружения (родительские)
        NULL,               // Рабочая директория (родительская)
        &si,
        &pi))
    {
        // Успешно запустили. Ждем завершения, если нужно.
        WaitForSingleObject(pi.hProcess, INFINITE); // Ждем, пока schtasks выполнится

        // Закрываем дескрипторы
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    else {
        std::wcerr << L"Failed to run task. Error: " << GetLastError() << std::endl;
        return false;
    }
}



int startfiles(int line_number = NULL, const std::string& file_type = "", PythonRuntime* python = nullptr, std::string codem = "") {
    std::wcout << L"Начало запуска...\n";
    if (codem != "") {
        //std::wcout << codem; Sleep(5000);
        RunFile(L"", "", NULL, python, codem);
        return 0;
    }
    LineEntry line_entry = line_parser(line_number, file_type, L"");
    std::wstring path = line_entry.path;
    std::wstring flags = line_entry.flags;
    if (path.empty()) {
        std::wcout << L"Путь не найден!\n";
        return 1;
    }
    std::wstring exe_name = extract_filename(line_entry.path);
    if (!flags.empty()) {
        if (flags.starts_with(L"Asadmin")) {
            if (flags.ends_with(exe_name) and (line_entry.name).empty())
                RunScheduledTask(exe_name);
            else {
                RunScheduledTask(line_entry.name);
            }
            return 0;
        }
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
                if (one_el.length() >= 3 && one_el.substr(one_el.length() - 3) == L".py") {
                    if (python != nullptr) {
                        RunFile(one_el);
                        start = i + 1;
                        continue;
                    }
                    else {
                        std::wcerr << L"Ошибка: PythonRuntime недоступен для запуска скрипта\n";
                        start = i + 1;
                        continue;
                    }
                }
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
                HINSTANCE result;
                /*if (one_el.length() >= 3 && one_el.substr(one_el.length() - 3) == L".py" || isRunning) {
                    ;
                }*/
               
                result = RunFile(one_el,"",NULL,python,"");
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
    HINSTANCE result = RunFile(path, file_type, line_number,python);
    if (reinterpret_cast<INT_PTR>(result) == 1) { return 0; }
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        std::wcout << L"Файл не запущен, ошибка: "
            << shell_error_control(static_cast<int>(reinterpret_cast<INT_PTR>(result))) << std::endl;
        return 1;
    }

    std::wcout << L"\033[32mФайл успешно запущен!\033[0m" << std::endl;
    return 0;
}
