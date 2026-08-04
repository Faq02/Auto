#include <string>
#include <locale>
#include <map>
#include <any>
#include <stdexcept>
#include <algorithm>
#include <cwctype>

#include "file_io.h"
#include "data_work.h"
#include "win_help.h"
#include "ui_interactions.h"
#include "StartFuncs.h"
#include <synchapi.h>


enum class Commands { clickByCoords, RclickByCoords,WAIT,execute_fastkeys,execute_fastkey,sendtext,start };

Commands get_comand_code_by_wstr(std::wstring key_code_wstr) {
	std::transform(key_code_wstr.begin(), key_code_wstr.end(), key_code_wstr.begin(), towlower);
	std::map<std::wstring, Commands> commands_wstr_version = {
		{L"rclick",    Commands::RclickByCoords },
		{L"rightclick",Commands::RclickByCoords },
		{L"click",     Commands::clickByCoords },
		{L"leftclick", Commands::clickByCoords },
		{L"lclick",    Commands::clickByCoords },

		{L"wait", Commands::WAIT},
		{L"sleep",Commands::WAIT},

		{L"fastkeys",Commands::execute_fastkeys},
		{L"keys",Commands::execute_fastkeys},

		{L"key",Commands::execute_fastkey},
		{L"press_key",Commands::execute_fastkey},
		{L"presskey",Commands::execute_fastkey},
		{L"keypress",Commands::execute_fastkey},

		{L"keyboardtext",Commands::sendtext},
		{L"text",Commands::sendtext},
		{L"sendtext",Commands::sendtext},

		{L"start",Commands::start},
		{L"run",Commands::start},
	};
	return commands_wstr_version[key_code_wstr];
}



bool click_start(std::vector<std::wstring> coords, bool rclick, int& line_counter) {
	try {
		win_click_on_pos(std::stoi(coords.at(0)), std::stoi(coords.at(1)),rclick ? true : false);
		return true;
	}
	catch (const std::exception& e) {
		colorfulPrint(L"Ошибка в линии(попытка клика):" + std::to_wstring(line_counter), PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
		system("pause");
		return false;
	}
}

std::vector<std::wstring> tokenize_with_quotes(const std::wstring& line) {
	std::vector<std::wstring> tokens;
	std::wstring current;
	bool in_quotes = false;

	for (size_t i = 0; i < line.size(); ++i) {
		wchar_t ch = line[i];

		if (ch == L'\"') {
			in_quotes = !in_quotes;
			current += ch; // если хотим сохранить кавычки в токене
			continue;
		}

		if (!in_quotes && (ch == L' ' || ch == L'\t')) {
			if (!current.empty()) {
				tokens.push_back(current);
				current.clear();
			}
			continue;
		}

		current += ch;
	}

	if (!current.empty()) {
		tokens.push_back(current);
	}

	return tokens;
}




int tokinizer(std::string scr_path) {
	
	//может быть rclick Rclick RCLICK RIGHTCLICK CLICK 



	/*sendtext ""*/
	

	const auto scr_lines = std::get<std::vector<std::wstring>> (readFile({ .file_path = scr_path,.for_full_read = true,.for_py_code = false,.isVector = true }));
	int line_counter = 0;
	for (const auto& line : scr_lines) {
		line_counter++;
		std::vector<std::wstring> space_splited = split(line, L' ');//где пока 1-100% команда-ключевое слово
		if (space_splited[0][0] == L'#' or space_splited[0][0] == L' ' or space_splited[0].empty()) continue;
		int command = (int)get_comand_code_by_wstr(space_splited[0]);
		std::vector<std::wstring> args = split(space_splited[1], L',');
		bool res;
		switch (command) {
		case (int)Commands::clickByCoords:
			click_start(split(space_splited[1], L','), false, line_counter); break;
		case (int)Commands::RclickByCoords:
			click_start(split(space_splited[1], L','), true, line_counter); break;
		case (int)Commands::WAIT:
			try {
				Sleep(std::stoi(args[0]) * 1000);//*1000 так как в секундах хочу
				break;
			}
			catch (const std::exception& e) {
				colorfulPrint(L"Ошибка в линии:" + line_counter, PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
				system("pause");
				return -1;
			}
		case (int)Commands::execute_fastkeys:
			try {
				res = SendShortcut(split(space_splited[1], L'+'));
				if (res == false) {
					colorfulPrint(L"Ошибка в линии(возможно нету такого keys):" + line_counter, PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
					system("pause");
				}
			}
			catch (const std::exception& e) {
				colorfulPrint(L"Ошибка в линии:" + line_counter, PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
				system("pause");
			}
			break;
		case (int)Commands::execute_fastkey:
			try {
				res = SendShortcut({ space_splited[1] });
				if (res == false) {
					colorfulPrint(L"Ошибка в линии(возможно нету такого key):" + line_counter, PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
					system("pause");
				}
			}
			catch (const std::exception& e) {
				colorfulPrint(L"Ошибка в линии:" + line_counter, PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
				system("pause");
			}
			break;
		case (int)Commands::sendtext:
			try {
				//найти первую ковычку и идти по линиям дальше пока не встречу вторую
				std::vector<std::wstring> vec = tokenize_with_quotes(line);
				if (vec.empty()) continue;
				if (vec[0] == L"#") continue;

				if (vec.size() < 2) {
					// Ошибка: нет текста
					colorfulPrint(L"Ошибка: нет текста для отправки", PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
					return -1;
				}
				std::wstring text = vec[1];

				if (text.front() == L'\"' && text.back() == L'\"') {
					text = text.substr(1, text.length() - 2);
				}
				SendText(text);
				break;
			}
			catch (const std::exception& e) {
				colorfulPrint(L"Ошибка в линии:" + line_counter, PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
				system("pause");
				break;
			}

		case (int)Commands::start:
			try {
				std::vector<std::wstring> vec = tokenize_with_quotes(line);
				if (vec.empty()) continue;
				if (vec[0] == L"#") continue;
				if (vec.size() < 2) {
					// Ошибка: нет текста
					colorfulPrint(L"нету пути для старта, линия: " + line_counter, PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
					return -1;
				}
				std::wstring path = vec[1];
				LineEntry entry;
				entry.path = path;
				startfilesN(FileType::null, entry, "", false);
			}
			catch (const std::exception& e) {
				colorfulPrint(L"Ошибка в линии:" + line_counter, PRINT_TEXTCOLOR::BLACK, PRINT_BACKGROUNDCOLOR::RED);
				system("pause");
			}
			break;
		}

	}
}