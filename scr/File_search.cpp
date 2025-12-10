#include <filesystem>
#include <iostream>
#include <windows.h>
#include <string>
#include <locale>
#include <vector>
#include <functional>
#include <map>
#include <unordered_set>
#include <future>
#include <atomic>
#include <queue>
#include <tuple>


namespace fs = std::filesystem;

static std::wstring extract_filename(const std::wstring& path) {
    const size_t last_slash = path.find_last_of(L"\\/");
    return (last_slash == std::wstring::npos) ? path : path.substr(last_slash + 1);
}

static std::tuple<bool, std::wstring> chooze_getter(std::vector<std::wstring> found_files, std::wstring res, std::wstring where_it_found) {
    std::wcout << L"Файлы найдены "<< where_it_found << std::endl;
    for (size_t i = 0; i < found_files.size(); ++i) {
        std::wcout << found_files[i] << std::endl;
    }
    std::wcout << L"Выберите если есть(нет-0): " << std::endl;
    std::wcin >> res;
    std::wcin.ignore();
    if (res != L"0") { return std::make_tuple(true,found_files[std::stoi(res) - 1]); }
    else { return std::make_tuple(false, res); }
}




static std::vector<std::wstring> get_disks_letter() {
    std::vector<std::wstring> disks;
    DWORD driveMask = GetLogicalDrives();
    if (driveMask == 0) {
        std::wcerr << L"Ошибка при получении списка дисков." << std::endl;
        return disks;
    }

    std::wcout << L"Список дисков и их имена:" << std::endl;

    for (int i = 0; i < 26; ++i) {
        if ((driveMask >> i) & 1) {
            wchar_t driveLetter = L'A' + i;
            std::wstring diskPath = std::wstring(1, driveLetter) + L":\\"; // Исправлено здесь
            std::wcout << diskPath << std::endl;
            disks.push_back(diskPath);
        }
    }
    return disks;
}

static std::vector<std::wstring> find_multiple_often_used_dirs(
    const std::wstring& disk_path,
    const std::vector<std::wstring>& dir_names,
    int max_depth = 3)
{
    std::vector<std::wstring> foundDirs;

    if (!fs::exists(disk_path)) {
        std::wcout << L"Диск " << disk_path << L" не существует или недоступен" << std::endl;
        return foundDirs;
    }

    // Инициализируем результат для всех искомых директорий

    foundDirs = std::vector<std::wstring>();


    // Преобразуем вектор в set для быстрого поиска
    std::unordered_set<std::wstring> target_dirs(dir_names.begin(), dir_names.end());

    // Вспомогательная функция для рекурсивного поиска
    std::function<void(const fs::path&, int)> searchDir;
    searchDir = [&](const fs::path& current_dir, int depth) {
        if (depth < 0) return;

        try {
            for (const auto& entry : fs::directory_iterator(current_dir)) {
                try {
                    if (entry.is_directory()) {
                        std::wstring dir_name = entry.path().filename().wstring();

                        // Проверяем, не является ли это одной из искомых папок
                        if (target_dirs.find(dir_name) != target_dirs.end()) {
                            std::wstring dir_path = entry.path().wstring();
                            foundDirs.push_back(dir_path);
                            std::wcout << L"Найдена папка " << dir_name << L": " << dir_path << std::endl;
                        }

                        // Рекурсивно ищем в подпапках
                        if (depth > 0) {
                            searchDir(entry.path(), depth - 1);
                        }
                    }
                }
                catch (const fs::filesystem_error&) {
                    continue;
                }
            }
        }
        catch (const fs::filesystem_error& e) {
            // Пропускаем директории с ошибками доступа
        }
        };

    // Начинаем поиск
    searchDir(fs::path(disk_path), max_depth);

    return foundDirs;
}




