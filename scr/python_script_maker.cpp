#include <iostream>
#include <windows.h>
#include <string>
#include <locale>
#include <vector>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <conio.h> // для ожидания ввода буквы
#include <codecvt>
#include <fcntl.h>
#include <io.h>
#include <shellapi.h>
#include <sstream>
#include "File_funcs.h"
#include "advanced_choice.h"

class PythonScriptBuilder {
private:
    std::wstring code;
    std::wstring coords;

public:
    void read_coords() {
        std::wifstream fin("coords.txt");
        std::wstring line = L"";
        std::getline(fin, line);
        fin.close();
        coords = line;
    }
    void get_coords() {
        std::wcout << L"Наведите курсор на нужное место и нажмите E..." << std::endl;

        // Ожидание любой клавиши
        _getch();

        std::wstring path = L"coords_get.py";
        ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);

        Sleep(1000);
        read_coords();

        if (!coords.empty()) {
            std::wcout << L"Координаты получены: " << coords << std::endl;
        }
        else {
            std::wcout << L"Не удалось получить координаты!" << std::endl;
        }
    }
    void add_delay(float seconds) {
        code = code + L"time.sleep(" + std::to_wstring(seconds) + L")" + L"\n";
    }
    void add_click_with_coords() { //у click() оч. много параметров => ?
        /*if (!has_coords) {
            std::wcout << L"Сначала получите координаты!" << std::endl;
            return;
        }*/
        code += L"pyautogui.click(" + coords + L")\n";
    }
    void add_click_current_expected_pos() {
        code += L"pyautogui.click()\n"; // клик где сейчас курсор
    }
    void move_mouse() {
        code += L"pyautogui.moveTo(" + coords + L")\n";
    }
    void add_hotkey(std::vector<std::wstring> hotkeys) {
        std::wstring inScobe = L"";
        for (size_t i = 0; i < hotkeys.size(); ++i) {
            inScobe += L"'" + hotkeys[i] + L"'";
            if (i < hotkeys.size() - 1) {
                inScobe += L",";
            }
        }
        code += L"pyautogui.hotkey(" + inScobe + L")\n"; 
    }
    void write(std::wstring text) {
        code += L"keyboard.write('" + text + L"')\n";  
    }
    void press(std::wstring key) {
        code += L"pyautogui.press('" + key + L"')\n";  
    }
    void save(std::string prog_name) {
        writefile(code, "script", prog_name, false);
    }
    void standart_lines() {
        code += L"import pyautogui\nimport time\nimport keyboard\n\n";
    }
};



int python_script_make(std::string prog_name) {
    //std::wcout << L"приветствую в создателе скриптов питона на основе библиотеки pyautogui!\nОЧЕНЬ ВАЖНО -- добавляй задержку! (можно не всегда, но надо думать, иначе программа(на которую скрипт) не успеет отреагировать)\n";
    //std::wcout << L"ОЧЕНЬ ВАЖНО -- добавляй задержку! (можно не всегда, но надо думать, иначе программа(на которую скрипт) не успеет отреагировать)\n";
    PythonScriptBuilder build;
    build.standart_lines();
    bool end = false;
    int count_escape = 0;
    while (true) {
        float sec = 0;
        int choice = advansed_chooser({ 
            .lines_to_choose = { L"добавить Клик по координатам",L"добавить Клик без координат(на предпологаемом расположении мышки)",L"передвинуть мышку на координаты",L"добавить комбинацию клавиш",L"добавить ввод с клавиатуры",L"нажать кнопку",L"ЗАДЕРЖКА", L"или 0-сохранить(закончить)" },
            .singleChoice = true, 
            .title = L"приветствую в создателе скриптов питона на основе библиотеки pyautogui!\nОЧЕНЬ ВАЖНО -- добавляй задержку! (можно не всегда, но надо думать, иначе программа(на которую скрипт) не успеет отреагировать)\n" })[0];
        std::wstring lime = L"";
        std::vector<std::wstring> hotkeys = {};
        switch (choice) {
            case 1:
                build.get_coords();
                build.add_click_with_coords();
                break;
            case 2:
                build.add_click_current_expected_pos();
                break;
            case 3:
                build.get_coords();
                build.move_mouse();
                break;
            case 4:
                std::wcout << L"Напишите ваши клавиши через пробел(например: \"ctrl c\" или: \"alt tab\")\n";
                hotkeys = make_massive_of_wstr();
                build.add_hotkey(hotkeys);
                break;
            case 5:
                std::wcout << L"Напиши то, что хочешь вводить:\n";
                std::getline(std::wcin, lime);
                build.write(lime);
                break;
            case 6:
                std::wcout << L"Введи кнопку, которую хочешь нажимать в скрипте\n";
                std::getline(std::wcin, lime);
                build.press(lime);
                break;
            case 7:
                sec = std::stof(chooser(L"в секундах (например: 0.2)"));
                build.add_delay(sec);
                break;
            case 0:
                build.save(prog_name);
                end = true;
                break;
            case 8:
                build.save(prog_name);
                end = true;
                break;
            case -1:
                if (count_escape < 2) { std::wcout << L"СОХРАНИТЬ? при последующем попадении сюда будет УДАЛЕНИЕ текущего скрипта\n\n"; }
                count_escape++;
                if(count_escape >= 2) { 
                    end = true; 
                    std::wcout << L"Я ПРЕДУПРЕЖДАЛ!\n\n";
                    break;
                }
                countdown(5, L"до возвращения: ", 0);
                break;
        }
        if (true == end) {
            break;
        }
    }
    return 0;
}