#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include "header.h"

namespace fs = std::filesystem;
using namespace std;

// Remove a specific file from the index
bool removeFromIndex(const string& filePath) {
    ifstream indexFile(".mygit/index");
    if (!indexFile) {
        cerr << "Error: No index file found\n";
        return false;
    }
    
    vector<string> indexLines;
    string line;
    bool found = false;
    
    // Read all lines except the one we want to remove
    while (getline(indexFile, line)) {
        if (line.empty()) continue;
        
        // Parse index format: "mode hash filename"
        istringstream iss(line);
        string mode, hash, filename;
        iss >> mode >> hash;
        
        // Get the rest as filename
        getline(iss, filename);
        if (!filename.empty() && filename[0] == ' ') {
            filename = filename.substr(1);
        }
        
        if (filename != filePath) {
            indexLines.push_back(line);
        } else {
            found = true;
        }
    }
    indexFile.close();
    
    if (!found) {
        cerr << "Error: '" << filePath << "' is not in the index\n";
        return false;
    }
    
    // Write back the modified index
    ofstream outFile(".mygit/index");
    if (!outFile) {
        cerr << "Error: Cannot write to index file\n";
        return false;
    }
    
    for (const string& indexLine : indexLines) {
        outFile << indexLine << "\n";
    }
    
    cout << "Unstaged '" << filePath << "'\n";
    return true;
}

// Reset to a specific commit (hard reset)
bool resetToCommit(const string& commitSHA) {
    // Validate commit exists
    if (!objectExists(commitSHA)) {
        cerr << "Error: Commit " << commitSHA << " does not exist\n";
        return false;
    }
    
    // Get tree from commit
    string treeSHA = getTreeSHAFromCommit(commitSHA);
    if (treeSHA.empty()) {
        cerr << "Error: Cannot extract tree from commit\n";
        return false;
    }
    
    cout << "Resetting to commit " << commitSHA << "\n";
    
    // Clear working directory
    clearWorkingDirectory();
    
    // Restore files from the commit's tree
    if (!restoreTree(treeSHA, ".")) {
        cerr << "Error: Failed to restore files from commit\n";
        return false;
    }
    
    // Clear the index (unstage everything)
    clearIndex();
    
    // Update HEAD to point to this commit
    writeHEAD(commitSHA);
    
    cout << "HEAD is now at " << commitSHA.substr(0, 8) << "\n";
    return true;
}

// Get files that are currently in the index
map<string, string> getIndexedFiles() {
    map<string, string> indexedFiles;
    
    ifstream indexFile(".mygit/index");
    if (!indexFile) {
        return indexedFiles;
    }
    
    string line;
    while (getline(indexFile, line)) {
        if (line.empty()) continue;
        
        istringstream iss(line);
        string mode, hash, filename;
        iss >> mode >> hash;
        
        getline(iss, filename);
        if (!filename.empty() && filename[0] == ' ') {
            filename = filename.substr(1);
        }
        
        if (!filename.empty()) {
            indexedFiles[filename] = hash;
        }
    }
    
    return indexedFiles;
}

// Reset specific files to HEAD (mixed reset for specific files)
bool resetFilesToHEAD(const vector<string>& filePaths) {
    string currentCommit = getCurrentCommit();
    if (currentCommit.empty()) {
        cerr << "Error: No commits found (cannot reset to HEAD)\n";
        return false;
    }
    
    // Get files from current commit
    map<string, string> committedFiles = getCommittedFiles();
    
    for (const string& filePath : filePaths) {
        // Check if file exists in current commit
        auto it = committedFiles.find(filePath);
        if (it == committedFiles.end()) {
            cerr << "Warning: '" << filePath << "' not found in HEAD commit\n";
            // Still remove from index if it's there
            removeFromIndex(filePath);
            continue;
        }
        
        // Remove from index first
        removeFromIndex(filePath);
        
        // Add back to index with the committed version
        string commitHash = it->second;
        addtoindex(filePath, commitHash);
        
        cout << "Reset '" << filePath << "' to HEAD\n";
    }
    
    return true;
}

// Main reset function that handles different reset types
bool reset(const vector<string>& args) {
    if (args.empty()) {
        // No arguments: unstage all files (git reset)
        return clearIndex();
    }
    
    // Check for --hard flag
    bool hardReset = false;
    vector<string> filePaths;
    string commitSHA;
    
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "--hard") {
            hardReset = true;
        } else if (args[i].length() == 40 && objectExists(args[i])) {
            // Looks like a commit SHA
            commitSHA = args[i];
        } else {
            // Assume it's a file path
            filePaths.push_back(args[i]);
        }
    }
    
    if (hardReset) {
        if (commitSHA.empty()) {
            commitSHA = getCurrentCommit();
            if (commitSHA.empty()) {
                cerr << "Error: No commits found\n";
                return false;
            }
        }
        return resetToCommit(commitSHA);
    }
    
    if (!commitSHA.empty() && filePaths.empty()) {
        // Soft/mixed reset to commit (just move HEAD and clear index)
        writeHEAD(commitSHA);
        clearIndex();
        cout << "Reset HEAD to " << commitSHA.substr(0, 8) << "\n";
        return true;
    }
    
    if (!filePaths.empty()) {
        // Reset specific files
        if (!commitSHA.empty()) {
            cerr << "Error: Cannot specify both commit and file paths without --hard\n";
            return false;
        }
        return resetFilesToHEAD(filePaths);
    }
    
    // Default: clear index
    return clearIndex();
}

// Command handler for main.cpp integration
bool handleReset(int argc, char* argv[]) {
    if (!fs::exists(".mygit")) {
        cerr << "Error: Not a mygit repository. Run 'mygit init' first.\n";
        return false;
    }
    
    vector<string> args;
    for (int i = 2; i < argc; i++) {
        args.push_back(argv[i]);
    }
    
    return reset(args);
}