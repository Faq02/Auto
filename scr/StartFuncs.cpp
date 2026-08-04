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
#include "win_help.h"
#include "fq-start.h"

namespace fs = std::filesystem;

#pragma comment(lib, "Shlwapi.lib")
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





bool LaunchSeparate(const std::wstring& path) {
    

    fs::path exePath(path);
    std::wstring workDir = exePath.parent_path().wstring();
    
    

    // Копируем путь, так как CreateProcessW может изменять строку аргументов
    std::wstring cmd = L"\"" + path + L"\"";

    // Переводим рабочую директорию в понятный для WinAPI формат
    LPCWSTR lpCurrentDirectory = workDir.empty() ? NULL : workDir.c_str();


    HANDLE hNul = CreateFileW(
        L"NUL",
        GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hNul == INVALID_HANDLE_VALUE) {
        std::cerr << "Не удалось открыть NUL-устройство." << std::endl;
        return false;
    }

    SetHandleInformation(hNul, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);


    

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    si.hStdOutput = hNul; // Заглушка для стандартного вывода
    si.hStdError = hNul; // Заглушка для ошибок
    si.hStdInput = NULL;
    si.dwFlags |= STARTF_USESTDHANDLES; // Говорим системе использовать наши дескрипторы

    ZeroMemory(&pi, sizeof(pi));


    BOOL bSuccess = CreateProcessW(
        NULL,
        &cmd[0],
        NULL,
        NULL,
        TRUE, // ВАЖНО: Разрешаем наследовать дескриптор NUL
        CREATE_NO_WINDOW | DETACHED_PROCESS, // Комбинируем флаги скрытия
        NULL,
        workDir.empty() ? NULL : workDir.c_str(),
        &si,
        &pi
    );

    CloseHandle(hNul);
    if (!bSuccess) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        std::cerr << "Ошибка запуска. Код: " << GetLastError() << std::endl;
        return false;
    }

    // Закрываем хэндлы, чтобы не было утечки памяти (процесс останется жить)
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}




