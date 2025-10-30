#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <iomanip>
#include <openssl/sha.h>
#include <zlib.h>
#include <cstring> 
#include "header.h"

using namespace std;
namespace fs = filesystem;

// Function to parse git object header
pair<string, size_t> parseHeader(const string& content) {
    size_t nullPos = content.find('\0');
    if (nullPos == string::npos) {
        return {"", 0};
    }
    
    string header = content.substr(0, nullPos);
    istringstream iss(header);
    string type;
    size_t size;
    iss >> type >> size;
    
    return {type, nullPos + 1};
}

// Function to read a tree object from a file
vector<TreeEntry> readTree(const string& treeSHA) {
    vector<TreeEntry> entries;
    
    // FIXED: Remove .gz extension to match your write-tree implementation
    string objectPath = ".mygit/objects/" + treeSHA.substr(0, 2) + "/" + 
                       treeSHA.substr(2);  // No .gz extension
    
    // Read compressed file
    ifstream file(objectPath, ios::binary);
    if (!file) {
        cerr << "Error: Could not open tree object file: " << objectPath << endl;
        return entries;
    }
    
    // Read the entire file into a vector
    vector<unsigned char> compressedData(
        (istreambuf_iterator<char>(file)),
        istreambuf_iterator<char>()
    );
    
    // Decompress the data using utility function
    string decompressed = decompressData(compressedData);
    if (decompressed.empty()) {
        cerr << "Error: Decompression failed for tree object\n";
        return entries;
    }
    
    // Parse header
    auto [type, contentStart] = parseHeader(decompressed);
    if (type != "tree") {
        cerr << "Error: Invalid tree object format\n";
        return entries;
    }
    
    // Parse entries using utility function
    size_t pos = contentStart;
    while (pos < decompressed.length()) {
        TreeEntry entry = parseTreeEntry(decompressed, pos);
        if (entry.sha.empty()) break;  // Break if parsing failed
        entries.push_back(entry);
    }
    
    return entries;
}

// Function to list the contents of a tree object
void lsTree(const string& treeSHA, bool nameOnly) {
    vector<TreeEntry> entries = readTree(treeSHA);
    if (entries.empty()) {
        cerr << "Error: No entries found for tree SHA: " << treeSHA << endl;
        return;
    }
    
    // Display entries based on the flag
    for (const auto& entry : entries) {
        if (nameOnly) {
            cout << entry.name << endl;
        } else {
            cout << entry.mode << " " << entry.type << " " 
                 << entry.sha << "\t" << entry.name << endl;
        }
    }
}

// ADDED: Command handler function for main.cpp integration
bool handleLsTree(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: mygit ls-tree [--name-only] <tree-sha>\n";
        return false;
    }
    
    // Check if repository exists
    if (!fs::exists(".mygit")) {
        cerr << "Error: Not a mygit repository. Run 'mygit init' first.\n";
        return false;
    }
    
    bool nameOnly = false;
    string treeSHA;
    
    // Parse arguments
    for (int i = 2; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--name-only") {
            nameOnly = true;
        } else {
            if (treeSHA.empty()) {
                treeSHA = arg;
            } else {
                cerr << "Error: Multiple tree SHAs specified\n";
                return false;
            }
        }
    }
    
    if (treeSHA.empty()) {
        cerr << "Error: No tree SHA specified\n";
        return false;
    }
    
    // Validate SHA format (basic check)
    if (treeSHA.length() != 40) {
        cerr << "Error: Invalid SHA format\n";
        return false;
    }
    
    lsTree(treeSHA, nameOnly);
    return true;
}