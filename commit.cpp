#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <vector>
#include <string>
#include <iomanip>
#include <openssl/evp.h>
#include <zlib.h>
#include "header.h"
#include <algorithm>

namespace fs = std::filesystem;
using namespace std;

// Function to get the current timestamp in Git format
string getCurrentTimestamp() {
    auto now = chrono::system_clock::now();
    time_t time = chrono::system_clock::to_time_t(now);
    stringstream ss;
    // Git timestamp format: seconds-since-epoch +timezone
    ss << time << " +0000"; // Using UTC for simplicity
    return ss.str();
}

// Function to update the branch reference
void updateRef(const string& commitHash) {
    fs::create_directories(".mygit/refs/heads");
    ofstream branchFile(".mygit/refs/heads/master");
    if (branchFile) {
        branchFile << commitHash;
    }
}

// Function to write to log file (NEW FUNCTION)
void writeToLog(const string& oldHash, const string& newHash, const string& message) {
    // Create logs directory if it doesn't exist
    fs::create_directories(".mygit/logs");
    
    // Open log file in append mode
    ofstream logFile(".mygit/logs/HEAD", ios::app);
    if (!logFile) {
        cerr << "Warning: Could not write to log file\n";
        return;
    }
    
    string timestamp = getCurrentTimestamp();
    string committerInfo = "Committer <committer@example.com>";
    
    // Format: oldHash newHash committerInfo timestamp commit: message
    logFile << (oldHash.empty() ? string(40, '0') : oldHash) << " " 
            << newHash << " " << committerInfo << " " 
            << timestamp << " commit: " << message << "\n";
    
    logFile.close();
}

// Function to compress data using zlib
string compressData(const string& data) {
    uLongf compressedSize = compressBound(data.length());
    vector<unsigned char> compressedData(compressedSize);
    
    int result = compress(
        compressedData.data(), 
        &compressedSize,
        reinterpret_cast<const Bytef*>(data.c_str()), 
        data.length()
    );
    
    if (result != Z_OK) {
        cerr << "Error: Compression failed\n";
        return "";
    }
    
    return string(reinterpret_cast<char*>(compressedData.data()), compressedSize);
}

// Function to write compressed object to store
void writeCompressedObject(const string& hash, const string& content) {
    string objectDir = ".mygit/objects/" + hash.substr(0, 2);
    string objectPath = objectDir + "/" + hash.substr(2); // No .gz extension
    
    fs::create_directories(objectDir);
    
    // Compress the content
    string compressedData = compressData(content);
    if (compressedData.empty()) {
        cerr << "Error: Failed to compress object\n";
        return;
    }
    
    ofstream objectFile(objectPath, ios::binary);
    if (!objectFile) {
        cerr << "Error: Could not write to object store\n";
        return;
    }
    
    objectFile.write(compressedData.c_str(), compressedData.size());
    objectFile.close();
}

// Function to create a tree from the current directory (like write-tree)
string createTreeFromWorkingDirectory() {
    // Use your existing writeTree function from tree.cpp
    return writeTree("."); // This should return the tree hash
}

