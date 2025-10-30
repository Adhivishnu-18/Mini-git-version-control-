#include <filesystem>
#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>  

using namespace std;
namespace fs = filesystem;

bool initialize() 
{
    string dir_name = ".mygit";
    cout << dir_name << " ";

    // Check if repository already exists and handle it gracefully
    if (fs::exists(dir_name)) {
        std::cout << "A git repository already exists in " << dir_name << "\n";
        return false;  // Return false to indicate no new initialization was done
    }

    try {
        // Create necessary directories for git structure
        fs::create_directories(dir_name);
        fs::create_directories(dir_name + "/objects");
        fs::create_directories(dir_name + "/refs/heads");
        fs::create_directories(dir_name + "/refs/tags");
        fs::create_directories(dir_name + "/logs");  // ADD THIS LINE

        // Create index file
        ofstream file(dir_name + "/index");
        if (file.is_open()) {
            file.close();
            cout << "File created: " << dir_name + "/index" << '\n';
        }

        // Create HEAD file pointing to master branch
        ofstream headFile(dir_name + "/HEAD");
        if (headFile.is_open()) {
            headFile << "";
            headFile.close();
        } else {
            std::cerr << "Error: Could not create HEAD file\n";
            return false;
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << "\n";  // Catch any standard exceptions
        return false;
    }

    return true;
}