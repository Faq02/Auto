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


#include "StartFuncs.h"
#include "settings.h"
#include "Groups.h"
#include "file_io.h"
#include "path_handler.h"
#include "converter.h"
#include "data_work.h"
#include "logger.h"
#include "ui_interactions.h"

using std::vector;
using std::wstring;
using std::string;
using std::get;

constexpr int standart_lines_count = 4;

constexpr int Delete = 2;
constexpr int Replace = 1;
constexpr int Add = 3;
constexpr int BACK_TO_ACTION = 1001;
constexpr int BACK_TO_OBJECT = 1002;
constexpr int EXIT_SAVE = 1003;
constexpr int EXIT_NO_SAVE = 1004;
constexpr int CONTINUE = 1005;
constexpr int EXIT_CODE = -1;

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
	string name; //для скриптов относительынй путь \scripts\name, для групп не используется

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
		
		lines_to_choose = showfile(FileType::Script, CURRENT_SETTINGS.showlines_num);

		object_num = advansed_chooser({ .lines_to_choose = lines_to_choose, .singleChoice = true, .title = L"Выберите " + config.what_choose + L"\n" })[0];
		return object_num;
	}
	
	int get_object_contents() {
		if (mode == FileType::Script) {
			wstring path = global_all_lines[FileType::Script][object_num-1].path;
			std::wstring path_name;
			path_name = extract_filename(path);
			name = WstringTo_Utf8(path_name);
			name = "scripts\\" + name;
			content = get<std::vector<std::wstring>>(readFile({ .file_path = name, .isVector = true }));
			return 0;
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
			if (line_num == EXIT_CODE) { return EXIT_CODE; }
			if (line_num == config.lines_to_choose.size()) { return EXIT_SAVE; }
			if (line_num == config.lines_to_choose.size() - 1) { return BACK_TO_OBJECT; }
			if (line_num == config.lines_to_choose.size() - 2) { return BACK_TO_ACTION; }
			return line_num;
		}
		nums_to_delete = advansed_chooser({
				.lines_to_choose = config.lines_to_choose,
				.singleChoice = false,
				.title = L"Выберите строки для удаления\n" });
		if (nums_to_delete.back() == EXIT_CODE) { nums_to_delete.pop_back(); return EXIT_CODE; }
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
	int action_handler() {
		//show_and_choose_object_content();
		std::wstring new_line;
		if (action_type == Replace) {
			if (mode == FileType::Script) {
				
				new_line.pop_back(); //удаляем \n который там создаётся
				//delete_lines_or_insert_or_add_one("scrtmp", {}, true, new_line, line_num + 4, false, false);
				std::wcout << L"before mass changes: \n";
				for (wstring line : content) {
					std::wcout << line + L"\n";
				}
				system("pause");
				content = massive_replace_line(content, line_num+standart_lines_count, new_line); //сразу обновляем и меняем
				std::wcout << L"after: \n";
				for (wstring line : content) {
					std::wcout << line + L"\n";
				}
				system("pause");
				return 0;
			}
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
				
				new_line.pop_back();
				content = add_to_massive(content, line_num+standart_lines_count, new_line);
				return 0;
			}
		}
	}
	void save_changes() {
		if (mode == FileType::Script) {
			if (std::filesystem::exists(name)) {
				if (remove(name.c_str()) == 0) { ; }
				else {
					std::wcerr << L"Ошибка при удалении старого файла";
				}
			}
			writefile(content, name, "", false);
			return;
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









void change_lines(FileType type, int id = 0) {
	//добавить выбор: путь, флаг, имя
	read_set(); //если расчитать возможный путь сюда, возможно можно избавиться
	LineEntry old_line_entry;
	if (id == 0) {
		vector<wstring> paths, names_accord_sett, flags;
		vector<int> ids = {};
		wstring shownamesSett = prog_settings(false, 3);//навсякий читаем прям из файла

		for (const auto& entry : global_all_lines[type]) {
			paths.push_back(entry.path);
			flags.push_back(entry.flags);
			ids.push_back(entry.id);
		}

		names_accord_sett = showfile(type, shownamesSett);
		vector<wstring> lines_to_choose;
		for (int i = 0; i < paths.size(); ++i) {
			lines_to_choose.push_back(
				L"Имя: " + names_accord_sett[i] + L"\n" +
				L"  Флаги: " + flags[i] + L"\n" +
				L"  Путь: " + paths[i] + L"\n" +
				L"  ID: " + std::to_wstring(ids[i]) //вообще может лучше не показывать его пользователю?
			);
		}

		int line_num = advansed_chooser({
			.lines_to_choose = lines_to_choose,
			.singleChoice = true,
			.title = L"Выберите объект для изминения:\n\033[31mесли в настроках стоит:\"показывать пути\",то автоматом ставится:\"имена/.exe имя\"!\n\033[0m" })[0];
		if (line_num == EXIT_CODE) return;
		old_line_entry = global_all_lines[type][line_num - 1];
	}
	
	if (id > 0) {
		if (const auto* entry_ptr = get_entry_by_id(type, id)) {
			old_line_entry = *entry_ptr;
		}
		else {log(L"changer.cpp 293 empty by id=" + std::to_wstring(id));}
	}
	bool end = false;
	wstring new_name;
	LineEntry new_line;
	int action;
	wstring path_choose_view_set = prog_settings(false, 1);
	
	while (end != true) {
		action = advansed_chooser({
			.lines_to_choose = {L"Имя", L"Флаги", L"Путь", L"Закончить"},
			.singleChoice = true,
			.title = L"Выберите что менять:\nЕсли нет кастомного имени берётся СТАРЫЕ ФЛАГИ ЗАМЕНЯТСЯ .exe-имя\n" })[0];
		std::vector<LineEntry> vec;
		switch (action) {
		case 1:
			new_name = input_line(L"Введите новое имя\nБыло: " + (old_line_entry.name.empty() ? L" {имени не было} " : old_line_entry.name));
			new_line.name = new_name;
			replace_entity({.type=type,.old_line=old_line_entry,.new_line_entry=new_line,.full=false,.path=false,.name=true,.flags=false});
			break;
		case 2:
			new_line.flags = choose_and_make_flags(old_line_entry.flags);
			replace_entity({ .type = type,.old_line = old_line_entry,.new_line_entry = new_line,
				.full = false,
				.flags = true });
			
			break;
		case 3:
			new_line.path = choose_file_on_pc(path_choose_view_set, type);
			
			replace_entity({ .type = type,.old_line = old_line_entry,.new_line_entry = new_line,
				.full = false,
				.path = true });
			break;
		default: end = true; break; // что-то кроме 1 2 3 - выходим, хотя возможно надо было дать 2 шанс 
		}
	}
}



std::wstring change_childs(FlagsContents flags) { //перенести в changer.cpp
	std::map<int, std::wstring> file_types_wstr = {
		{0,L"Игра"},
		{1,L"Программа"},
		{2,L"Ссылка"},
		{3,L"Скрипт"},
		{4,L"Группа"}
	};
	/*
	Редактирование детей для "Моя запись":

	Текущие дети:
	  1. Игра: "Cyberpunk 2077"
	  2. Программа: "Discord"
	  3. Ссылка: "YouTube"

	Выберите действие:
	  [1] Добавить детей
	  [2] Удалить детей
	  [3] Закончить
	*/
	struct ChildInfo { FileType type; int id = 0; };
	std::vector<ChildInfo> all_children = {};
	for (const auto& [type, ids] : flags.Children) {
		for (int id : ids) {
			all_children.push_back({ type, id });
		}
	}

	/*std::map<FileType, std::vector<int>> flags.Children*/





	while (true) {

		
		
		std::vector<std::wstring> lines_to_show = {};
		for (const auto& child : all_children) {
			const LineEntry* entry = get_entry_by_id(child.type, child.id);
			std::wstring name = L"";
			name = get_name_accords_sett(entry, CURRENT_SETTINGS.showlines_num, child.type);
			lines_to_show.push_back(file_types_wstr[(int)child.type] + L':' + name);
		}
		int childs_size = static_cast<int>(lines_to_show.size());
		lines_to_show.push_back(L"-");
		lines_to_show.push_back(L"Добавить");
		lines_to_show.push_back(L"Удалить");
		lines_to_show.push_back(L"Закончить");
		int choice = advansed_chooser({
			.lines_to_choose = lines_to_show,
			.singleChoice = true,
			.title = L"Выберите дочерние элементы для изминения самой линии(на которую указывает) или выберите действие" })[0];
		
		if (choice == EXIT_CODE) break;
		
		if (choice <= childs_size) {
			ChildInfo chousen_child = all_children[choice - 1];
			change_lines(chousen_child.type, chousen_child.id);
			continue;
		}
		if (choice == childs_size + 1) continue;
		int add = childs_size + 2;
		int del = childs_size + 3;
		int end = childs_size + 4;
		if (choice == add) {
			auto [file_type, chosen] = ask_lines_in_file_type();
			for (size_t i = 0; i < chosen.size(); ++i) {
				ChildInfo new_child;
				new_child.type = file_type;
				new_child.id = global_all_lines[file_type][chosen[i] - 1].id;
				all_children.push_back(new_child);
			}
			continue;
		}
		if (choice == del) {
			std::vector<int> lines_to_del = advansed_chooser({
			.lines_to_choose = lines_to_show,
			.singleChoice = false,
			.title = L"Выберите элементы для удаления" });
			std::sort(lines_to_del.begin(), lines_to_del.end(), std::greater<int>());
			for (int idx : lines_to_del) {
				if (idx >= 1 && idx <= all_children.size()) {
					all_children.erase(all_children.begin() + (idx - 1));
				}
			}
			continue;
		}
		if(choice == end) break;

	}
	//сохранение
	//удачи понимать
	std::map<FileType, std::vector<int>> grouped;
	for (const auto& child : all_children) {
		grouped[child.type].push_back(child.id);
	}

	std::wstring new_child_flag = L"Children";
	for (const auto& [type, ids] : grouped) {
		new_child_flag += L"{" + std::to_wstring(static_cast<int>(type)) + L"-";
		for (size_t i = 0; i < ids.size(); ++i) {
			if (i > 0) new_child_flag += L",";
			new_child_flag += std::to_wstring(ids[i]);
		}
		new_child_flag += L"}";
	}
	return new_child_flag;

}

int changer(FileType mode) {
	if (mode != FileType::Script and mode != FileType::Group) { change_lines(mode, 0); return 0; }
	Changer editor;
	editor.setconfig(mode);
	bool end =false;
	bool changed = false;
	int action;
	while (true) {
		if (end) { break; }
		// 1. ВЫБОР ОБЪЕКТА
		if (editor.choose_object() == EXIT_CODE) return 0;

		// 2. ЗАГРУЗКА
		editor.get_object_contents();
		if (mode == FileType::Script) editor.translate_contents_for_scripts();

		// 3. ЦИКЛ РЕДАКТИРОВАНИЯ
		
		while (true) {
			// 3.1 Выбор действия
			if (!changed) { action = editor.choose_object_action(); }
			changed = false;

			if (action == BACK_TO_OBJECT or action == EXIT_CODE) {
				// спросить пользователя?
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

			if (selection == BACK_TO_ACTION or selection == EXIT_CODE) continue;
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
			editor.action_handler();
			changed = true;
			// 3.4 Обновить перевод (если скрипт)
			if (mode == FileType::Script) editor.translate_contents_for_scripts();
		}
	}
	return 0;
}

