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


namespace fs = std::filesystem;

class FilesChecker {
private:
    bool need_rewrite = false;
public:

    //проверяет и меняет вектор настроек, если он не верный
    std::vector<std::wstring> check_settings(std::vector<std::wstring>& settings) {
        
        std::vector<std::wstring> valid_settings = {};
        std::wstring standart_setting_val = L"1";
        std::vector<uint64_t> elements_limits = { PATH_CHOOSE_VIEW.size(),IF_PATH_WRONG.size(),SHOWLINES.size(),1}; //исправить соеденив с settings.h и .cpp чтобы потом не бегать и искать какие пределы изменились
        const size_t expected_size = elements_limits.size();

        // Если настроек меньше, дополняем стандартными
        if (settings.size() < expected_size) {
            need_rewrite = true;
            settings.resize(expected_size, standart_setting_val);
        }
        for (int i = 0; i < settings.size(); ++i) {
            std::wstring setting = settings[i];
            if (setting.empty()) {
                need_rewrite = true;
                valid_settings.push_back(standart_setting_val); continue;
            }
            bool non_digit_found = false;
            for (wchar_t c : setting) {
                if (!iswdigit(c)) { 
                    need_rewrite = true;
                    non_digit_found = true;
                    valid_settings.push_back(standart_setting_val); break;
                }
            }
            if (non_digit_found == true) continue;
            //если здесь, то stoi безопасен(проверка выше)
            int int_sett = stoi(setting);
            if(!(int_sett > 0 and int_sett <= elements_limits[i])) { 
                need_rewrite = true;
                valid_settings.push_back(standart_setting_val); continue; 
            }
            valid_settings.push_back(setting);
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
                    writefile({L"1",L"1",L"1",L"1"}, getFileName(FileType::Settings), "", false);
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
