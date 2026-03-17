#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>
#include <locale>
#include <codecvt>
#include <fcntl.h>
#include <io.h>
#include <map>
#include <functional>
#include <shellapi.h>
#include <limits>
#include <vector>
#include <sstream>
#include <ShlObj.h>
#include <comutil.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shlwapi.h>
#include <tuple>
#include <filesystem>
#include <cctype>
#include <variant>
#include <thread>
#include <iomanip>
#include <cwctype>
#include <string_view>
#include <taskschd.h>
#include <comdef.h>
#include <strsafe.h>
#include <objbase.h>
#include "StartFuncs.h"
#include "File_funcs.h"
#include "Groups.h"
#include "python_script_maker.h"
#include "advanced_choice.h"
#include "Changer.h"



#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsuppw.lib")


using std::string;
using std::wstring;
using std::wofstream;
using std::wifstream;
using std::vector;
namespace fs = std::filesystem;

constexpr wstring Console_standart = L"1";
constexpr wstring Win_path = L"2";

std::wstring chooser(const std::wstring& what_choice)
{
    std::wcout << what_choice << L'\n';
    wstring choice;
    //choice.reserve(4); -- было нужно для числовых значений, но есть использование как строки
    std::wcin >> choice;
    std::wcin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    return choice;
}


void countdown(double seconds_to_wait, std::wstring wstring_to_show, int duration) {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    std::wcout << std::fixed << std::setprecision(duration);
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - start;
        double remaining = seconds_to_wait - elapsed.count();

        if (remaining <= 0.0) {
            GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
            cursorInfo.bVisible = TRUE;
            SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
            break;
        }
        std::wcout << L"\r" << wstring_to_show << remaining;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}




std::wstring extract_filename(const std::wstring& path) {
    const size_t last_slash = path.find_last_of(L"\\/");
    return (last_slash == std::wstring::npos) ? path : path.substr(last_slash + 1);
}

std::vector<std::wstring> make_massive_of_wstr() {
    std::vector<std::wstring> wideStrings;
    std::wstring line;

    // Считываем всю строку до Enter
    if (std::getline(std::wcin, line)) {
        std::wistringstream iss(line);
        std::wstring tempStr;

        // Разбиваем строку на слова
        while (iss >> tempStr) {
            wideStrings.push_back(tempStr);
        }
    }
    return wideStrings;
}


static string file_chooser(string file_type, string prog_name="")
{
    string file_name = "progpaths.txt";
    if (file_type == "game")
    {
        file_name = "gamespaths.txt";
    }
    else if (file_type == "link")
    {
        file_name = "linkspath.txt";
    }
    else if (file_type == "group"){ file_name = "groups.txt"; }
    else if (file_type == "sett") { file_name = "settings.txt"; }
    else if (file_type == "script") 
    {
        file_name = "scripts\\script_" + prog_name + ".py";
    }
    else if (file_type == "scripts") { file_name = "scripts.txt"; }
    else if (file_type == "scrtmp") { file_name = "scrtemp.txt"; }
    return file_name;
}




std::tuple<bool, std::wstring> Win_path_selector()
{
    OPENFILENAME ofn;
    wchar_t filePath[MAX_PATH] = L"";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = L"*.exe *.bat *.cmd *.txt *.doc *.url *.py\0*.exe;*.bat;*.cmd;*.txt;*.doc;*.url;*.py\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"Что записать в файл?";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileName(&ofn) == TRUE) {
        wstring tmp = std::wstring(filePath);
        return std::make_tuple(true,tmp);
    }
    else { return std::make_tuple(false, L""); }
        // Пользователь отменил выбор или произошла ошибка
        //DWORD error = CommDlgExtendedError();
}

void If_prog_view_is_2_and_false()
{
    DWORD error = CommDlgExtendedError();
    if (error) {
        std::wcerr << L"Ошибка: " << error << L"\n";
    }
    else {
        std::wcout << L"Выбор файла отменен" << L"\n";
    }
}

wstring choose_file_on_pc(wstring& path_choose_view, short app_type) {
    if (path_choose_view == Win_path and app_type != 3) {
        std::tuple<bool, std::wstring> has_chousen;
        has_chousen = Win_path_selector();
        if (std::get<0>(has_chousen) == true) {
            return std::get<1>(has_chousen);
        }
        else {
            If_prog_view_is_2_and_false(); 
            return L"";
        }
    }
    if (path_choose_view == Console_standart) {
        wstring file_path;
        file_path = chooser(L"Введите путь: \n");
        return file_path;
    }
    return chooser(L"Введите ссылку: \n");
}


