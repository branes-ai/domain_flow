#include <iostream>
#include <fstream>
#include <iomanip>
#include <format>
#include <filesystem>
#include <nlohmann/json.hpp>


int main() {
	using json = nlohmann::json;

	auto testFile = std::filesystem::absolute(std::filesystem::path{ __FILE__ }).parent_path() /= "test.json";
    const std::filesystem::path& path{ testFile };

    std::ifstream f(path);
    json data = json::parse(f);

	f.close();

	std::cout << std::setw(4) << data << std::endl;

    return EXIT_SUCCESS;
}
