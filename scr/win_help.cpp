#include <string>
#include <windows.h>
#include <taskschd.h>
#include <comdef.h>
#include <ShlObj.h>
#include <iostream>
#include "win_help.h"
#include <filesystem>
#include "file_io.h"


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
            std::wcout << L"Пользователь отказался от повышения прав." << std::endl;
        }
    }
    else {
        // Успешно запустили копию — закрываем текущий процесс немедленно
        //_exit(0);
    }
}

int makeTaskAdmin() {
    if (choose_line(5, FileType::Settings) == L"1") { return -1; }
    std::wstring path = std::filesystem::current_path();
    path += L"\\auto.exe";
    std::wstring name = L"Asadmin_auto";
    if (AmIAdmin()) { ; }
    else { make_task_with_elevate(); return 0; }
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) { std::wcerr << L"COM initialization failed.\n"; return 1; }

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

    // Создаем объект Task Scheduler
    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
        IID_ITaskService, (void**)&pService);
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
    delete_lines_or_insert_or_add_one(FileType::Settings, {}, true, L"1", 5, false, false);
    return 0;
}
