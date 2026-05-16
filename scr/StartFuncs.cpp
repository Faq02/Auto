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
#include "StartFuncs.h"
#include "File_search.h"
#include "settings.h"
#include "file_io.h"
#include "converter.h"
#include "Manager.h"
#include "data_work.h"
#include "path_handler.h"
#include "logger.h"
#include "ui_interactions.h"

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

//сортирует пути для запуска из групп
static std::vector<LineEntry> sort_entries(
    std::vector<LineEntry> entries)
{
    std::stable_partition(
        entries.begin(),
        entries.end(),
        [](auto& e) {
            return e.path.starts_with(L"http")
                || e.path.starts_with(L"https");
        });

    std::stable_partition(
        entries.begin(),
        entries.end(),
        [](auto& e) {
            return e.path.ends_with(L".py")
                || e.path.ends_with(L".bat")
                || e.path.ends_with(L".ps1");
        });

    return entries;
}

std::wstring del_close_after(std::wstring flags) {
    size_t clafter_ind = flags.find(L"CloseAfter");
    if (clafter_ind != std::wstring::npos) { 
        std::vector<std::wstring> flags_vec = split(flags,L':');
        flags = L"";
        bool was_closeafter_before = false;
        for (int i = 0; i < flags_vec.size();++i) {
            if (flags_vec[i] == L"CloseAfter") { was_closeafter_before = true; continue; }
            if (i != 0 and was_closeafter_before == false) { was_closeafter_before = false; flags = flags + L":" + flags_vec[i]; continue; }
            flags = flags + flags_vec[i];
        }
        return flags;
    }
    return flags; //включая двоеточее, разделяющее флаги
}