// Alternative: Create tree from index (if you want to use staging area)
string createTreeFromIndex() {
    ifstream indexFile(".mygit/index");
    if (!indexFile) {
        cerr << "Error: No staged files. Use 'mygit add' first.\n";
        return "";
    }

    vector<tuple<string, string, string>> entries; // mode, hash, path
    string line;
    
    // Parse index entries
    while (getline(indexFile, line)) {
        if (line.empty()) continue;
        
        istringstream iss(line);
        string mode, type, hash, path;
        
        // Parse your index format: "100644 hash path" or "100644 Blob hash path"
        if (line.find(" Blob ") != string::npos) {
            // Format: "100644 Blob hash path"
            iss >> mode >> type >> hash;
            getline(iss, path);
            path = path.substr(1); // Remove leading space
        } else {
            // Format: "100644 hash path"
            iss >> mode >> hash;
            getline(iss, path);
            path = path.substr(1); // Remove leading space
        }
        
        entries.push_back({mode, hash, path});
    }

    if (entries.empty()) {
        cerr << "Error: Nothing to commit (empty index)\n";
        return "";
    }

    // Sort entries by path
    sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return get<2>(a) < get<2>(b);
    });

    // Build tree content in Git format
    string treeContent;
    for (const auto& entry : entries) {
        string mode = get<0>(entry);
        string hash = get<1>(entry);
        string path = get<2>(entry);
        
        // Git tree entry format: [mode] [filename]\0[20-byte binary SHA]
        treeContent += mode + " " + path + '\0';
        
        // Convert hex hash to binary
        for (size_t i = 0; i < hash.length(); i += 2) {
            string byteStr = hash.substr(i, 2);
            char byte = static_cast<char>(stoi(byteStr, nullptr, 16));
            treeContent += byte;
        }
    }

    // Create tree object with header
    string header = "tree " + to_string(treeContent.length());
    string completeTree = header + '\0' + treeContent;
    
    // Calculate hash and store
    string treeHash = computeSHA1FromString(completeTree);
    writeCompressedObject(treeHash, completeTree);
    
    return treeHash;
}

// Function to create a commit
string createCommit(const string& message, const string& treeHash, const string& parentHash) {
    string authorInfo = "Author <author@example.com>";
    string committerInfo = "Committer <committer@example.com>";
    string timestamp = getCurrentTimestamp();

    stringstream commitContent;
    commitContent << "tree " << treeHash << "\n";
    
    if (!parentHash.empty()) {
        commitContent << "parent " << parentHash << "\n";
    }
    
    commitContent << "author " << authorInfo << " " << timestamp << "\n";
    commitContent << "committer " << committerInfo << " " << timestamp << "\n";
    commitContent << "\n" << message << "\n";

    string content = commitContent.str();

    // Create commit object with proper header
    string header = "commit " + to_string(content.size());
    string completeCommit = header + '\0' + content;

    // Compute hash and store
    string commitHash = computeSHA1FromString(completeCommit);
    writeCompressedObject(commitHash, completeCommit);

    return commitHash;
}

// Function to clear the index after commit
bool clearIndex() {
    ofstream indexFile(".mygit/index", ios::trunc);
    if (!indexFile) {
        cerr << "Error: Cannot clear index file\n";
        return false;
    }
    indexFile.close();
    return true;
}

// Main function to handle the commit process
int handleCommit(int argc, char* argv[]) {
    // Check if repository exists
    if (!fs::exists(".mygit")) {
        cerr << "Error: Not a mygit repository. Run 'mygit init' first.\n";
        return 1;
    }

    string message = "Default commit message";
    bool hasMessage = false;

    // Parse command line arguments for commit message
    for (int i = 2; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-m" && i + 1 < argc) {
            message = argv[++i];
            hasMessage = true;
        }
    }

    // If no message provided, use default or prompt
    if (!hasMessage) {
        cout << "No commit message provided, using default.\n";
    }

    string parentHash = readHEAD();
    
    // Create tree from index (staged files) or working directory
    string treeHash = createTreeFromIndex(); // Use staged files
    // string treeHash = createTreeFromWorkingDirectory(); // Alternative: use all files
    
    if (treeHash.empty()) {
        return 1;
    }

    // Create commit
    string commitHash = createCommit(message, treeHash, parentHash);
    
    if (commitHash.empty()) {
        cerr << "Error: Failed to create commit\n";
        return 1;
    }

    // Write to log file BEFORE updating HEAD (NEW)
    writeToLog(parentHash, commitHash, message);

    // Update HEAD and refs
    writeHEAD(commitHash);
    updateRef(commitHash);
    
    // Clear the index after successful commit
    clearIndex();
    
    // Display commit information (Git-like output)
    cout << commitHash << endl;
    
    return 0;
}