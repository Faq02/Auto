#include <windows.h>
#include <taskschd.h>
#include <comdef.h>
#include <ShlObj.h>
#include <TlHelp32.h>
#include <filesystem>

#include "file_io.h"
#include "win_help.h"
#include "logger.h"
#include "settings.h"
#include "ui_interactions.h"



#pragma comment(lib, "comsuppw.lib")



bool AmIAdmin() { return IsUserAnAdmin() != FALSE; }

static void make_task_with_elevate() {


    wchar_t szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);

    std::wstring cmdLine = L"--create-task";


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
            log(L"Пользователь отказался от повышения прав.\n");
        }
    }
    else {
        // Успешно запустили копию — закрываем текущий процесс немедленно
        //_exit(0);
    }
}

int makeTaskAdmin() {

    if (prog_settings(false,5) == L"1") { return -1; }
    std::wstring path = std::filesystem::current_path();
    path += L"\\auto.exe";
    std::wstring name = L"Asadmin_auto";
    if (AmIAdmin()) { ; }
    else { make_task_with_elevate(); return 0; }
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) { log(L"COM initialization failed.\n"); return 1; }

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
        log(L"CoInitializeSecurity failed. Error: 0x" + hr);
        CoUninitialize();
        return 1;
    }

    // Создаем объект Task Scheduler
    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
        IID_ITaskService, (void**)&pService);
    if (FAILED(hr)) {
        log(L"Failed to create TaskScheduler object. Error: " + hr);
        CoUninitialize();
        return 1;
    }

    // Подключаемся к планировщику
    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        log(L"Failed to connect to Task Scheduler. Error: " + hr); 
        pService->Release(); 
        CoUninitialize();
        return 1;
    }

    // Получаем корневую папку задач
    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) {
        log(L"Cannot get root folder. Error: " + hr);
        pService->Release();
        CoUninitialize(); 
        return 1;
    }

    // Создаем новое определение задачи
    ITaskDefinition* pTask = NULL;
    hr = pService->NewTask(0, &pTask);
    if (FAILED(hr)) {
        log(L"Failed to create task definition. Error: " + hr);
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return 1;
    }

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
                pExecAction->put_Arguments(_bstr_t(L"--Asadmin"));
                pExecAction->Release();
            }
            pAction->Release();
        }
        pActionCollect->Release();
    }
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
    if (SUCCEEDED(hr)) {
        log(L"Task successfully registered.\n"); pRegisteredTask->Release();
        //pRegisteredTask->Run(_variant_t(), NULL);
    }
    else {
        log(L"Failed to register task. Error: " + hr);
        if (hr == 0x80070005) { // E_ACCESSDENIED
            //заместо просьбы просто запрашивать повышение прав 

            log(L"Access denied. Run this program as Administrator to create the task.\n");
        }
    }
    pTask->Release();
    pRootFolder->Release();
    pService->Release();
    CoUninitialize();
    delete_lines_or_insert_or_add_one(FileType::Settings, {}, true, L"1", 5, false, false);
    return 0;
}


//возвращает bool работает ли процесс и и его PID
std::tuple<bool, DWORD> IsProcessRunning(const std::wstring& processName) {
    PROCESSENTRY32W entry = { sizeof(PROCESSENTRY32W) };
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    DWORD pid = 0;
    if (snapshot == INVALID_HANDLE_VALUE) return std::make_tuple(false, pid);

    bool exists = false;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, processName.c_str()) == 0) {
                exists = true;
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return std::make_tuple(exists, pid);
}


struct TargetProcess {
    DWORD pid;
    HWND hwnd;
};





static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    TargetProcess* target = reinterpret_cast<TargetProcess*>(lParam);
    DWORD windowPid;
    GetWindowThreadProcessId(hwnd, &windowPid);

    if (windowPid == target->pid && GetShellWindow() != hwnd) {
        HWND owner = GetWindow(hwnd, GW_OWNER);
        LONG style = GetWindowLong(hwnd, GWL_STYLE);
        bool isMainWin = (style & WS_CAPTION) == WS_CAPTION;

        // Теперь активируем ТОЛЬКО видимые главные окна
        if (owner == NULL && isMainWin && IsWindowVisible(hwnd)) {
            target->hwnd = hwnd;
            return FALSE;
        }
    }
    return TRUE;
}


bool ActivateProcessByPID(DWORD pid) {
    TargetProcess target = { pid, NULL };
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&target));
    if (target.hwnd != NULL) {
        // Проверяем состояние окна
        if (IsIconic(target.hwnd)) {
            ShowWindow(target.hwnd, SW_RESTORE);  
        }
        else {
            ShowWindow(target.hwnd, SW_SHOW);
        }
        SetForegroundWindow(target.hwnd);
        return true;
    }
    else {
        return false;
    }
}