static std::vector<std::wstring> find_file_onDisk_with_exclude(const fs::path& directory,
    const std::wstring& target,
    const std::unordered_set<std::wstring>& exclude_dirs) {
    std::vector<std::wstring> found_files;

    try {
        for (auto it = fs::recursive_directory_iterator(
            directory,
            fs::directory_options::skip_permission_denied
        ); it != fs::recursive_directory_iterator(); ++it) {
            try {
                if (it->is_directory() &&
                    exclude_dirs.find(it->path().wstring()) != exclude_dirs.end()) {

                    // Вызываем метод на самом итераторе `it`
                    it.disable_recursion_pending();
                    continue;
                }

                if (it->is_regular_file() && it->path().filename() == target) {
                    found_files.push_back(it->path().wstring());
                }
            }
            catch (const fs::filesystem_error&) {
                continue;
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        std::wcout << L"Ошибка доступа к директории: " << e.what() << std::endl;
    }
    return found_files;
}

static std::vector<std::wstring> find_file_fast(const fs::path& directory,
    const std::wstring& target,
    int max_depth = 3) {
    std::vector<std::wstring> found_files;
    std::function<void(const fs::path&, int)> search;

    search = [&](const fs::path& current_dir, int depth) {
        if (depth > max_depth) return;

        try {
            for (const auto& entry : fs::directory_iterator(current_dir)) {
                try {
                    if (entry.is_regular_file() && entry.path().filename() == target) {
                        found_files.push_back(entry.path().wstring());
                    }
                    else if (entry.is_directory() && depth < max_depth) {
                        search(entry.path(), depth + 1);
                    }
                }
                catch (const fs::filesystem_error&) {
                    continue;
                }
            }
        }
        catch (const fs::filesystem_error&) {
            // Пропускаем ошибки доступа
        }
        };

    search(directory, 0);
    return found_files;
}

// Параллельный поиск по нескольким дискам
static std::vector<std::wstring> parallel_search(const std::vector<std::wstring>& disks,
    const std::wstring& target,
    const std::unordered_set<std::wstring>& exclude_dirs) {
    std::vector<std::future<std::vector<std::wstring>>> futures;
    std::vector<std::wstring> all_results;

    // Запускаем поиск на каждом диске в отдельном потоке
    for (const auto& disk : disks) {
        futures.push_back(std::async(std::launch::async, [&, disk]() {
            return find_file_fast(disk, target, 3);
            }));
    }

    // Собираем результаты
    for (auto& future : futures) {
        auto results = future.get();
        all_results.insert(all_results.end(), results.begin(), results.end());
    }

    return all_results;
}




std::wstring search(std::wstring path) {

    auto disks = get_disks_letter();
    std::vector<std::wstring> target_directories = {
        L"Games", L"Program Files", L"Program Files (x86)", L"Steam", L"SteamLibrary",
        L"GOG Games", L"Epic Games", L"Документы", L"Documents", L"Видео", L"Video",
        L"Downloads", L"Изображения", L"Pictures", L"Images", L"Desktop"
    };

    // Этап 1: Поиск в частых папках
    std::wcout << L"=== ПОИСК частых ПАПок ===" << std::endl;
    std::vector<std::wstring> all_found_dirs;
    std::unordered_set<std::wstring> exclude_dirs; // Для исключения при полном поиске

    for (const auto& disk : disks) {
        auto found_on_disk = find_multiple_often_used_dirs(disk, target_directories, 3);
        for (const auto& dir : found_on_disk) {
            all_found_dirs.push_back(dir);
            exclude_dirs.insert(dir); // Добавляем в исключения
        }
    }
    std::wcout << L"найдено: " << all_found_dirs.size() <<L" частых папок" << std::endl;

    std::wstring target = extract_filename(path);
    std::wcout << L"цель: " << target << std::endl;
    std::wstring res = L"0";
    static std::tuple<bool, std::wstring> response;
    short a = 0;
    std::vector<std::wstring> found_files;

    // Ищем в найденных частых папках
    for (size_t i = 0; i < all_found_dirs.size(); ++i) {
        auto files_in_dir = find_file_fast(all_found_dirs[i], target, 20);
        found_files.insert(found_files.end(), files_in_dir.begin(), files_in_dir.end());
        
    }
    if (!found_files.empty()) {
        response = chooze_getter(found_files, res, L"в частых папках!");
        if (std::get<0>(response) == true) { std::wcout << L"итог: " << std::get<1>(response) << std::endl; return std::get<1>(response); }
    }

    // Этап 2: Если не нашли в частых папках - быстрый поиск по дискам с исключениями
    if (found_files.empty()) {
        std::wcout << L"\nБЫСТРЫЙ ПОИСК ПО ДИСКАМ" << std::endl;

        for (const auto& disk : disks) {
            std::wcout << L"Диск " << disk << std::endl;
            found_files = find_file_onDisk_with_exclude(disk, target, exclude_dirs);
        }
        if (!found_files.empty()) {
            response = chooze_getter(found_files, res, L"на одном из дисков");
            if (std::get<0>(response) == true) { std::wcout << L"итог: " << std::get<1>(response) << std::endl; return std::get<1>(response); }
        }
    }

    // Этап 3: Если все еще не нашли - глубокий поиск
    if (found_files.empty()) {
        std::wcout << L"\n=== ГЛУБОКИЙ ПОИСК ПО ВСЕМ ДИСКАМ ===" << std::endl;
        found_files = parallel_search(disks, target, exclude_dirs);
        if (!found_files.empty()) {
            response = chooze_getter(found_files, res, L"на одном из дисков");
            if (std::get<0>(response) == true) { std::wcout << L"итог: " << std::get<1>(response) << std::endl; return std::get<1>(response); }
            else { std::wcout << L"\nпрости не сгодня" << std::endl; }
        }
    }
}

