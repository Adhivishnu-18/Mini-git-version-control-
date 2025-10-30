#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include "header.h"

namespace fs = std::filesystem;
using namespace std;

// Function to check if a file is hidden (same as in add.cpp)
bool isHiddenFile(const fs::path& path) {
    string filename = path.filename().string();
    return !filename.empty() && (filename[0] == '.' || filename == ".mygit");
}

// Read the index file and return staged files
map<string, string> readIndex() {
    map<string, string> stagedFiles;
    
    ifstream indexFile(".mygit/index");
    if (!indexFile) {
        return stagedFiles; // Empty map if no index file
    }
    
    string line;
    while (getline(indexFile, line)) {
        if (line.empty()) continue;
        
        // Parse index format: "mode hash filename" or "mode Blob hash filename"
        istringstream iss(line);
        string mode, type_or_hash, hash, filename;
        iss >> mode;
        iss >> type_or_hash;
        
        if (type_or_hash == "Blob") {
            // Format: "100644 Blob hash filename"
            iss >> hash;
            getline(iss, filename);
        } else {
            // Format: "100644 hash filename"  
            hash = type_or_hash;
            getline(iss, filename);
        }
        
        // Remove leading space from filename
        if (!filename.empty() && filename[0] == ' ') {
            filename = filename.substr(1);
        }
        
        if (!filename.empty()) {
            stagedFiles[filename] = hash;
        }
    }
    
    return stagedFiles;
}

// Get current HEAD commit hash
string getCurrentCommit() {
    ifstream headFile(".mygit/HEAD");
    if (!headFile) {
        return ""; // No HEAD file (no commits yet)
    }
    
    string headContent;
    getline(headFile, headContent);
    
    // If HEAD is empty or doesn't exist, there are no commits
    if (headContent.empty()) {
        return "";
    }
    
    return headContent;
}

// Get files from the last commit's tree (if it exists)
map<string, string> getCommittedFiles() {
    map<string, string> committedFiles;
    
    string currentCommit = getCurrentCommit();
    if (currentCommit.empty()) {
        return committedFiles; // No commits yet
    }
    
    // Read commit object to get tree SHA
    string objectContent = readObjectFile(currentCommit);
    if (objectContent.empty()) {
        return committedFiles;
    }
    
    auto [type, size, content] = parseObject(objectContent);
    if (type != "commit") {
        return committedFiles;
    }
    
    // Extract tree SHA from commit
    istringstream iss(content);
    string line;
    string treeSHA;
    while (getline(iss, line)) {
        if (line.find("tree ") == 0) {
            treeSHA = line.substr(5);
            break;
        }
    }
    
    if (treeSHA.empty()) {
        return committedFiles;
    }
    
    // Recursively collect all files from the tree
    collectFilesFromTree(treeSHA, "", committedFiles);
    
    return committedFiles;
}

// Recursively collect files from tree object
void collectFilesFromTree(const string& treeSHA, const string& prefix, map<string, string>& files) {
    vector<TreeEntry> entries = readTreeEntries(treeSHA);
    if (entries.empty()) {
        return;
    }
    
    for (const auto& entry : entries) {
        string fullPath = prefix.empty() ? entry.name : prefix + "/" + entry.name;
        
        if (entry.type == "blob") {
            files[fullPath] = entry.sha;
        } else if (entry.type == "tree") {
            collectFilesFromTree(entry.sha, fullPath, files);
        }
    }
}

// Get all files in working directory
set<string> getWorkingDirectoryFiles() {
    set<string> workingFiles;
    
    // Recursively scan working directory
    scanDirectory(".", "", workingFiles);
    
    return workingFiles;
}

// Recursively scan directory for files
void scanDirectory(const string& dirPath, const string& prefix, set<string>& files) {
    try {
        for (const auto& entry : fs::directory_iterator(dirPath)) {
            if (isHiddenFile(entry.path())) {
                continue; // Skip hidden files and .mygit
            }
            
            string relativePath = prefix.empty() ? 
                entry.path().filename().string() : 
                prefix + "/" + entry.path().filename().string();
            
            if (fs::is_regular_file(entry.status())) {
                files.insert(relativePath);
            } else if (fs::is_directory(entry.status())) {
                scanDirectory(entry.path().string(), relativePath, files);
            }
        }
    } catch (const fs::filesystem_error& e) {
        // Skip directories we can't read
    }
}