static HINSTANCE RunFile(std::wstring path, FileType type, std::string code_from_scr_make="") {
    // Проверяем расширение файла
    log(L"вход в RunFile\nпуть:" + path +
        L"\nтип:" + StringToWstring(FILE_NAMES.at(type)));
        

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
        
        /*if (!code_from_scr_make.empty()) {
            std::wcout << L"не пусто1"; Sleep(5000);
        }
        else {
            std::wcout << L"пусто1"; Sleep(5000);
        }*/
        

    }
    
    //проверка существования+поиск если надо(по настройке(можно просто предложить переделать путь))
    if (!fs::exists(path) and type != FileType::Link) {
        std::wstring mode = prog_settings(false, 2);
        std::wstring foundPath;
        std::wstring pr_view = prog_settings(false, 1);
        if (mode == L"1") {
            foundPath = search(path);
        }
        //else if (mode == L"2") {//ПОИТОГУ ЭТО НЕ РАБОТАЕТ СО СКРИПТАМИ и так-же с новой функций startfiles которая без line_num
        //    std::wcout << L"Такого файла нету, согласно настройкам выберите(или введите) сами: " << std::endl;
        //    manager(1, type, pr_view, true, line_num, python); //возвращаемся для добавления в нужный файл
        //    return (HINSTANCE)1;
        //}
    }

    if (path.length() >= 4 && path.substr(path.length() - 4) == L".exe") {
        size_t lastSlash = path.find_last_of(L"\\/");
        std::wstring exeName = (lastSlash != std::wstring::npos) ?
            path.substr(lastSlash + 1) : path;
        
        auto [running, pid] = IsProcessRunning(exeName);
        
        if (running) {
            if( ActivateProcessByPID(pid) == true) return(HINSTANCE)33;
            else { ; }
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
static int start_with_flags(std::wstring path,std::wstring raw_line, std::wstring& fl, FileType type, bool from_admin) {
    std::vector flags = split(fl, L':');
    sort_flags(flags);

    bool hasAsadmin = false;
    bool hasCloseAfter = false;
    bool hasSeparate = false;

    for (const auto& flag : flags) {
        if (flag == Flags::Asadmin) hasAsadmin = true;
        if (flag == Flags::CloseAfter) hasCloseAfter = true;
        if (flag == Flags::Separate) hasSeparate = true;
    }

    // ===== ЛОГИКА ЗАПУСКА =====
    bool program_started = false;

    // 1. Обработка Asadmin
    if (hasAsadmin) {
        if (from_admin) {
            // Уже с админскими правами — запускаем программу
            if (hasSeparate) LaunchSeparate(path);
            else RunFile(path, type, "");
            program_started = true;
        }
        else {
            // Нет прав — создаем задачу и выходим (без запуска!)
            log(L"Создаем задачу для запуска от администратора\n");
            std::vector<std::wstring> asadminParams = {
                std::to_wstring((int)type),
                raw_line
            };
            if (remove((FILE_NAMES.at(FileType::Asadmintmp).c_str())) == 0) { ; }
            else { log(L"Ошибка при удалении asadmintmp перед запуском"); }
            writefile(asadminParams, FILE_NAMES.at(FileType::Asadmintmp), "", false);
            RunScheduledTask();

            // Выходим из текущего процесса — программа запустится позже от админа
            if (hasCloseAfter) {
                // Если есть CloseAfter, но программа еще не запущена — просто выходим
                std::exit(0);
            }
            return 0;
        }
    }

    // 2. Обработка CloseAfter (если программа еще не запущена)
    if (hasCloseAfter && !program_started) {
        if (hasSeparate) LaunchSeparate(path);
        else RunFile(path, type, "");
        program_started = true;
        std::exit(0); // Закрываем программу после запуска
    }

    if (hasSeparate and !program_started) {
        LaunchSeparate(path);
        program_started = true;
        return 0;
    }

    // 3. Если нет флагов — просто запускаем
    if (!program_started) {
        RunFile(path, type, "");
    }

    return 0;
}


//static int groupStart(std::wstring& raw_line) {
//
//    Group group = group_parser(raw_line);
//    std::vector<std::wstring> paths, flags = {};
//    bool indeed_closeAfter = false;
//    group.entries = sort_entries(group.entries);
//    for (const LineEntry& entry : group.entries) {
//        paths.push_back(entry.path);
//        if (indeed_closeAfter != true) {
//            if (entry.flags.find(L"CloseAfter") != std::wstring::npos) {
//                indeed_closeAfter = true;
//            }
//        }
//        flags.push_back(entry.flags);
//    }
//    std::wstring line_flags = L"";
//    if (std::filesystem::exists("Grouptmp.txt")) { remove("Grouptmp.txt"); }
//    else { std::wcerr << L"Ошибка при удалении Grouptmp.txt возможно его не существует"; }
//    for (int i = 0; i < paths.size(); ++i) {
//        //временно
//        line_flags = flags[i];
//        if (!flags[i].empty()) {
//            if (indeed_closeAfter == true) { line_flags = del_close_after(flags[i]); }//удаление close_after, если вообще был
//            writefile(paths[i] + L"\"" + L"\"" + line_flags, FILE_NAMES.at(FileType::Grouptmp), "", false);
//            continue;
//            //startfiles(FileType::Grouptmp);
//        }
//        writefile(paths[i], FILE_NAMES.at(FileType::Grouptmp), "", false);
//    }
//    return 0;
//
//}


//весь код здесь-набор if-else привет yandere-dev
int startfilesN(FileType type, const LineEntry& line_entry,
    const std::string& codem, bool from_admin) {
    if (line_entry.id == -1) {
        return -1;
    }
    log(L"Начало запуска...(вход в startfiles)\n");
    if (type == FileType::Group) {
        Group group = group_parser(line_entry.path); //переделать по экономнее, можно использовать global...
        for (size_t i = 0; i < group.types.size();++i) {
            
            const LineEntry* entry = get_entry_by_id(group.types[i], group.IDs[i]);
            if(entry != nullptr) startfilesN(FileType::null, *entry, codem, from_admin);
            
        }
        return 0;
    }
    if (type == FileType::Script) {
        tokinizer(WstringTo_Utf8(line_entry.path));
        return 0;
    }
    //log(from_admin);
    if (codem != "") {
        //std::wcout << codem; Sleep(5000);
        RunFile(L"", FileType::null, codem);
        return 0;
    }
    std::wstring path = line_entry.path;
    std::wstring flags = line_entry.flags;
    std::wstring raw_line = line_entry.path + L"\"" + line_entry.name + L"\"" + line_entry.flags + L"*" + std::to_wstring(line_entry.id);
    log(L"путь:" + path + L"\nфлаги:" + flags);
    if (path.empty()) {
        std::wcout << L"Путь не найден!\n";
        return 1;
    }
    std::wstring exe_name; exe_name = extract_filename(line_entry.path);
    if (!flags.empty() and type != FileType::Group) {
        log(L"есть флаги:" + flags + L"\nидем запускать");
        start_with_flags(path,raw_line, flags, type, from_admin);
        return 0;
    }

    /*if (file_name != "link" && GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {

        std::wcout << L"Файл не найден!\n";
        return 1;
    }*/
    HINSTANCE result = RunFile(path, type);
    if (reinterpret_cast<INT_PTR>(result) == 1) { return 0; }
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        colorfulPrint(L"Файл не запущен, ошибка:" + shell_error_control(static_cast<int>(reinterpret_cast<INT_PTR>(result))), PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
        log(L"Файл не запущен, ошибка: " + shell_error_control(static_cast<int>(reinterpret_cast<INT_PTR>(result))) + L"\n");
        return 1;
    }
    if (reinterpret_cast<INT_PTR>(result) == 33) {
        log(L"Файл уже запущен.\n");
        colorfulPrint(L"Файл уже запущен", PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::BLUE);
        Sleep(1000);
        return 0;
    }

    colorfulPrint(L"Файл успешно запущен", PRINT_TEXTCOLOR::GREEN);
    log(L"Файл успешно запущен!");
    return 0;


}








int startfiles(FileType type, int line_number = NULL, std::string codem = "", bool from_admin=false) {
    

    LineEntry line_entry = line_parser(type, line_number, L"");

    // Вызываем новую версию с полученной структурой
    return startfilesN(type, line_entry, codem, from_admin);
}

