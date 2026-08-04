#include <string>
#include <locale>
#include <vector>
#include <algorithm>
#include <cwctype>

#include "file_io.h"

//1 раз читается файл с .fq в память ограничение-100 000 i I guess


bool is_blank_or_empty(const std::wstring& line) {
	// line.empty() проверяет отсутствие символов
	// std::all_of проверяет, являются ли все символы пробельными (включая \t)
	return line.empty() || std::all_of(line.begin(), line.end(), [](unsigned char c) {
		return std::isspace(c);
		});
}



class Fq {
public:
	void set_private(std::string& path_to_scR) {
		path_to_scr = path_to_scR;
	}



private:

	std::string path_to_scr;
	std::vector<std::wstring> script;


	
	enum class OpCode {
		CLICK,
		WAIT,
		LOOP_START, // Начало цикла
		LOOP_END    // Конец цикла
	};


	/*# Запуск браузера
	RUN "C:\Program Files\Google\Chrome\Application\chrome.exe"
	WAIT 2000
	# Создаем цикл на 5 повторений
	# Синтаксис: REPEAT [кол-во] [имя_метки]
	REPEAT 5
		CLICK 500, 400
		WAIT 500
		# Ищем картинку внутри региона (x, y, w, h)
		# Если нашли — кликаем, если нет — идем дальше
		FIND_IMAGE "button.png" IN_REGION 100, 100, 400, 400
		WAIT 1000
	# Конец цикла
	END_REPEAT*/




	void Tokinizer() {
		for (const auto& line : script) {
			if (!is_blank_or_empty(line)) {

			}
		}


		struct Command {
			OpCode op;                 
			int arg1 = 0;              
			int arg2 = 0;              
			std::wstring& text_arg;     

			int target_line = -1;      
		};






	}
	void read() {
		script = std::get<std::vector<std::wstring> > (readFile({
			.file_path = path_to_scr,
			.for_full_read = true,
			.isVector = true
			}));
	}
	
};