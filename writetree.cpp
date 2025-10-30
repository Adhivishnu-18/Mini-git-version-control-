#include <openssl/sha.h>
#include <openssl/evp.h>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <string>
#include <filesystem>
#include <iostream>
#include <vector>
#include <zlib.h>  
#include <bits/stdc++.h>
#include "header.h"

namespace fs = std::filesystem;
using namespace std;

// Helper function to read file content
string readFileContent(const string& filePath) {
    ifstream file(filePath, ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Function to create and store a blob object, returns hash
string hashObject(const fs::path& filePath, bool writeFlag) {
    string fileContent = readFileContent(filePath.string());
    if (fileContent.empty() && fs::file_size(filePath) > 0) {
        return "";
    }
    
    // Create blob format
    string blobContent = "blob " + to_string(fileContent.length()) + '\0' + fileContent;
    string hash = computeSHA1FromString(blobContent); // Use function from utilities.cpp
    
    if (writeFlag) {
        // Compress the blob content
        uLongf compressedSize = compressBound(blobContent.size());
        vector<unsigned char> compressedData(compressedSize);
        
        if (compress(compressedData.data(), &compressedSize,
                    reinterpret_cast<const Bytef*>(blobContent.data()),
                    blobContent.size()) != Z_OK) {
            return "";
        }
        
        // Store compressed blob
        string objectDir = ".mygit/objects/" + hash.substr(0, 2);
        fs::create_directories(objectDir);
        string objectPath = objectDir + "/" + hash.substr(2);
        
        ofstream blobFile(objectPath, ios::binary);
        if (!blobFile) {
            return "";
        }
        
        blobFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedSize);
        blobFile.close();
    }
    
    return hash;
}

// Main writeTree function
string writeTree(const fs::path& dirPath = ".") {
    cout << "Creating tree structure for: \"" << fs::absolute(dirPath) << "\"" << endl;
    
    if (!fs::exists(".mygit/objects")) {
        fs::create_directories(".mygit/objects");
    }

    vector<tuple<string, string, string>> entries; 
    // (filename, mode, hash)

    // Collect all entries
    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (isHidden(entry.path())) continue; // Use function from utilities.cpp

        string entryName = entry.path().filename().string();
        string mode, hash;

        if (fs::is_regular_file(entry.status())) {
            // Create blob and get its hash
            hash = hashObject(entry.path(), true);  
            if (hash.empty()) {
                cerr << "Failed to create blob for " << entry.path() << endl;
                continue;
            }
            mode = "100644"; // regular file
        }
        else if (fs::is_directory(entry.status())) {
            // Recursively create subtree
            hash = writeTree(entry.path());
            if (hash.empty()) {
                continue;
            }
            mode = "40000"; // directory (tree)
        }
        entries.push_back({entryName, mode, hash});
    }

    // Sort entries by filename
    sort(entries.begin(), entries.end(),
         [](const auto& a, const auto& b) {
             return get<0>(a) < get<0>(b);
         });

    // Build tree content
    string fullContent;
    for (const auto& entry : entries) {
        string filename = get<0>(entry);
        string mode     = get<1>(entry);
        string hashHex  = get<2>(entry);

        // mode + space + filename + null
        fullContent += mode;
        fullContent += " ";
        fullContent += filename;
        fullContent += '\0';

        // convert hash hex -> raw 20 bytes
        for (size_t i = 0; i < hashHex.size(); i += 2) {
            string byteStr = hashHex.substr(i, 2);
            char byte = (char)stoi(byteStr, nullptr, 16);
            fullContent.push_back(byte);
        }
    }

    // Add tree header
    string header = "tree " + to_string(fullContent.size()) + '\0';
    string completeObject = header + fullContent;

    // Hash the whole tree object
    string treeHash = computeSHA1FromString(completeObject); // Use function from utilities.cpp

    // Compress
    uLongf compressedSize = compressBound(completeObject.size());
    vector<unsigned char> compressedData(compressedSize);

    if (compress(compressedData.data(), &compressedSize,
                 reinterpret_cast<const Bytef*>(completeObject.data()),
                 completeObject.size()) != Z_OK) {
        cerr << "Error: Compression failed\n";
        return "";
    }

    // Save in .mygit/objects/xx/yyyy... format
    string objectDir = ".mygit/objects/" + treeHash.substr(0, 2);
    fs::create_directories(objectDir);
    string objectPath = objectDir + "/" + treeHash.substr(2);

    ofstream treeFile(objectPath, ios::binary);
    if (!treeFile) {
        cerr << "Error: Could not create tree object file\n";
        return "";
    }
    treeFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedSize);
    treeFile.close();

    cout << "Created tree object with hash: " << treeHash << endl;
    return treeHash;
}

// Handler function for command line interface
bool handleWriteTree(int argc, char* argv[]) {
    // Check if repository is initialized
    if (!fs::exists(".mygit")) {
        cerr << "Error: Not a mygit repository. Run 'mygit init' first.\n";
        return false;
    }
    
    string treeHash = writeTree(".");
    if (treeHash.empty()) {
        cerr << "Error: Failed to create tree\n";
        return false;
    }
    
    // Output just the hash for command line usage (like git write-tree)
    cout << treeHash << endl;
    return true;
}