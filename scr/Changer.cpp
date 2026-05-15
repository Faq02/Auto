#include <map>
#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>
#include <locale>
#include <codecvt>
#include <vector>
#include <filesystem>
#include <unordered_set>
#include "advanced_choice.h"

#include "python_script_maker.h"
#include "StartFuncs.h"
#include "settings.h"
#include "Groups.h"
#include "file_io.h"
#include "path_handler.h"
#include "converter.h"
#include "data_work.h"

using std::vector;
using std::wstring;
using std::string;
using std::get;

constexpr int Delete = 2;
constexpr int Replace = 1;
constexpr int Add = 3;
constexpr int BACK_TO_ACTION = 1001;
constexpr int BACK_TO_OBJECT = 1002;
constexpr int EXIT_SAVE = 1003;
constexpr int EXIT_NO_SAVE = 1004;
constexpr int CONTINUE = 1005;
constexpr int ESCAPE = -1;

template<typename T>
bool isOneOf(T value, std::initializer_list<T> options) {
	for (const auto& option : options) {
		if (value == option) {
			return true;
		}
	}
	return false;
}

template<typename Func>
vector<wstring> transform_massive(const vector<wstring>& massive,Func transform_func) {
	vector<wstring> new_massive;
	for (size_t i = 0; i < massive.size(); i++) {
		transform_func(i + 1, massive[i], new_massive);
	}
	return new_massive;
}

// Конкретные функции через лямбды:
vector<wstring> massive_replace_line(const vector<wstring>& massive,int line_num,const wstring& new_line) {
	return transform_massive(massive, [&](int current_line,const wstring& line,vector<wstring>& result) {
			if (current_line == line_num) {
				result.push_back(new_line);
			}
			else {
				result.push_back(line);
			}
		});
}

vector<wstring> add_to_massive(const vector<wstring>& massive,int line_num,const wstring& new_line) {
	return transform_massive(massive, [&](int current_line,const wstring& line,vector<wstring>& result) {
		result.push_back(line);
		if (current_line == line_num) {
				result.push_back(new_line);
			}
		});
}

vector<wstring> delete_lines_in_massive(const vector<wstring>& massive,const vector<int>& line_nums) {
	std::unordered_set<int> to_delete(line_nums.begin(), line_nums.end());
	return transform_massive(massive, [&](int current_line,
		const wstring& line,
		vector<wstring>& result) {
			if (to_delete.find(current_line) == to_delete.end()) {
				result.push_back(line);
			}
		});
}

class Changer {
private:
	vector<wstring> content; //пути в группах / строки кода в скриптах
	FileType mode;
	int object_num;
	int line_num;
	vector<wstring> translated_scrlines; //переведённые скрипты
	int action_type;
	vector<int> nums_to_delete;
	string name;

public:
	struct ModeConfig {
		std::string tempFile;
		std::string writeFile;
		std::wstring what_choose;
		vector<wstring> lines_to_choose;
	};
	ModeConfig config;
	void setconfig(FileType moderaw=FileType::null) {
		if (moderaw != FileType::null) { mode = moderaw; }
		if (mode == FileType::Script) { config = { "scrtemp.txt", "scrtmp", L"скрипт",translated_scrlines}; }
		else { config = { "grouptemp.txt", "1grouptemp.txt", L"группу",content}; }
	}

	int choose_object() {
		vector<wstring> lines_to_choose;
		if (mode == FileType::Group) lines_to_choose = show_groups(CURRENT_SETTINGS.showlines_num);
		else { lines_to_choose = showfile(FileType::Script, CURRENT_SETTINGS.showlines_num); }

		object_num = advansed_chooser({ .lines_to_choose = lines_to_choose, .singleChoice = true, .title = L"Выберите " + config.what_choose + L"\n" })[0];
		return object_num;
	}
	
	int get_object_contents() {
		if (mode == FileType::Script) {
			wstring path = choose_line(object_num, FileType::Script);
			std::wstring path_name = extract_filename(path);
			name = WstringTo_Utf8(path_name);
			name = "scripts\\" + name;
			content = get<std::vector<std::wstring>>(readFile({ .file_path = name, .isVector = true }));
			return 0;
		}
		wstring line = choose_line(object_num, FileType::Group,true);
		size_t start = 0;
		//получение линий группы - старый вариант без имён и флагов входящих в неё элементов
		for (size_t i = 0; i <= line.length(); i++) {
			if (i == line.length() || line[i] == L'|') {
				if (i > start) {  // Избегаем пустых строк
					content.push_back(line.substr(start, i - start));
				}
				start = i + 1;
			}
		}
	}

	void translate_contents_for_scripts() {
		//для скриптов нужен перевод на русский, для групп просто вывод:
		if (!translated_scrlines.empty()) { translated_scrlines.clear(); }
		translated_scrlines = translate_script_insides(content);
		
	}

