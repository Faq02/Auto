#include <string>
#include <locale>
#include <vector>

#include "advanced_choice.h"



//int fqUI() {
//    while (true) {
//        float sec = 0;
//        int choice = advansed_chooser({
//            .lines_to_choose = { L"добавить Клик по координатам",
//            L"добавить Клик без координат(на предпологаемом расположении мышки)",
//            L"передвинуть мышку на координаты",
//            L"добавить комбинацию клавиш",
//            L"добавить ввод с клавиатуры",
//            L"нажать кнопку",
//            L"ЗАДЕРЖКА",
//            L"запуск чего-то(выполняется консолью)",
//            L"проверка",
//            L"Показать"},
//            .singleChoice = true,
//            .title = L"приветствую в создателе скриптов питона на основе библиотеки pyautogui!\nОЧЕНЬ ВАЖНО -- добавляй задержку! (можно не всегда, но надо думать, иначе программа(на которую скрипт) не успеет отреагировать)\n" })[0];
//        std::wstring lime = L"";
//        std::vector<std::wstring> hotkeys = {};
//        switch (choice) {
//        case 1:
//            if (python != nullptr) {
//                build.get_coords(*python);
//                build.add_click_with_coords();
//                break;
//            }
//            break;
//        case 2:
//            build.add_click_current_expected_pos();
//            break;
//        case 3:
//            build.move_mouse();
//            break;
//        case 4:
//            std::wcout << L"Напишите ваши клавиши через пробел(например: \"ctrl c\" или: \"alt tab\")\n";
//            hotkeys = make_massive_of_wstr();
//            build.add_hotkey(hotkeys);
//            break;
//        case 5:
//            std::wcout << L"Напиши то, что хочешь вводить:\n";
//            std::getline(std::wcin, lime);
//            build.write(lime);
//            break;
//        case 6:
//            std::wcout << L"Введи кнопку, которую хочешь нажимать в скрипте\n";
//            std::getline(std::wcin, lime);
//            build.press(lime);
//            break;
//        case 7:
//            sec = std::stof(input_word(L"в секундах (например: 0.2)"));
//            build.add_delay(sec);
//            break;
//        case 8:
//            build.choose_program_to_start(*python);
//            break;
//        case 9:
//            if (python != nullptr) {
//                build.check(*python);
//                break;
//            }
//            break;
//        case 10:
//            build.show_script();
//            break;
//        case 0:
//            if (from_changer) {
//                continue;
//                break;
//            }
//            build.save(prog_name);
//            end = true;
//            break;
//        case 11:
//            if (from_changer) {
//                continue;
//                break;
//            }
//            std::wcout << L"Сохраняем\n";
//            build.save(prog_name);
//            end = true;
//            break;
//        case -1:
//            if (!from_changer) {
//                if (count_escape < 2) { std::wcout << L"СОХРАНИТЬ? при последующем попадении сюда будет УДАЛЕНИЕ текущего скрипта\n\n"; }
//                count_escape++;
//                if (count_escape >= 2) {
//                    end = true;
//                    std::wcout << L"Я ПРЕДУПРЕЖДАЛ!\n\n";
//                    break;
//                }
//                countdown(5, L"до возвращения: ", 0);
//                break;
//            }
//            break;
//        }
//        if (true == end) {
//            break;
//        }
//        if (from_changer) {
//            line = build.end();
//            return line;
//            break;
//        }
//    }
//    return 0;
//}