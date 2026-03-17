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
#include "File_funcs.h"
#include "python_script_maker.h"
#include "StartFuncs.h"
#include "settings.h"
#include "Groups.h"

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
std::map<wstring, wstring> sorted_dict = {
	{L"pyautogui.click", L"клик по координатам: " },
	{L"time.sleep", L"пауза на: " },
	{L"pyautogui.hotkey", L"нажатие горячей клавиши: " },
	{L"keyboard.write", L"ввод с клавиатуры: " },
	{L"pyautogui.press", L"нажатие клавиши: "},
	{L"pyautogui.moveTo", L"передвинуть мышку на координаты" }
		};

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
	string mode;
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
	void setconfig(string moderaw="") {
		if (!moderaw.empty()) { mode = moderaw; }
		if (mode == "scripts") { config = { "scrtemp.txt", "scrtmp", L"скрипт",translated_scrlines}; }
		else { config = { "grouptemp.txt", "1grouptemp.txt", L"группу",content}; }
	}

	int choose_object() {
		object_num = advansed_chooser({
			.lines_to_choose = get<std::vector<std::wstring>>(readFile({.file_type = mode, .isVector = true})),
			.singleChoice = true,
			.title = L"Выберите " + config.what_choose + L"\n" })[0];
		return object_num;
	}
	
	int get_object_contents() {
		if (mode == "scripts") {
			wstring path = choose_line(object_num, "scripts");
			std::wstring path_name = extract_filename(path);
			name = WstringTo_Utf8(path_name);
			name = "1scripts\\" + name;
			content = get<std::vector<std::wstring>>(readFile({ .file_type = name, .isVector = true }));
			return 0;
		}
		wstring line = choose_line(object_num, "group");
		size_t start = 0;
		for (size_t i = 0; i <= line.length(); i++) {
			if (i == line.length() || line[i] == L'|') {
				if (i > start) {  // Избегаем пустых строк
					content.push_back(line.substr(start, i - start));
				}
				start = i + 1;
			}
		}
	}

	int translate_contents_for_scripts() {
		//для скриптов нужен перевод на русский, для групп просто вывод:
		if (!translated_scrlines.empty()) { translated_scrlines.clear(); }
		for (const auto& line : content) {
			if (line.size() < 5) {
				continue;
			}

			size_t pos = line.find(L"(");
			if (pos != std::wstring::npos) {
				std::wstring func_name = line.substr(0, pos);
				auto it = sorted_dict.find(func_name);
				//std::wcout << it->second.first << line.substr(pos) << std::endl;
				//std::wcout << line.substr(pos); Sleep(3000);
				if (line.substr(pos) == L"()") { translated_scrlines.push_back(L"клик на предпологаемом месте мышки"); }
				else { translated_scrlines.push_back(it->second + line.substr(pos)); }
			}
		}
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
		if (mode == "scripts") {
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
			if (mode == "scripts") {
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
			for(int li : nums_to_delete) {
				std::wcout << li <<L"\n";
			}
			system("pause");
			content = delete_lines_in_massive(content, nums_to_delete);
			return 0;
		}
		if (action_type == Add) {
			if (mode == "scripts") {
				new_line = python_script_make("", true, nullptr);
				new_line.pop_back();
				content = add_to_massive(content, line_num+4, new_line);
				return 0;
			}
			new_line = group_add(CURRENT_SETTINGS.path_view_num, true);
			add_to_massive(content, line_num, new_line);
			return 0;
		}
	}
	void save_changes() {
		if (mode == "scripts") {
			if (std::filesystem::exists(name.substr(1))) {
				if (remove((name.substr(1)).c_str()) == 0) { ; }
				else {
					std::wcerr << L"Ошибка при удалении старого файла";
				}
			}
			writefile(content, name, "", false);
			return;
		}
		if (mode == "group") {
			std::wstring retern_line;
			for (size_t i = 0; i < content.size(); i++) {
				retern_line += content[i] + L'|';
			}
			retern_line.pop_back();//удаление последнего |
			std::wcout << object_num << L"\n";
			system("pause");
			delete_lines_or_insert_or_add_one("group", {}, true, retern_line, object_num, false, false);
			std::wcout << object_num << L"\n";
			system("pause");
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

static void change_lines(string file_type) {
	//добавить выбор: путь, флаг, имя
	read_set(); //если расчитать возможный путь сюда, возможно можно избавиться
	vector<LineEntry> file_entry = file_parser(file_type);
	vector<wstring> paths;
	vector<wstring> names;
	for (const auto& entry : file_entry) {
		names.push_back(entry.name);
		paths.push_back(entry.path);
	}

	vector<wstring> lines_to_choose;
	for (int i = 0; i < paths.size(); ++i) {
		lines_to_choose.push_back(names[i] + L"  --  " + paths[i]);
	}

	int line_num = advansed_chooser({
		.lines_to_choose = lines_to_choose,
		.singleChoice = true,
		.title = L"Выберите что изменить:\nЕсли нет кастомного имени берётся .exe-имя\n"})[0];
	if (line_num == -1) { return; }
	LineEntry line_entry = line_parser(line_num, file_type, L"");
	wstring new_name = chooser(L"Введите новое имя\nБыло: "+line_entry.name);
	wstring new_line = line_entry.path + L"\"" + new_name + L"\"" + line_entry.flags;
	delete_lines_or_insert_or_add_one(file_type, {}, true, new_line, line_num, false, false);
}


int changer(string mode, PythonRuntime* python = nullptr) {
	if (mode != "scripts" or mode != "groups") { change_lines(mode); return 0; }
	
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
		if (mode == "scripts") editor.translate_contents_for_scripts();

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
			if (mode == "scripts") editor.translate_contents_for_scripts();
		}
	}
	return 0;
}