	int show_and_choose_object_content() {
		setconfig(); //устанавливает lines_to_show в зависимости от режима
		config.lines_to_choose.push_back(L" ");
		config.lines_to_choose.push_back(L"Вернуться к выбору действия");
		config.lines_to_choose.push_back(L"Вернуться к выбору скрипта/группы");
		config.lines_to_choose.push_back(L"Закончить");
		if (action_type == Replace or action_type == Add) {
			line_num = advansed_chooser({
					.lines_to_choose = config.lines_to_choose,
					.singleChoice = true,
					.title = action_type == Replace ? L"Выберите строку для изменения\n" : L"Выберите строку после которой будет добавлена новая\n" })[0];
			if (line_num == ESCAPE) { return ESCAPE; }
			if (line_num == config.lines_to_choose.size()) { return EXIT_SAVE; }
			if (line_num == config.lines_to_choose.size() - 1) { return BACK_TO_OBJECT; }
			if (line_num == config.lines_to_choose.size() - 2) { return BACK_TO_ACTION; }
			return line_num;
		}
		nums_to_delete = advansed_chooser({
				.lines_to_choose = config.lines_to_choose,
				.singleChoice = false,
				.title = L"Выберите строки для удаления\n" });
		if (nums_to_delete.back() == ESCAPE) { nums_to_delete.pop_back(); return ESCAPE; }
		if (nums_to_delete.back() == config.lines_to_choose.size() - 2) { nums_to_delete.pop_back(); return BACK_TO_ACTION; }
		if (nums_to_delete.back() == config.lines_to_choose.size() - 1) { nums_to_delete.pop_back(); return BACK_TO_OBJECT; }
		if (nums_to_delete.back() == config.lines_to_choose.size()) { nums_to_delete.pop_back(); return EXIT_SAVE; }
		if (mode == FileType::Script) {
			for (int& x : nums_to_delete) {
				x += 4;
			}
		}
		return line_num;

	}
	int choose_object_action() {
		int action = advansed_chooser({
			.lines_to_choose = {L"Заменить", L"Удалить", L"Добавить", L"Выбрать объект", L"Закончить(сохранить и выйти)"},
			.singleChoice = true,
			.title = L"Выберите режим: \n" })[0];
		action_type = action;
		switch (action) {
		case 4:return BACK_TO_OBJECT;
		case 5:return EXIT_SAVE;
		}
		return action;
	}
	int action_handler(PythonRuntime* python) {
		//show_and_choose_object_content();
		std::wstring new_line;
		if (action_type == Replace) {
			if (mode == FileType::Script) {
				new_line = python_script_make("", true, python);
				new_line.pop_back(); //удаляем \n который там создаётся
				//delete_lines_or_insert_or_add_one("scrtmp", {}, true, new_line, line_num + 4, false, false);
				std::wcout << L"before mass changes: \n";
				for (wstring line : content) {
					std::wcout << line + L"\n";
				}
				system("pause");
				content = massive_replace_line(content, line_num+4, new_line); //сразу обновляем и меняем
				std::wcout << L"after: \n";
				for (wstring line : content) {
					std::wcout << line + L"\n";
				}
				system("pause");
				return 0;
			}
			new_line = group_add(CURRENT_SETTINGS.path_view_num, true);
			content = massive_replace_line(content, line_num, new_line);
			return 0;
		}
		if (action_type == Delete) {
			/*for(int li : nums_to_delete) {
				std::wcout << li <<L"\n";
			}
			system("pause");*/
			content = delete_lines_in_massive(content, nums_to_delete);
			return 0;
		}
		if (action_type == Add) {
			if (mode == FileType::Script) {
				new_line = python_script_make("", true, nullptr);
				new_line.pop_back();
				content = add_to_massive(content, line_num+4, new_line);
				return 0;
			}
			new_line = group_add(CURRENT_SETTINGS.path_view_num, true);
			if (!new_line.empty() && new_line.back() == L'|')
				new_line.pop_back();
			content = add_to_massive(content, line_num, new_line);
			return 0;
		}
	}
	void save_changes() {
		if (mode == FileType::Script) {
			if (std::filesystem::exists(name.substr(1))) {
				if (remove((name.substr(1)).c_str()) == 0) { ; }
				else {
					std::wcerr << L"Ошибка при удалении старого файла";
				}
			}
			writefile(content, name, "", false);
			return;
		}
		if (mode == FileType::Group) {
			std::wstring retern_line;
			for (size_t i = 0; i < content.size(); i++) {
				retern_line += content[i] +L'|';// group_add итак добавляет | в конец
			}
			//retern_line.pop_back();//удаление последнего | зачем если логика с ним...
			//std::wcout << object_num << L"\n";
			//system("pause");
			delete_lines_or_insert_or_add_one(FileType::Group, {}, true, retern_line, object_num, false, false);
			//std::wcout << object_num << L"\n";
			//system("pause");
		}
	}
	//void clear_lines() 
};
//template<typename T>
//bool isOneOf(T value, std::initializer_list<T> options) {
//	for (const auto& option : options) {
//		if (value == option) {
//			return true;
//		}
//	}
//	return false;
//}

