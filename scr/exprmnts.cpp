#include <windows.h>
#include <iostream>
#include <string>
using namespace std;
int main() 
{
	string path_raw = R"C:\ярлыки\вортекс";
	string path;
	for (i = 0; i < path_raw.lenght(); i++)
	{
		if (path_raw[i] == '\\')
		{
			path += '/';
		}
		path += path_raw[i];
	}
	cout << path << endl;
	return 0;
}