#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <windows.h>
#include <algorithm> 
#include <cwctype> 

#include "file_io.h"
#include "logger.h"
#include "settings.h"
#include "data_work.h"


namespace fs = std::filesystem;

class FilesChecker {
private:
    bool need_rewrite = false;

    // Константы для индексов (0-based)
    static constexpr size_t IDX_PATH_CHOOSE = 0;
    static constexpr size_t IDX_IF_PATH_WRONG = 1;
    static constexpr size_t IDX_SHOWLINES = 2;
    static constexpr size_t IDX_COLORS = 3;
    static constexpr size_t IDX_FLAG_TASK_ADMIN = 4;
    static constexpr size_t EXPECTED_SIZE = 5;

    const std::wstring DEFAULT_COLORS = L"8:7:7:16:8";
    const std::wstring DEFAULT_FLAG = L"0";

    // Проверка цветовой строки
    bool is_valid_color_setting(const std::wstring& str) {
        std::vector<std::wstring> parts = split(str, L':');
        if (parts.size() != 5) return false;

        // Допустимые значения для TextColor (0..15, но используются не все)
        auto is_valid_text_color = [](int v) -> bool {
            // Разрешённые: 0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15
            // (8 – Gray, но он уже есть; 1 – Gray; 0 – Black; и т.д.)
            // Проще проверить диапазон 0..15, т.к. все значения перечислены
            return v >= 0 && v <= 15;
            };
        auto is_valid_bg_color = [](int v) -> bool {
            // Проверяем, что значение состоит ТОЛЬКО из допустимых битов
            const int ALLOWED_BITS = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;
            // Проверяем, что:
            // 1) Нет лишних битов (v & ~ALLOWED_BITS == 0)
            // 2) Значение не равно 0 (Black разрешён)
            // 3) Или содержит хотя бы один базовый цвет или интенсивность
            return (v & ~ALLOWED_BITS) == 0;
            };

        try {
            int vals[5];
            for (int i = 0; i < 5; ++i) vals[i] = std::stoi(parts[i]);
            // Проверка по позициям:
            // 0: field_base_c   -> TextColor
            // 1: field_active_c -> TextColor
            // 2: lines_c        -> TextColor
            // 3: hover_c        -> BackgroundColor
            // 4: chosen_c       -> TextColor
            if (!is_valid_text_color(vals[0])) return false;
            if (!is_valid_text_color(vals[1])) return false;
            if (!is_valid_text_color(vals[2])) return false;
            if (!is_valid_bg_color(vals[3])) return false;
            if (!is_valid_text_color(vals[4])) return false;
            return true;
        }
        catch (...) {
            return false;
        }
    }

public:
    // Проверяет и возвращает исправленный вектор настроек
    std::vector<std::wstring> check_settings(std::vector<std::wstring>& settings) {
        std::vector<std::wstring> valid_settings;
        valid_settings.reserve(EXPECTED_SIZE);

        // Приводим к ожидаемому размеру
        if (settings.size() < EXPECTED_SIZE) {
            need_rewrite = true;
            settings.resize(EXPECTED_SIZE, L"1"); // временно заполняем
        }
        else if (settings.size() > EXPECTED_SIZE) {
            need_rewrite = true;
            settings.resize(EXPECTED_SIZE);
        }

        // Обрабатываем каждый элемент
        for (size_t i = 0; i < EXPECTED_SIZE; ++i) {
            std::wstring setting = settings[i];
            bool ok = false;

            if (i == IDX_COLORS) {
                // Проверка цвета
                if (is_valid_color_setting(setting)) {
                    ok = true;
                    valid_settings.push_back(setting);
                }
                else {
                    need_rewrite = true;
                    valid_settings.push_back(DEFAULT_COLORS);
                }
            }
            else if (i == IDX_FLAG_TASK_ADMIN) {
                // Флаг: должен быть "0" или "1"
                if (setting == L"0" || setting == L"1") {
                    ok = true;
                    valid_settings.push_back(setting);
                }
                else {
                    need_rewrite = true;
                    valid_settings.push_back(DEFAULT_FLAG);
                }
            }
            else {
                // Для первых трёх настроек: проверяем, что это число в допустимом диапазоне
                if (setting.empty()) {
                    need_rewrite = true;
                    valid_settings.push_back(L"1");
                    continue;
                }
                bool non_digit = false;
                for (wchar_t c : setting) {
                    if (!iswdigit(c)) { non_digit = true; break; }
                }
                if (non_digit) {
                    need_rewrite = true;
                    valid_settings.push_back(L"1");
                    continue;
                }
                int val = std::stoi(setting);
                // Определяем лимит в зависимости от индекса
                size_t limit = 0;
                if (i == IDX_PATH_CHOOSE) limit = PATH_CHOOSE_VIEW.size();
                else if (i == IDX_IF_PATH_WRONG) limit = IF_PATH_WRONG.size();
                else if (i == IDX_SHOWLINES) limit = SHOWLINES.size();
                if (val >= 1 && val <= static_cast<int>(limit)) {
                    valid_settings.push_back(setting);
                }
                else {
                    need_rewrite = true;
                    valid_settings.push_back(L"1");
                }
            }
        }

        return valid_settings;
    }

	void main() {
        std::vector<std::string> files = { "progpaths.txt", "gamespaths.txt", "linkspath.txt", "groups.txt", "scripts.txt", "settings.txt" };
        wchar_t c;
        bool no_need_full_rewrite = false;
        //для txt файлов
        for (short i = 0; i < files.size(); ++i) {
            if (std::filesystem::exists(files[i]) && files[i] == "settings.txt") { //есть и имя настройки
                std::vector<std::wstring> settings = std::get<std::vector<std::wstring>>(readFile({ .file_path = getFileName(FileType::Settings),.isVector = true }));
                settings = check_settings(settings);
                //system("pause");
                if (need_rewrite == true) {
                    if (!remove(getFileName(FileType::Settings).c_str())) {
                        log(L"ошибка при проверке файла settings а именно удаления\n");
                    }
                    writefile(settings, getFileName(FileType::Settings), "", false);
                }
            }


            if (!std::filesystem::exists(files[i])) { // нету
                std::ofstream(files[i].c_str()).close(); //создание пустого файла
                if (files[i] == "settings.txt") { //стандартная запись для настроек(жизненно важно, иначе краш)
                    writefile({ L"1", L"1", L"1", L"8:7:7:16:8", L"0" }, getFileName(FileType::Settings), "", false);
                }
            }

        }
        std::wstring directory_path = L"scripts";
        if (!fs::exists(directory_path) or !fs::is_directory(directory_path)) {
            if (!CreateDirectory(directory_path.c_str(), NULL)) {
                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    log(L"Ошибка при создании директории: " + GetLastError() + L'\n');
                    return;
                }
            }
        }
	}
};



void files_checker() {
    FilesChecker checker;
    checker.main();
    return;
}
