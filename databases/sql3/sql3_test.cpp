#include <iostream>
#include <iomanip>
#include <sqlite3.h>

int main() 
{
	std::cout << "Sqlite3 version: " << sqlite3_libversion() << '\n';
	return EXIT_SUCCESS;
}