static HINSTANCE RunFile(std::wstring path, FileType type, int line_num = NULL, PythonRuntime* python = nullptr, std::string code_from_scr_make="") {
    // Проверяем расширение файла
    log(L"вход в RunFile\nпуть:" + path + 
        L"\nтип:" + StringToWstring(FILE_NAMES.at(type))+ 
        L"\nномер линии:" + std::to_wstring(line_num));
    log(L"  python: " + std::wstring(python ? L"OK\n" : L"NULL\n"));
    if (!code_from_scr_make.empty()) {log(L"  code: <present, size=" + std::to_wstring(code_from_scr_make.size()) + L">");}
    
    if (type == FileType::Script || !code_from_scr_make.empty()) {
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
            //std::wcout << L"с питоном то проблемки...\n"; Sleep(50000);
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
                    log(L"\033[32mСкрипт успешно запущен!\033[0m'\n");
                    SetCurrentDirectoryW(old_dir);
                    return (HINSTANCE)1;
                }              
                std::wstring wcode = get<std::wstring>(readFile({ .file_path = WstringTo_Utf8(path),.for_py_code = true, .isVector = false, }));
                std::string code = WstringTo_Utf8(wcode);
                std::wcout << L"Запускаем... " << path << std::endl;
                python->execute_safe(code);
                log(L"\033[32mСкрипт успешно запущен!\033[0m'\n");
                SetCurrentDirectoryW(old_dir);
                return (HINSTANCE)1;
            }
        }

    }
    
    //проверка существования+поиск если надо(по настройке(можно просто предложить переделать путь))
    if (!fs::exists(path) and type != FileType::Link) {
        std::wstring mode = prog_settings(false, 2);
        std::wstring foundPath;
        std::wstring pr_view = prog_settings(false, 1);
        if (mode == L"1") {
            foundPath = search(path);
        }
        else if (mode == L"2") {//ПОИТОГУ ЭТО НЕ РАБОТАЕТ СО СКРИПТАМИ
            std::wcout << L"Такого файла нету, согласно настройкам выберите(или введите) сами: " << std::endl;
            manager(1, type, pr_view, true, line_num, python); //возвращаемся для добавления в нужный файл
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



static bool RunScheduledTask() {
    // Формируем команду: schtasks /run /tn "ИмяЗадачи"
    
    std::wstring taskName = L"Asadmin_auto";
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
//логика запуска с флагами
static int start_with_flags(std::wstring path, int line_number, std::wstring& fl, FileType type,bool from_admin) {
    //парсим флаги, в вектор
    //пониая какой какой(они могут быть в любом порядке) запускаем в правильном порядке например: 1флаг-closeafter 2-asadmin естественно сначало должен выполниться asadmin иначе
    //всё сломается
    std::vector flags = split(fl, L':');
    sort_flags(flags);
    log(L"вошли в start_with_flags\n");
    log(L"путь:" + path + L"\nline_num:"+std::to_wstring(line_number) + (from_admin ? L"\nкак админ" : L"\nне как админ"));
    log(L"флаги:\n");
    for (std::wstring i : flags) {
        log(i+L"\n");
    }
    std::vector<std::wstring> asadminParams = {};
    asadminParams.push_back(std::to_wstring((int)type));
    asadminParams.push_back(std::to_wstring(line_number));
    bool has_started_path = false;
    for (std::wstring flag : flags) {
        if (flag == Flags::Asadmin) {
            from_admin ? log(L"from_admin\n") : log(L"not from_admin\n");
            if (from_admin) { RunFile(path, type, line_number, nullptr, ""); has_started_path = true; continue; }
            log(L"запуск с Asadmin\n");
            if (remove((FILE_NAMES.at(FileType::Asadmintmp).c_str())) == 0) { ; }
            else { log(L"Ошибка при удалении asadmintmp перед запуском"); } 
            writefile(asadminParams, FILE_NAMES.at(FileType::Asadmintmp), "", false); //создаём и записываем новый
            RunScheduledTask(); //запускаем задачу
        }
        if (flag == Flags::CloseAfter) {
            log(L"запуск с CloseAfter\n");
            if (has_started_path == false) { RunFile(path, type, line_number, nullptr, ""); std::exit(0); }
            std::exit(0);
        }
    }
    return 0;
}


static int groupStart(int group_num) {
    std::wstring gr_line = choose_line(group_num, FileType::Group, true);
    Group group = group_parser(gr_line);
    std::vector<std::wstring> paths, flags = {};
    bool indeed_closeAfter = false;
    group.entries = sort_entries(group.entries);
    for (const LineEntry& entry : group.entries) {
        paths.push_back(entry.path);
        if (indeed_closeAfter != true) {
            if (entry.flags.find(L"CloseAfter") != std::wstring::npos) {
                indeed_closeAfter = true;
            }
        }
        flags.push_back(entry.flags);
    }
    std::wstring line_flags = L"";
    if (std::filesystem::exists("Grouptmp.txt")) { remove("Grouptmp.txt"); }
    else { std::wcerr << L"Ошибка при удалении Grouptmp.txt возможно его не существует"; }
    for (int i = 0; i < paths.size(); ++i) {
        //временно
        line_flags = flags[i];
        if (!flags[i].empty()) {
            if (indeed_closeAfter == true) { line_flags = del_close_after(flags[i]); }//удаление close_after, если вообще был
            writefile(paths[i] + L"\"" + L"\"" + line_flags, FILE_NAMES.at(FileType::Grouptmp), "", false);
            continue;
            //startfiles(FileType::Grouptmp);
        }
        writefile(paths[i], FILE_NAMES.at(FileType::Grouptmp), "", false);
    }
    return 0;

}

int startfiles(FileType type, int line_number = NULL, PythonRuntime* python = nullptr, std::string codem = "", bool from_admin=false) {
    
    if (type == FileType::Grouptmp and line_number == 0) {
        for (int i = 0; i < std::get<std::vector<std::wstring>>(readFile({ .file_path = getFileName(FileType::Grouptmp), .for_full_read = true, .isVector = true })).size();i++) {
            log(L"запускаем "+ std::to_wstring(i+1) + L"линию из Grouptmp...\n");
            startfiles(FileType::Grouptmp, i + 1,python,"",from_admin);
            Sleep(1000);
        }
        return 0;
    }

    log(L"Начало запуска...(вход в startfiles)\n");
    //log(from_admin);
    if (codem != "") {
        //std::wcout << codem; Sleep(5000);
        RunFile(L"", FileType::null, NULL, python, codem);
        return 0;
    }
    LineEntry line_entry = line_parser(type, line_number, L"");
    std::wstring path = line_entry.path;
    std::wstring flags = line_entry.flags;
    log(L"путь:" + path+L"\nфлаги:"+flags);
    if (path.empty()) {
        std::wcout << L"Путь не найден!\n";
        return 1;
    }
    std::wstring exe_name = extract_filename(line_entry.path);
    if (!flags.empty() and type != FileType::Group) {
        log(L"есть флаги:" + flags + L"\nидем запускать");
        start_with_flags(path, line_number, flags, type,from_admin);
        return 0;
    }

    if (type == FileType::Group) {
        //size_t start = 0;
        //for (size_t i = 0; i <= path.length(); i++) {
        //    if (i == path.length() || path[i] == L'|') {
        //        std::wstring one_el = path.substr(start, i - start);
        //        if (one_el.empty()) {
        //            start = i + 1;
        //            continue;
        //        }

        //        // Проверяем начало строки
        //        if (one_el.length() >= 5 && one_el.substr(0, 5) != L"https") {
        //            one_el = where_are_you_go_lnk(one_el);
        //        }

        //        bool isRunning = false;
        //        if (one_el.length() >= 3 && one_el.substr(one_el.length() - 3) == L".py") {
        //            if (python != nullptr) {
        //                RunFile(one_el,FileType::null);
        //                start = i + 1;
        //                continue;
        //            }
        //            else {
        //                log(L"Ошибка: PythonRuntime недоступен для запуска скрипта\n");
        //                start = i + 1;
        //                continue;
        //            }
        //        }
        //        if (one_el.length() >= 4 && one_el.substr(one_el.length() - 4) == L".exe") {
        //            size_t lastSlash = one_el.find_last_of(L"\\/");
        //            std::wstring exeName = (lastSlash != std::wstring::npos) ?
        //                one_el.substr(lastSlash + 1) : one_el;
        //            isRunning = IsProcessRunning(exeName);
        //        }

        //        std::wcout << L"Запускаем... " << one_el << std::endl;

        //        if (isRunning) {
        //            size_t lastSlash = one_el.find_last_of(L"\\/");
        //            std::wstring exeName = (lastSlash != std::wstring::npos) ?
        //                one_el.substr(lastSlash + 1) : one_el;
        //            std::wcout << L"\033[31mПриложение\033[0m " << exeName
        //                << L" \033[31mуже запущено\033[0m \nЗапуск отменён\n" << std::endl;
        //            log(L"\033[31mПриложение\033[0m " + exeName
        //                + L" \033[31mуже запущено\033[0m \nЗапуск отменён\n");
        //            start = i + 1;
        //            continue;
        //        }
        //        HINSTANCE result;
        //        /*if (one_el.length() >= 3 && one_el.substr(one_el.length() - 3) == L".py" || isRunning) {
        //            ;
        //        }*/
        //       
        //        result = RunFile(one_el, FileType::null,NULL,python,"");
        //        if (reinterpret_cast<INT_PTR>(result) <= 32) {
        //            log(L"Файл не запущен, ошибка: " + shell_error_control(static_cast<int>(reinterpret_cast<INT_PTR>(result)))+L"\n");
        //        }
        //        else {
        //            std::wcout << L"\033[32mФайл успешно запущен!\033[0m" << std::endl;
        //            log(L"\033[32mФайл успешно запущен!\033[0m\n");
        //        }
        //        
        //        start = i + 1;
        //    }
        //}
        //return 0;
        groupStart(line_number);
        startfiles(FileType::Grouptmp);
        return 0;
    }

    /*if (file_name != "link" && GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {

        std::wcout << L"Файл не найден!\n";
        return 1;
    }*/
    HINSTANCE result = RunFile(path, type, line_number,python);
    if (reinterpret_cast<INT_PTR>(result) == 1) { return 0; }
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        std::wcout << colorfulPrint(L"Файл не запущен, ошибка:", PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED)

            << shell_error_control(static_cast<int>(reinterpret_cast<INT_PTR>(result))) << std::endl;
        log(L"Файл не запущен, ошибка: " + shell_error_control(static_cast<int>(reinterpret_cast<INT_PTR>(result))) + L"\n");
        return 1;
    }
    if (reinterpret_cast<INT_PTR>(result) == 33) {
        log(L"Файл уже запущен.\n");
        std::wcout << colorfulPrint(L"Файл уже запущен", PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::BLUE) << std::endl;
        return 0;
    }

    std::wcout << colorfulPrint(L"Файл успешно запущен", PRINT_TEXTCOLOR::GREEN) << std::endl;
    log(L"Файл успешно запущен!");
    return 0;
}