// Compute hash for a working directory file
string computeWorkingFileHash(const string& filePath) {
    if (!fs::exists(filePath)) {
        return "";
    }
    
    ifstream file(filePath, ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    ostringstream buffer;
    buffer << file.rdbuf();
    string fileContent = buffer.str();
    
    // Create blob format and compute hash
    string blobContent = "blob " + to_string(fileContent.length()) + '\0' + fileContent;
    return computeSHA1FromString(blobContent);
}

// Generate status report
vector<FileStatus> generateStatus() {
    vector<FileStatus> statusList;
    
    // Get file lists
    map<string, string> stagedFiles = readIndex();
    map<string, string> committedFiles = getCommittedFiles();
    set<string> workingFiles = getWorkingDirectoryFiles();
    
    // Collect all unique file paths
    set<string> allFiles;
    
    // Add all staged files
    for (const auto& pair : stagedFiles) {
        allFiles.insert(pair.first);
    }
    
    // Add all committed files
    for (const auto& pair : committedFiles) {
        allFiles.insert(pair.first);
    }
    
    // Add all working files
    for (const string& file : workingFiles) {
        allFiles.insert(file);
    }
    
    // Analyze each file
    for (const string& filePath : allFiles) {
        FileStatus status;
        status.filePath = filePath;
        
        bool isStaged = stagedFiles.find(filePath) != stagedFiles.end();
        bool isCommitted = committedFiles.find(filePath) != committedFiles.end();
        bool inWorkingDir = workingFiles.find(filePath) != workingFiles.end();
        
        if (isStaged) {
            status.stagedHash = stagedFiles[filePath];
        }
        
        if (inWorkingDir) {
            status.workingHash = computeWorkingFileHash(filePath);
        }
        
        // Determine file status based on combinations
        if (!isCommitted && isStaged) {
            // New file that's been staged
            if (inWorkingDir && status.stagedHash == status.workingHash) {
                status.status = "added";
            } else if (inWorkingDir && status.stagedHash != status.workingHash) {
                status.status = "added_modified";
            } else if (!inWorkingDir) {
                status.status = "added_deleted";
            }
        }
        else if (!isCommitted && !isStaged && inWorkingDir) {
            // New file, not staged
            status.status = "untracked";
        }
        else if (isCommitted && !isStaged && !inWorkingDir) {
            // File was in last commit but deleted and not staged for deletion
            status.status = "deleted_unstaged";
        }
        else if (isCommitted && isStaged && !inWorkingDir) {
            // File deletion is staged
            status.status = "deleted";
        }
        else if (isCommitted && isStaged && inWorkingDir) {
            // File exists in all three places - check for modifications
            string commitHash = committedFiles[filePath];
            if (status.stagedHash != commitHash) {
                // Staged version differs from committed version
                if (status.stagedHash == status.workingHash) {
                    status.status = "modified";
                } else {
                    status.status = "modified_modified";
                }
            } else {
                // Staged version same as committed, check working version
                if (status.stagedHash != status.workingHash) {
                    status.status = "modified_unstaged";
                }
            }
        }
        else if (isCommitted && !isStaged && inWorkingDir) {
            // File exists in commit and working dir, but not staged
            string commitHash = committedFiles[filePath];
            if (commitHash != status.workingHash) {
                status.status = "modified_unstaged";
            }
        }
        
        // Only add files with a status
        if (!status.status.empty()) {
            statusList.push_back(status);
        }
    }
    
    return statusList;
}

// Display status in a git-like format
void displayStatus() {
    if (!fs::exists(".mygit")) {
        cerr << "Error: Not a mygit repository. Run 'mygit init' first.\n";
        return;
    }
    
    vector<FileStatus> statusList = generateStatus();
    
    // Separate files by status category
    vector<FileStatus> staged;
    vector<FileStatus> modified;
    vector<FileStatus> untracked;
    
    for (const auto& fileStatus : statusList) {
        if (fileStatus.status == "added" || fileStatus.status == "modified" || 
            fileStatus.status == "deleted") {
            staged.push_back(fileStatus);
        }
        else if (fileStatus.status == "modified_unstaged" || 
                 fileStatus.status == "added_modified" ||
                 fileStatus.status == "modified_modified" ||
                 fileStatus.status == "deleted_unstaged") {
            modified.push_back(fileStatus);
        }
        else if (fileStatus.status == "untracked") {
            untracked.push_back(fileStatus);
        }
    }
    
    // Display current branch/commit info
    string currentCommit = getCurrentCommit();
    if (currentCommit.empty()) {
        cout << "On initial commit\n";
    } else {
        cout << "HEAD commit: " << currentCommit.substr(0, 8) << "...\n";
    }
    cout << "\n";
    
    // Display staged files
    if (!staged.empty()) {
        cout << "Changes to be committed:\n";
        cout << "  (use \"mygit reset <file>...\" to unstage)\n\n";
        
        for (const auto& file : staged) {
            if (file.status == "added") {
                cout << "\tnew file:   " << file.filePath << "\n";
            } else if (file.status == "modified") {
                cout << "\tmodified:   " << file.filePath << "\n";
            } else if (file.status == "deleted") {
                cout << "\tdeleted:    " << file.filePath << "\n";
            }
        }
        cout << "\n";
    }
    
    // Display modified files
    if (!modified.empty()) {
        cout << "Changes not staged for commit:\n";
        cout << "  (use \"mygit add <file>...\" to update what will be committed)\n";
        cout << "  (use \"mygit checkout -- <file>...\" to discard changes)\n\n";
        
        for (const auto& file : modified) {
            if (file.status == "modified_unstaged" || 
                file.status == "added_modified" || 
                file.status == "modified_modified") {
                cout << "\tmodified:   " << file.filePath << "\n";
            } else if (file.status == "deleted_unstaged") {
                cout << "\tdeleted:    " << file.filePath << "\n";
            }
        }
        cout << "\n";
    }
    
    // Display untracked files
    if (!untracked.empty()) {
        cout << "Untracked files:\n";
        cout << "  (use \"mygit add <file>...\" to include in what will be committed)\n\n";
        
        for (const auto& file : untracked) {
            cout << "\t" << file.filePath << "\n";
        }
        cout << "\n";
    }
    
    // Summary message
    if (staged.empty() && modified.empty() && untracked.empty()) {
        cout << "Nothing to commit, working tree clean\n";
    } else {
        if (staged.empty()) {
            if (!modified.empty() || !untracked.empty()) {
                cout << "No changes added to commit (use \"mygit add\" to track)\n";
            }
        }
    }
}

// Command handler for main.cpp integration
bool handleStatus(int argc, char* argv[]) {
    if (argc > 2) {
        cerr << "Usage: mygit status\n";
        return false;
    }
    
    displayStatus();
    return true;
}