static void change_lines(FileType type) {
	//добавить выбор: путь, флаг, имя
	read_set(); //если расчитать возможный путь сюда, возможно можно избавиться
	vector<LineEntry> file_entry = file_parser(type);
	vector<wstring> paths, names_accord_sett,flags;
	wstring shownamesSett = prog_settings(false, 3);
	bool no_names = shownamesSett == L"3";
	names_accord_sett = showfile(type, shownamesSett);
	if (no_names) names_accord_sett = showfile(type, L"1");

	for (const auto& entry : file_entry) {
		paths.push_back(entry.path);
		flags.push_back(entry.flags);
	}

	names_accord_sett = showfile(type, prog_settings(false, 3));
	vector<wstring> lines_to_choose;
	for (int i = 0; i < paths.size(); ++i) {
		lines_to_choose.push_back(L"Имя: " + names_accord_sett[i] + L"\n" + L"Флаги: " + flags[i] + L"\n" + L"Путь: " + paths[i]);
	}

	int line_num = advansed_chooser({
		.lines_to_choose = lines_to_choose,
		.singleChoice = true,
		.title = L"Выберите объект для изминения:\n\033[31mесли в настроках стоит:\"показывать пути\",то автоматом ставится:\"имена/.exe имя\"!\n\033[0m"})[0];
	if (line_num == ESCAPE) return;
	LineEntry line_entry = line_parser(type, line_num, L"");
	bool end = false;
	wstring new_name;
	wstring new_line;
	int action;
	wstring path_choose_view_set = prog_settings(false, 1);
	while (end != true) {
		action = advansed_chooser({
			.lines_to_choose = {L"Имя", L"Флаги", L"Путь", L"Закончить"},
			.singleChoice = true,
			.title = L"Выберите что менять:\nЕсли нет кастомного имени берётся СТАРЫЕ ФЛАГИ ЗАМЕНЯТСЯ .exe-имя\n" })[0];
		switch (action) {
		case 1:
			if (line_num == -1) { return; }
			new_name = input_line(L"Введите новое имя\nБыло: " + (line_entry.name.empty() ? L" - " : line_entry.name));
			new_line = line_entry.path + L"\"" + new_name + L"\"" + line_entry.flags;
			delete_lines_or_insert_or_add_one(type, {}, true, new_line, line_num, false, false);
			break;
		case 2:
			new_line = line_entry.path + L"\"" + line_entry.name + L"\"" + make_flags();
			delete_lines_or_insert_or_add_one(type, {}, true, new_line, line_num, false, false);
			break;
		case 3:
			new_line = choose_file_on_pc(path_choose_view_set,type) + L"\"" + line_entry.name + L"\"" + line_entry.flags;
			delete_lines_or_insert_or_add_one(type, {}, true, new_line, line_num, false, false);
			break;
		default: end = true; break; // что-то кроме 1 2 3 - выходим, хотя возможно надо было дать 2 шанс 
		}
	}



}


int changer(FileType mode, PythonRuntime* python = nullptr) {
	if (mode != FileType::Script and mode != FileType::Group) { change_lines(mode); return 0; }
	Changer editor;
	editor.setconfig(mode);
	bool end =false;
	bool changed = false;
	int action;
	while (true) {
		if (end) { break; }
		// 1. ВЫБОР ОБЪЕКТА
		if (editor.choose_object() == ESCAPE) return 0;

		// 2. ЗАГРУЗКА
		editor.get_object_contents();
		if (mode == FileType::Script) editor.translate_contents_for_scripts();

		// 3. ГЛАВНЫЙ ЦИКЛ РЕДАКТИРОВАНИЯ
		//ПРОБЛЕМА: обект не меняется.
		while (true) {
			// 3.1 Выбор действия
			if (!changed) { action = editor.choose_object_action(); }
			changed = false;

			if (action == BACK_TO_OBJECT or action == ESCAPE) {
				// Сохранить текущий? (спросить пользователя)
				//не чувак мы просто сохраняем не спрашивая
				editor.save_changes();
				// Выбрать новый объект
				break;
			}

			if (action == EXIT_SAVE) {
				editor.save_changes();
				end = true;
				break;
			}

			// 3.2 Выбор строки для действия
			int selection = editor.show_and_choose_object_content();

			if (selection == BACK_TO_ACTION or selection == ESCAPE) continue;
			if (selection == BACK_TO_OBJECT) {
				editor.save_changes();
				// Вернуться к выбору объекта
				break; // выйти из цикла редактирования, вернуться к пункту 1
			}
			if (selection == EXIT_SAVE) {
				editor.save_changes();
				end = true;
				return 0;
			}

			// 3.3 Выполнение действия
			editor.action_handler(python);
			changed = true;
			// 3.4 Обновить перевод (если скрипт)
			if (mode == FileType::Script) editor.translate_contents_for_scripts();
		}
	}
	return 0;
}