int writefile(std::vector<std::wstring> lines, string file_type, string prog_name = "", bool show = true) { //дозаписывает 

    std::string file_name;
    if (file_type[0] == '1') {

        file_name = file_type.substr(1);
    }
    else { file_name = file_chooser(file_type, prog_name); }
    wofstream fout(file_name, std::ios::app);
    fout.imbue(std::locale(fout.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    short i = 0;
    for (short i = 0; i < lines.size(); i++)
    {
        std::wstring line = lines[i];
        /*
        if (line.substr(0, 5) != L"https" && file_type != "group")
        {
            if (line.size() > 1) {
                std::filesystem::path p(line);
                std::wstring extension = p.extension().wstring();
            }
            std::wstring extension = line;
            if (extension != L".exe" && extension != L".txt" && extension != L".doc" && extension != L".url" && extension != L".bat" && extension != L".cmd" && extension != L".py" && extension != L"1" && extension != L"2" && extension != L"3" && extension != L"4" && extension != L"\n1" && extension != L"\n2" && extension != L"\n3" && extension != L"\n4") {
                    line += L".lnk";
            }
        }
        */
        fout << line << std::endl;
    }
    if(show) { std::wcout << L"Добавлено " << lines.size() << L" путей в файл" << std::endl; }
    fout.close();
    return 0;
}
int writefile(std::wstring line, std::string file_type, string prog_name = "", bool show=true) {
    return writefile(std::vector<std::wstring>{line}, file_type, prog_name, show);
}



std::variant<std::wstring, std::vector<std::wstring>> readFile(ReadOptions options) {
    std::string file_name;
    if (options.file_type[0] == '1') {
        file_name = options.file_type.substr(1);
    }
    else {
        file_name = file_chooser(options.file_type);
    }

    std::wifstream fin(file_name);
    fin.imbue(std::locale(fin.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    std::wstring line;

    
    size_t pos;
    if (options.isVector) {
        std::vector<std::wstring> lines;
        while (std::getline(fin, line)) {
            pos = line.find(L'\"');
            if (pos != std::wstring::npos and options.for_full_read != true) { lines.push_back(line.substr(0, pos)); }
            else { lines.push_back(line); }
        }
        fin.close();
        return lines;
    }
    else {
        short int line_cnt = 0;
        std::wstring content;
        while (std::getline(fin, line)) {
            line_cnt++;
            std::wstring res;
            if(options.for_py_code == false) { 
                res = std::to_wstring(line_cnt) + L" " + res; 
            }
            else {
                res = line; // для питона просто читаем код скрипта
            }

            content += res + L"\n";
        }
        fin.close();
        return content;
    }
}

wstring read_url_from_url_file(const wstring& wpath) {
    // Открываем файл .url для чтения
    string spath = WstringTo_Utf8(L"1" + wpath);
    auto lines = std::get<std::vector<std::wstring>>(readFile({ .file_type = spath,.for_py_code = false, .isVector = true }));

    wstring line;
    for (size_t i = 0; i < lines.size();++i) {
        // Ищем строку с URL=
        line = lines[i];
        if (line.find(L"URL=") == 0) {
            wstring url_wstr = line.substr(4);
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


void make_txt_for_scripts(std::string directory_path) {
    short c = 0;
    std::wstring file_contents;
    string file_name = file_chooser("scripts");
    if (remove(file_name.c_str()) == 0) { ; } //удаляем старый файл
    else { std::wcout << L"ошибка удаления файла с путями скриптов для перезаписи прости"; }

    for (const auto& entry : std::filesystem::directory_iterator(directory_path)) { //собираем названия скриптов
        if (entry.is_regular_file()) { //проверка на обычный файл, не директорию
            c++;
            std::wstring relativePath = L"scripts\\" + entry.path().filename().wstring(); //создание неполного пути
            std::wstring full_scrPath = std::filesystem::absolute(relativePath).wstring(); //создание полного пути по неполному
            file_contents += full_scrPath + L"\n";
        }
    }

    // Удаляем последний символ если строка не пустая
    if (!file_contents.empty()) {
        file_contents.pop_back();
    }

    writefile(file_contents, "scripts", "", false);
}



//автономно выбирает 1 линию(из txt файла) по номеру человеко подобному: 1 2 3, а не 0 1 2 (из за line_count++; перед проверкой а не после)
wstring choose_line(short line_number, string file_type) { //добавить определение из какого файла брать
    std::string file_name;
    if (file_type[0] == '1') {
        file_name = file_type.substr(1);
    }
    else { file_name = file_chooser(file_type); }
    wifstream fin;
    fin.open(file_name);
    fin.imbue(std::locale(fin.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    wstring line;
    wstring res_line;
    short int line_count = 0;
    int pos;
    while (std::getline(fin, line)) {
        line_count++;
        if (line_count == line_number) {
            pos = line.find(L'\"');
            if (pos != std::wstring::npos) { res_line = line.substr(0, pos); break; } //выбираем только
            res_line = line;
            break;
        }
    }
    fin.close();
    return res_line;
}

LineEntry line_parser(short line_number=NULL, std::string file_type="", wstring raw_line = L"") {
    LineEntry entry;
    //entry.path = choose_line(line_number, file_type)
    if (raw_line.empty()) {
        std::string file_name;
        if (file_type[0] == '1') {
            file_name = file_type.substr(1);
        }
        else { file_name = file_chooser(file_type); }
        wifstream fin;
        fin.open(file_name);
        fin.imbue(std::locale(fin.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
        wstring line;
        short int line_count = 0;
        while (std::getline(fin, line)) {
            line_count++;
            if (line_count == line_number) {
                raw_line = line;
                break;
            }
        }
        fin.close();
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

vector<LineEntry> file_parser(std::string file_type) {
    auto lines = get<std::vector<std::wstring>>(readFile({.file_type = file_type,.for_full_read = true, .for_py_code = false, .isVector = true, }));
    vector<LineEntry> result;

    for (auto& l : lines)
        result.push_back(line_parser(NULL,"",l));
    return result;
}
//возрождение функции из пепла...
vector<wstring> showfile(string file_type, wstring setting)
{
    vector<LineEntry> file_entry = file_parser(file_type);
    vector<wstring> result;

    bool gr_or_lnk = (file_type == "scrips" || file_type == "group");
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
                else{ display = entry.path; }
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

std::wstring StringToWstring(const std::string& str) {
    if (str.empty()) return std::wstring();

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr_to(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr_to[0], size_needed);
    return wstr_to;
}

bool AmIAdmin() { return IsUserAnAdmin() != FALSE; }

void ElevateNow(std::wstring name, std::wstring path) {


    wchar_t szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);

    std::wstring cmdLine = L"--create-task \"" + name + L"\" \"" + path + L"\"";


    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpVerb = L"runas";                       // Запрос UAC
    sei.lpFile = szPath;
    sei.lpParameters = cmdLine.c_str();
    sei.nShow = SW_SHOW;                          // Скрыть окно (можно SW_SHOWMINNOACTIVE)
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;


    if (!ShellExecuteEx(&sei)) {
        // Если пользователь нажал "Нет", ShellExecuteEx вернет FALSE.
        // Здесь ВАЖНО просто выйти, а не пытаться снова.
        DWORD dwError = GetLastError();
        if (dwError == ERROR_CANCELLED) {
            std::wcout << L"Пользователь отказался от повышения прав." << std::endl;
        }
    }
    else {
        // Успешно запустили копию — закрываем текущий процесс немедленно
        //_exit(0);
    }
}

int makeTaskAdmin(std::wstring name, std::wstring path) {
    if (AmIAdmin()) { ; }
    else { ElevateNow(name, path); return 0; }
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) { std::wcerr << L"COM initialization failed.\n"; return 1; }

    std::wcout << L"1\n";
    // Настройки безопасности COM
    //hr = CoInitializeSecurity(
    //    NULL,
    //    -1,
    //    NULL,
    //    NULL,
    //    RPC_C_AUTHN_LEVEL_PKT,      // Уровень аутентификации
    //    RPC_C_IMP_LEVEL_IMPERSONATE, // Уровень олицетворения
    //    NULL,
    //    EOAC_NONE,                   // Дополнительные флаги
    //    NULL);
    if (FAILED(hr)) {
        std::wcerr << L"CoInitializeSecurity failed. Error: 0x" << std::hex << hr << std::endl;
        CoUninitialize();
        return 1;
    }
    std::wcout << L"2\n";

    // Создаем объект Task Scheduler
    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
        IID_ITaskService, (void**)&pService);
    std::wcout << L"3\n";
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create TaskScheduler object. Error: " << std::hex << hr << std::endl; CoUninitialize(); Sleep(100000);
        return 1;
    }

    // Подключаемся к планировщику
    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        std::wcerr << L"Failed to connect to Task Scheduler. Error: " << std::hex << hr << std::endl; pService->Release(); CoUninitialize(); Sleep(100000);
        return 1;
    }
    std::wcout << L"4\n";
    // Получаем корневую папку задач
    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot get root folder. Error: " << std::hex << hr << std::endl; pService->Release(); CoUninitialize(); Sleep(100000);
        return 1;
    }

    // Создаем новое определение задачи
    ITaskDefinition* pTask = NULL;
    hr = pService->NewTask(0, &pTask);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create task definition. Error: " << std::hex << hr << std::endl; pRootFolder->Release(); pService->Release(); CoUninitialize(); Sleep(100000);
        return 1;
    }
    std::wcout << L"5\n";
    // --- НАСТРОЙКИ РЕГИСТРАЦИИ (Principal) ---
    IPrincipal* pPrincipal = NULL;
    hr = pTask->get_Principal(&pPrincipal);
    if (SUCCEEDED(hr)) {
        // Запускать с наивысшими привилегиями (UAC будет молчать)
        pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
        // Логин пользователя (интерактивный токен) - когда активен текущий пользователь
        pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
        pPrincipal->Release();
    }

    // --- НАСТРОЙКИ ЗАДАЧИ (Settings) ---
    ITaskSettings* pSettings = NULL;
    hr = pTask->get_Settings(&pSettings);
    if (SUCCEEDED(hr)) {
        pSettings->put_AllowDemandStart(VARIANT_TRUE);   // Можно запустить вручную
        pSettings->put_StartWhenAvailable(VARIANT_TRUE); // Запустить при возможности
        pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE); // Разрешить на батареях
        pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);    // Не останавливать на батареях
        pSettings->Release();
    }
    std::wcout << L"6\n";
    size_t lastSlash = path.find_last_of(L"\\/");
    std::wstring workingDir;
    if (std::string::npos != lastSlash) {
        workingDir = path.substr(0, lastSlash + 1);
    }
    // --- ДЕЙСТВИЕ (Action) ---
    IActionCollection* pActionCollect = NULL;
    hr = pTask->get_Actions(&pActionCollect);
    if (SUCCEEDED(hr)) {
        IAction* pAction = NULL;
        hr = pActionCollect->Create(TASK_ACTION_EXEC, &pAction);
        if (SUCCEEDED(hr)) {
            IExecAction* pExecAction = NULL;
            hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
            if (SUCCEEDED(hr)) {
                // Путь к твоему BAT-файлу или EXE
                pExecAction->put_Path(_bstr_t(path.c_str()));
                if (!workingDir.empty()) { pExecAction->put_WorkingDirectory(_bstr_t(workingDir.c_str())); }
                // Можно добавить аргументы
                // pExecAction->put_Arguments(_bstr_t(L"/your/args"));
                pExecAction->Release();
            }
            pAction->Release();
        }
        pActionCollect->Release();
    }
    std::wcout << L"7\n";
    //триггера нет-запуск будет командой: schtasks /run /tn "имя"

    //РЕГИСТРАЦИЯ ЗАДАЧИ
    IRegisteredTask* pRegisteredTask = NULL;
    _variant_t userId(L""); // Локальный пользователь

    hr = pRootFolder->RegisterTaskDefinition(
        _bstr_t(name.c_str()),        // Имя задачи
        pTask,
        TASK_CREATE_OR_UPDATE,            // Создать или обновить
        userId,                            // Владелец
        _variant_t(),                      // Пароль (не нужен для интерактивного)
        TASK_LOGON_INTERACTIVE_TOKEN,      // Тип логина
        _variant_t(L""),                   // Настройки (не обязательны)
        &pRegisteredTask);
    std::wcout << L"8\n";
    if (SUCCEEDED(hr)) {
        std::wcout << L"Task successfully registered.\n"; pRegisteredTask->Release();
        //pRegisteredTask->Run(_variant_t(), NULL);
    }
    else {
        std::wcerr << L"Failed to register task. Error: " << std::hex << hr << std::endl; Sleep(100000);
        if (hr == 0x80070005) { // E_ACCESSDENIED
            //заместо просьбы просто запрашивать повышение прав 

            std::wcerr << L"Access denied. Run this program as Administrator to create the task.\n";
        }
    }
    pTask->Release();
    pRootFolder->Release();
    pService->Release();
    CoUninitialize();
    std::wcout << L"9\n";
    return 0;
}




int delete_lines_or_insert_or_add_one(std::string file_type, std::vector<int> numbers = {}, bool insert = false, std::wstring line_to_insert_or_add = L"", short line_number = NULL, bool show = true, bool add = false)
{
    string file_name = file_chooser(file_type);
    wifstream fread;
    fread.open(file_name);
    fread.imbue(std::locale(fread.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001"))); //получает локаль файла и меняет на широкоформатную...
    if (fread.is_open() == false)
    {
        std::wcerr << "Ошибка открытия файла." << L"\n"; //добавить какой файл в ошибку
        return 1;
    }
    wofstream fwrit("temp.txt");
    fwrit.imbue(std::locale(fwrit.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
    if (fwrit.is_open() == false)
    {
        std::wcerr << "Ошибка открытия файла." << L"\n"; //тоже самое
        return 1;
    }
    short int lin_num = 1;
    wstring line;
    if (insert == false && add == false)
    {
        while (getline(fread, line)) {
            if (std::find(numbers.begin(), numbers.end(), lin_num) == numbers.end()) {
                fwrit << line << std::endl;
            }
            lin_num++;
        }
    }
    else
    {
        while (getline(fread, line))
        {
            if (lin_num == line_number)
            {
                if (add) {
                    fwrit << line <<L"\n" << line_to_insert_or_add;
                }
                else {
                    fwrit << line_to_insert_or_add << std::endl;
                }
                lin_num++;
                continue;
            }
            fwrit << line << std::endl;
            lin_num++;
        }
    }
    fread.close();
    fwrit.close();
    if (remove(file_name.c_str()) != 0) {
        std::wcerr << "ошибка удаления старого файла" << L"\n";
        return 1;
    }
    if (rename("temp.txt", file_name.c_str()) != 0) {
        std::wcerr << "ошибка переименовывания нового файла" << L"\n";
        return 1;
    }
    if (show == true) {
        std::wcout << L"Готово\nНовый файл:\n" << L"\n";
        std::wcout << std::get<std::wstring>(readFile({ .file_type = file_type,  .for_py_code = false, .isVector = false }));
    }
    return 0;
}
//можно вынести в файл changer изминение строки: пути, флагов, имени


void files_checker() {
    std::vector<std::string> files = { "progpaths.txt", "gamespaths.txt", "linkspath.txt", "groups.txt", "scripts.txt", "settings.txt"};
    wchar_t c;
    bool no_need_full_rewrite = false;
    //для txt файлов
    for (short i = 0; i < files.size(); ++i) {
        if (std::filesystem::exists(files[i]) && files[i] == "settings.txt") { //есть и имя настройки
            std::wifstream file(files[i]);
            file.imbue(std::locale(file.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
            while (file.get(c)) {

                if (!std::isspace(static_cast<unsigned char>(c))) { // Найден символ, который не является пробелом/enter-ом.
                    no_need_full_rewrite = true;
                    file.close();
                    break;
                }
            }
            file.close();
            if (no_need_full_rewrite == true) { //если не пустой проверяем не пустая ли 1 строка
                std::wifstream file(files[i]);
                file.imbue(std::locale(file.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
                wchar_t r;
                while (file.get(r)) {
                    if (std::isspace(static_cast<unsigned char>(r))) {
                        file.close();
                        delete_lines_or_insert_or_add_one("sett", {}, true, L"1", 1,false); //функция замены или удаления где true-замена int 1 - позиция строки 
                        break;
                    }
                    file.close(); //закрытие на случай не сработаного условия
                    break;
                }
            }
            if (no_need_full_rewrite == false) {
                if (remove(files[i].c_str()) == 0) { ; }
                else { std::wcout << L"ошибка удаления пустого файла настроек"; }
                wofstream fout(files[i], std::ios::app);
                fout.imbue(std::locale(fout.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
                fout << L"1" << std::endl;
                fout.close();
            }
        }


        if (!std::filesystem::exists(files[i])) { // нету
            std::ofstream(files[i].c_str()).close(); //создание пустого файла
            if (files[i] == "settings.txt") { //стандартная запись для настроек(жизненно важно)
                wofstream fout(files[i], std::ios::app);
                fout.imbue(std::locale(fout.getloc(), new std::codecvt_byname<wchar_t, char, mbstate_t>(".65001")));
                fout << L"1" << L"\n";
                fout.close();
            }
        }

    }
    std::wstring directory_path = L"scripts";
    if (!fs::exists(directory_path) or !fs::is_directory(directory_path)) {
        if (!CreateDirectory(directory_path.c_str(), NULL)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                std::cerr << "Ошибка при создании директории: " << GetLastError() << std::endl;
                return;
            }
        }
    }
    
}

void mass_files_delete(std::vector<int> num_of_indxes, string file_type) {
    std::wstring filename = L"";
    for (char i = 0; i < num_of_indxes.size(); i++) {
        filename = choose_line(num_of_indxes[i]+1, file_type);
        if (fs::remove(filename) != 0) {
            std::wcerr << L"Ошибка при удалении файла: " << filename << L"\n";
        }
    }
    //перезапись txt файла для скриптов
    if (file_type == "scripts") { make_txt_for_scripts("scripts\\"); }
}


bool isNotSpace(wchar_t c) {
    return !std::isspace(static_cast<wint_t>(c));
}

std::string coords_getter() {
    std::ifstream fin("coords.txt");
    std::string line = "";
    std::getline(fin, line);
    return line;
}


std::string WstringTo_Utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    // Получаем необходимый размер буфера
    int size_needed = WideCharToMultiByte(
        CP_UTF8,
        0,  // Флаги (0 по умолчанию)
        wstr.c_str(),
        static_cast<int>(wstr.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (size_needed <= 0) {
        // Ошибка при получении размера
        return std::string();
    }

    // Создаем строку нужного размера
    std::string result(size_needed, 0);

    // Выполняем преобразование
    int converted_length = WideCharToMultiByte(
        CP_UTF8,
        0,
        wstr.c_str(),
        static_cast<int>(wstr.size()),
        &result[0],
        size_needed,
        nullptr,
        nullptr
    );

    if (converted_length <= 0) {
        // Ошибка при преобразовании
        return std::string();
    }

    // Возвращаем результат
    return result;
}

void func_linker(short mode, const string& file_type, const wstring prog_view, bool from_start_files = false, short line_num = NULL, PythonRuntime* python = nullptr) {
    bool is_group_or_link = file_type == "group" or file_type == "link";
    /*
    if (prog_view != L"2" && from_start_files == false)
    {
        showfile(file_type); //вывод текущего файла в консоль, нужно из-за чего-то связаного с выбором файла через win_path_selector
    }
    */
    if (mode == 1) { //1-добавить
        if (file_type == "group")
        {
            group_add(prog_view);
            return;
        }
        if (file_type == "scripts") {
            std::wstring prog_name = chooser(L"Введите название скрипта");
            std::string pr_name = WstringTo_Utf8(prog_name);
            if (python != nullptr) {
                python_script_make(pr_name, false, python);
            }
            else {
                std::wcout << L"с питоном то проблемки...\n"; Sleep(50000);
                return;
            }
            return;
        }
        if ((prog_view == L"1" || file_type == "link") && (file_type != "group" || file_type != "scripts")) //если (вид записи в файл - консольный или тип файла - link) и тип файла != группе
        {
            /*
            реализовано добавление через пробел, но надо давать выбор - поэтому нужны ещё настройки(имеется ввиду добавлять через интер пока не 0)
            а также реализация первого через фун. massive maker
            */
            std::vector<std::wstring> lines;
            std::wstring lime;
            std::wcout << L"Что записать в файл?\n";
            std::getline(std::wcin, lime);
            auto it = std::find_if(lime.rbegin(), lime.rend(), isNotSpace); //узнаём 1 символ не пробел с конца строки
            lime.erase(it.base(), lime.end()); //режем по этот символ включ.

            int c = -1;
            int first = 0, second = 0;

            for (int i = 0; i < lime.size(); ++i) {
                c++;
                bool is_path_start = (std::isupper(lime[i]) && (i + 2 < lime.size()) && (lime.substr(i + 1, 2) == L":\\" || lime.substr(i + 1, 2) == L":/"));
                bool is_end = (c == lime.size() - 1);
                if ((is_path_start || is_end) && c != 0) {
                    first = second;
                    second = c;
                    if (is_end) {
                        second += 2;
                    }
                    if (first < second) {
                        lines.push_back(lime.substr(first, second - first - 1));
                    }
                }
            }


            if (from_start_files == true) {
                delete_lines_or_insert_or_add_one(file_type, {}, true, lime, line_num, false); //заменяем неправильную строку и запускаем правильную
                startfiles(line_num, file_type, nullptr, "");
            }
            else {
                writefile(lines, file_type); //стандартный вызов-просто добавление строки в конец
            }
        }
        else
        {
            std::tuple<bool, std::wstring> result = Win_path_selector();
            if (from_start_files == true) {
                if (std::get<0>(result) == true) {
                    delete_lines_or_insert_or_add_one(file_type, {}, true, std::get<1>(result), line_num, false);
                    startfiles(line_num, file_type, nullptr, "");
                }
                else { If_prog_view_is_2_and_false(); } // отмена поиска
            }
            else {
                if (std::get<0>(result) == true) { writefile(std::get<1>(result), file_type); return; }
                else { If_prog_view_is_2_and_false(); }
            }
        }
    }
    
    else if (mode == 2 and is_group_or_link == false) {
        func_linker(1, file_type, prog_view, false, 0, python); //добавляем объект в .txt или новый скрипт
        if (file_type == "scripts") { make_txt_for_scripts("scripts\\"); }
        vector<wstring> paths = get<vector<wstring>>(readFile({ .file_type = file_type,.for_full_read = false,.for_py_code = false,.isVector = true }));
        wstring last_path = paths[paths.size() - 1];
        wstring name = extract_filename(last_path);
        vector<wstring> flagos = { L"Asadmin" };
        wstring chousen_flag = flagos[advansed_chooser({ .lines_to_choose = flagos,.singleChoice = true,.title = L"Выбирай: \n" })[0] - 1];
        makeTaskAdmin(name, last_path);
        delete_lines_or_insert_or_add_one(file_type, {}, true, (last_path + L"\"" + name + L"\"" + (chousen_flag + name)), paths.size(), false, false);
        //готово
        
    }
    
    else if (mode == 3 or (mode == 2 and is_group_or_link == true)) { //удалить
        int choice = 0;
        if (file_type == "group")
        {
            choice = advansed_chooser({
                .lines_to_choose = { L"всю группу", L"её некоторые внутренности" },
                .singleChoice = true,
                .title = L"" })[0];

            switch (choice)
            {
            case 1: { break; }
            case 2: { group_del(); break; }
            }

        }
        if (file_type == "scripts")
        {
            std::vector<int> numbers = advansed_chooser({
                .lines_to_choose = std::get<std::vector<std::wstring>>(readFile({.file_type = file_type, .isVector = true})),
                .singleChoice = false,
                .title = L"Выберите номера удаляемых скриптов\n" });
            mass_files_delete(numbers, file_type);
            return;
        }
        if (file_type != "group" || choice == 1) //посути объединение и групп и негрупп... хороший ход
        {
            //std::vector<short> numbers = massive_maker(L"Выберите номера удаляемых строк"); //создание массива чисел для след строки
            std::vector<int> numbers = advansed_chooser({
                .lines_to_choose = std::get<std::vector<std::wstring>>(readFile({.file_type = file_type, .isVector = true })),
                .singleChoice = false,
                .title = L"Выберите номера удаляемых строк\n" });
            delete_lines_or_insert_or_add_one(file_type, numbers); //если надо несколько строк удалить -- удаляем несколько
        }

    }
    //переделать в changer!
    else if (mode == 4 or (mode == 3 and is_group_or_link == true)) {
        //выбор что менять - путь,имя,флаги 
        //пока кроме имени нету ничего
        //if (file_type == "group" or file_type == "scripts") {
        changer(file_type, python);
        return;


    }
}

