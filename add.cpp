#include <iostream>
#include "header.h"
#include <bits/stdc++.h>
#include <string>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

// Function to compute hash for file content (same as in write-tree)
string computeHashForFile(const string& filePath) {
    // Read file content
    ifstream file(filePath, ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    ostringstream buffer;
    buffer << file.rdbuf();
    string fileContent = buffer.str();
    
    // Create blob format: "blob <size>\0<content>"
    string blobContent = "blob " + to_string(fileContent.length()) + '\0' + fileContent;
    
    // Return hash of blob content
    return computeSHA1FromString(blobContent); // Use your existing SHA1 function
}

void addtoindex(const string& filePath, const string& hash) {
    // Skip validation since hash is already computed
    fs::path pathObj(filePath);
    string fileName = pathObj.filename().string();

    // Skip hidden files (those starting with a dot)
    if (isHidden(pathObj)) {
        cout << "Skipping hidden file: " << fileName << "\n";
        return;
    }

    // Create .mygit/index file if it doesn't exist
    fs::path indexPath = ".mygit/index";
    if (!fs::exists(indexPath.parent_path())) {
        fs::create_directories(indexPath.parent_path());
    }

    if (!fs::exists(indexPath)) {
        ofstream createFile(indexPath);
        createFile.close();
    }

    // Open file in append mode
    ofstream outFile(indexPath, ios::app);
    if (!outFile) {
        cerr << "Error: Cannot open index file" << endl;
        return;
    }

    // Index format: mode hash filename (or keep your "Blob" format)
    string indexEntry = "100644 " + hash + " " + filePath;
    outFile << indexEntry << '\n';
    
    cout << "Added to staging area: " << filePath << endl;
    outFile.close();
}

// FIXED: Single function to create blob and add to index
bool addFileToStaging(const string& filePath) {
    // 1. Compute hash for the file
    string hash = computeHashForFile(filePath);
    if (hash.empty()) {
        cerr << "Error: Failed to compute hash for " << filePath << endl;
        return false;
    }
    
    // 2. Create and store the blob object
    bool success = hashObject(filePath, true);
    if (!success) {
        cerr << "Error: Failed to create blob for " << filePath << endl;
        return false;
    }
    
    // 3. Add to index
    addtoindex(filePath, hash);
    return true;
}

void listFilesDFS(const fs::path& path) {
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (isHidden(entry.path())) {
            continue; // Skip hidden files and directories
        }

        if (fs::is_regular_file(entry.status())) {
            cout << "Processing file: " << entry.path() << '\n';
            // FIXED: Single function call
            addFileToStaging(entry.path().string());
        }
    }
}

void add(const string& filename) {
    // Check if repository is initialized
    if (!fs::exists(".mygit")) {
        cerr << "Error: Not a mygit repository. Run 'mygit init' first.\n";
        return;
    }

    if (filename == ".") {
        cout << "Adding all files in current directory...\n";
        listFilesDFS(".");
    } else if (fs::exists(filename)) {
        if (fs::is_regular_file(filename)) {
            if (!isHidden(filename)) {
                cout << "Adding file: " << filename << endl;
                // FIXED: Single function call
                addFileToStaging(filename);
            } else {
                cout << "Skipping hidden file: " << filename << endl;
            }
        } else if (fs::is_directory(filename)) {
            cout << "Adding directory: " << filename << endl;
            listFilesDFS(filename);
        }
    } else {
        cerr << "Error: File or directory '" << filename << "' does not exist\n";
    }
}

// Command handler for main.cpp integration
bool handleAdd(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: mygit add <file1> [file2] ... or mygit add .\n";
        return false;
    }
    
    // Process all specified files
    for (int i = 2; i < argc; i++) {
        add(argv[i]);
    }
    
    return true;
}