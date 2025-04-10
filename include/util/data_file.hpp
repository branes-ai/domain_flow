#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdexcept>

std::string replaceExtension(const std::string& filename, const std::string& oldExtension, const std::string& newExtension) {
    std::string result = filename;
    size_t pos = result.rfind(oldExtension);

    if (pos != std::string::npos && pos == result.length() - oldExtension.length()) {
        result.replace(pos, oldExtension.length(), newExtension);
    }
    return result;
}

std::string generateDataFile(const std::string& fileName) {
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path rootPath = currentPath;

    // Navigate up until you find the repository root (containing .git).
    while (!std::filesystem::exists(rootPath / ".git") && rootPath.has_parent_path()) {
        rootPath = rootPath.parent_path();
    }

    if (!std::filesystem::exists(rootPath / ".git")) {
        throw std::runtime_error("Could not find repository root.");
    }

    std::filesystem::path dataFilePath = rootPath / "data" / fileName;

    std::string dataFileName{};
    if (std::filesystem::exists(dataFilePath)) {
        dataFileName = dataFilePath.string();
    }
	else {
		throw std::runtime_error("Data file not found: " + dataFilePath.string());
	}

	return dataFileName;
}

std::string readDataFile(const std::string& fileName) {
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path rootPath = currentPath;

    // Navigate up until you find the repository root (containing .git).
    while (!std::filesystem::exists(rootPath / ".git") && rootPath.has_parent_path()) {
        rootPath = rootPath.parent_path();
    }

    if (!std::filesystem::exists(rootPath / ".git")) {
        throw std::runtime_error("Could not find repository root.");
    }

    std::filesystem::path dataFilePath = rootPath / "data" / fileName;

    if (std::filesystem::exists(dataFilePath)) {
        std::ifstream file(dataFilePath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open data file.");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    } else {
        throw std::runtime_error("Data file not found: " + dataFilePath.string());
    }
}

// then use this pattern in the calling environment
//int main() {
//    try {
//        std::string data = readDataFile("test_data.txt");
//        std::cout << "Data: " << data << std::endl;
//    } catch (const std::runtime_error& e) {
//        std::cerr << "Error: " << e.what() << std::endl;
//    }
//    return 0;
//}