void win_click_on_pos(int x, int y,bool Right) {
    SetCursorPos(x, y);

    // Симулируем нажатие левой кнопки мыши
    INPUT inputs[2] = {};

    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = Right ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_LEFTDOWN;

    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = Right ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_LEFTUP;

    SendInput(2, inputs, sizeof(INPUT));
}

static WORD GetVkCode(std::wstring key) {
    // Приводим к нижнему регистру для универсальности
    std::transform(key.begin(), key.end(), key.begin(), ::towlower);

    // Модификаторы и спецклавиши
    if (key == L"ctrl" || key == L"control") return VK_CONTROL;
    if (key == L"shift") return VK_SHIFT;
    if (key == L"alt") return VK_MENU;
    if (key == L"win" || key == L"windows") return VK_LWIN;
    if (key == L"enter" || key == L"return") return VK_RETURN;
    if (key == L"space") return VK_SPACE;
    if (key == L"tab") return VK_TAB;
    if (key == L"escape" || key == L"esc") return VK_ESCAPE;
    if (key == L"backspace") return VK_BACK;
    if (key == L"delete" || key == L"del") return VK_DELETE;

    // Функциональные клавиши (F1 - F12)
    if (key.length() > 1 && key[0] == L'f') {
        try {
            int fNum = std::stoi(key.substr(1));
            if (fNum >= 1 && fNum <= 12) return VK_F1 + (fNum - 1);
        }
        catch (...) {}
    }

    // Одиночные буквы и цифры (берём ASCII код заглавной буквы)
    if (key.length() == 1) {
        return (WORD)::towupper(key[0]);
    }

    return 0; // Неизвестная клавиша
}

// Универсальная функция выполнения комбинации
bool SendShortcut(const std::vector<std::wstring>& keys) {
    if (keys.empty()) return false;

    std::vector<WORD> vkCodes;
    for (const auto& key : keys) {
        WORD vk = GetVkCode(key);
        if (vk != 0) {
            vkCodes.push_back(vk);
        }

        else {
            colorfulPrint(L"LОшибка: Неизвестная клавиша '" + key+L'\'', PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
            return false;
        }
    }

    // Всего событий будет в 2 раза больше, чем клавиш (нажатие + отпускание)
    size_t totalEvents = vkCodes.size() * 2;
    std::vector<INPUT> inputs(totalEvents);
    ZeroMemory(inputs.data(), inputs.size() * sizeof(INPUT));

    size_t index = 0;

    // 1. Имитируем нажатие (KeyDown) всех клавиш по очереди
    for (WORD vk : vkCodes) {
        inputs[index].type = INPUT_KEYBOARD;
        inputs[index].ki.wVk = vk;
        index++;
    }

    // 2. Имитируем отпускание (KeyUp) в ОБРАТНОМ порядке
    for (auto it = vkCodes.rbegin(); it != vkCodes.rend(); ++it) {
        inputs[index].type = INPUT_KEYBOARD;
        inputs[index].ki.wVk = *it;
        inputs[index].ki.dwFlags = KEYEVENTF_KEYUP;
        index++;
    }

    // Отправляем всю цепочку за один системный вызов
    UINT sent = SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
    return sent == inputs.size();
}


bool SendText(const std::wstring& text) {
    if (text.empty()) return false;

    // Каждому символу нужно 2 события: нажатие и отпускание
    size_t totalEvents = text.length() * 2;
    std::vector<INPUT> inputs(totalEvents);
    ZeroMemory(inputs.data(), inputs.size() * sizeof(INPUT));

    size_t index = 0;

    for (wchar_t ch : text) {
        // 1. Нажатие символа (KeyDown)
        inputs[index].type = INPUT_KEYBOARD;
        inputs[index].ki.wVk = 0; // Для UNICODE физический код клавиши не нужен
        inputs[index].ki.wScan = ch; // Передаем сам символ в ScanCode
        inputs[index].ki.dwFlags = KEYEVENTF_UNICODE;
        index++;

        // 2. Отпускание символа (KeyUp)
        inputs[index].type = INPUT_KEYBOARD;
        inputs[index].ki.wVk = 0;
        inputs[index].ki.wScan = ch;
        inputs[index].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        index++;
    }

    // Отправляем весь текст в активное окно одним пакетом
    UINT sent = SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
    return sent == inputs.size();
}




//возвращает такое:L"1000,900"
std::wstring fq_maker::get_cursor_pos() {

    POINT pt;
    if (GetCursorPos(&pt)) return std::to_wstring(pt.x) + L',' + std::to_wstring(pt.y);
    else return L"";
}
