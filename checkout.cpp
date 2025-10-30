#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <iomanip>
#include <zlib.h>
#include <cstring>
#include "header.h"

namespace fs = std::filesystem;
using namespace std;

// Clear working directory (except .mygit)
void clearWorkingDirectory() {
    try {
        for (const auto& entry : fs::directory_iterator(".")) {
            if (entry.path().filename() == ".mygit") continue;
            
            if (fs::is_directory(entry)) {
                fs::remove_all(entry);
            } else {
                fs::remove(entry);
            }
        }
    } catch (const fs::filesystem_error& e) {
        cerr << "Warning: Error clearing working directory: " << e.what() << endl;
    }
}

// Restore tree recursively
bool restoreTree(const string& treeSHA, const fs::path& currentPath) {
    vector<TreeEntry> entries = readTreeEntries(treeSHA);
    if (entries.empty()) {
        return false;
    }

    for (const auto& entry : entries) {
        fs::path fullPath = currentPath / entry.name;

        if (entry.type == "blob") {
            // Read blob content
            string objectContent = readObjectFile(entry.sha);
            if (objectContent.empty()) {
                cerr << "Error: Blob object " << entry.sha << " not found for " << entry.name << "\n";
                continue;
            }

            auto [type, size, content] = parseObject(objectContent);
            if (type != "blob") {
                cerr << "Error: Expected blob, got " << type << " for " << entry.name << "\n";
                continue;
            }

            // Create parent directories if needed
            if (fullPath.has_parent_path()) {
                fs::create_directories(fullPath.parent_path());
            }

            // Write file content
            ofstream outFile(fullPath, ios::binary);
            if (!outFile) {
                cerr << "Error: Cannot create file " << fullPath << "\n";
                continue;
            }
            outFile.write(content.c_str(), content.length());
            outFile.close();
            
            cout << "Restored file: " << fullPath << "\n";
            
        } else if (entry.type == "tree") {
            // Create directory and recursively restore subtree
            if (!fs::exists(fullPath)) {
                fs::create_directories(fullPath);
            }
            cout << "Created directory: " << fullPath << "\n";
            
            if (!restoreTree(entry.sha, fullPath)) {
                cerr << "Error: Failed to restore subtree " << entry.sha << "\n";
                return false;
            }
        }
    }
    
    return true;
}

// Main checkout function
bool checkout(const string& commitSHA) {
    // Check if repository exists
    if (!fs::exists(".mygit")) {
        cerr << "Error: Not a mygit repository. Run 'mygit init' first.\n";
        return false;
    }

    // Validate commit SHA format
    if (commitSHA.length() != 40) {
        cerr << "Error: Invalid commit SHA format\n";
        return false;
    }

    // Check if commit object exists
    if (!objectExists(commitSHA)) {
        cerr << "Error: Commit " << commitSHA << " does not exist\n";
        return false;
    }

    // Get tree SHA from commit
    string treeSHA = getTreeSHAFromCommit(commitSHA);
    if (treeSHA.empty()) {
        cerr << "Error: Cannot extract tree from commit\n";
        return false;
    }

    cout << "Checking out commit " << commitSHA << "\n";
    cout << "Tree SHA: " << treeSHA << "\n";

    // Clear current working directory (except .mygit)
    cout << "Clearing working directory...\n";
    clearWorkingDirectory();

    // Restore files from tree object
    cout << "Restoring files from tree...\n";
    if (!restoreTree(treeSHA,".")) {
        cerr << "Error: Failed to restore tree\n";
        return false;
    }

    // Update HEAD to point to the checked out commit
    writeHEAD(commitSHA);

    cout << "Successfully checked out commit " << commitSHA << "\n";
    return true;
}

// Command handler for main.cpp integration
bool handleCheckout(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: mygit checkout <commit-sha>\n";
        return false;
    }
    
    string commitSHA = argv[2];
    return checkout(commitSHA);
}