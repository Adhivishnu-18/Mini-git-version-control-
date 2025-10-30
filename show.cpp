#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <map>
#include <iomanip>
#include "header.h"

namespace fs = std::filesystem;
using namespace std;

// Parse commit object and extract information
CommitInfo parseCommitObject(const string& commitSHA) {
    CommitInfo info;
    info.commitHash = commitSHA;
    
    string objectContent = readObjectFile(commitSHA);
    if (objectContent.empty()) {
        return info;
    }
    
    auto [type, size, content] = parseObject(objectContent);
    if (type != "commit") {
        cerr << "Error: Object is not a commit\n";
        return info;
    }
    
    // Parse commit content line by line
    istringstream iss(content);
    string line;
    bool inMessage = false;
    
    while (getline(iss, line)) {
        if (inMessage) {
            if (!info.message.empty()) info.message += "\n";
            info.message += line;
            continue;
        }
        
        if (line.empty()) {
            inMessage = true;
            continue;
        }
        
        if (line.find("tree ") == 0) {
            info.treeHash = line.substr(5);
        } else if (line.find("parent ") == 0) {
            info.parentHash = line.substr(7);
        } else if (line.find("author ") == 0) {
            info.author = line.substr(7);
        } else if (line.find("committer ") == 0) {
            info.committer = line.substr(10);
            // Extract timestamp from committer line
            size_t lastSpace = info.committer.find_last_of(' ');
            if (lastSpace != string::npos) {
                size_t secondLastSpace = info.committer.find_last_of(' ', lastSpace - 1);
                if (secondLastSpace != string::npos) {
                    info.timestamp = info.committer.substr(secondLastSpace + 1);
                    info.committer = info.committer.substr(0, secondLastSpace);
                }
            }
        }
    }
    
    return info;
}

// Compare two trees and show differences
    void showTreeDiff(const string& oldTreeSHA, const string& newTreeSHA, const string& prefix) {
    // Get entries from both trees
    map<string, TreeEntry> oldEntries, newEntries;
    
    if (!oldTreeSHA.empty()) {
        vector<TreeEntry> oldTreeEntries = readTreeEntries(oldTreeSHA);
        for (const auto& entry : oldTreeEntries) {
            oldEntries[entry.name] = entry;
        }
    }
    
    if (!newTreeSHA.empty()) {
        vector<TreeEntry> newTreeEntries = readTreeEntries(newTreeSHA);
        for (const auto& entry : newTreeEntries) {
            newEntries[entry.name] = entry;
        }
    }
    
    // Get all unique file/directory names
    set<string> allNames;
    for (const auto& pair : oldEntries) allNames.insert(pair.first);
    for (const auto& pair : newEntries) allNames.insert(pair.first);
    
    for (const string& name : allNames) {
        string fullPath = prefix.empty() ? name : prefix + "/" + name;
        
        bool inOld = oldEntries.find(name) != oldEntries.end();
        bool inNew = newEntries.find(name) != newEntries.end();
        
        if (!inOld && inNew) {
            // New file/directory
            TreeEntry& entry = newEntries[name];
            if (entry.type == "blob") {
                cout << "diff --git a/" << fullPath << " b/" << fullPath << "\n";
                cout << "new file mode " << entry.mode << "\n";
                cout << "index 0000000.." << entry.sha.substr(0, 7) << "\n";
                cout << "--- /dev/null\n";
                cout << "+++ b/" << fullPath << "\n";
                
                // Show file content with + prefix
                string objectContent = readObjectFile(entry.sha);
                if (!objectContent.empty()) {
                    auto [type, size, content] = parseObject(objectContent);
                    if (type == "blob") {
                        istringstream contentStream(content);
                        string line;
                        while (getline(contentStream, line)) {
                            cout << "+" << line << "\n";
                        }
                    }
                }
            } else {
                // Recursively show new directory
                showTreeDiff("", entry.sha, fullPath);
            }
        } else if (inOld && !inNew) {
            // Deleted file/directory
            TreeEntry& entry = oldEntries[name];
            if (entry.type == "blob") {
                cout << "diff --git a/" << fullPath << " b/" << fullPath << "\n";
                cout << "deleted file mode " << entry.mode << "\n";
                cout << "index " << entry.sha.substr(0, 7) << "..0000000\n";
                cout << "--- a/" << fullPath << "\n";
                cout << "+++ /dev/null\n";
                
                // Show file content with - prefix
                string objectContent = readObjectFile(entry.sha);
                if (!objectContent.empty()) {
                    auto [type, size, content] = parseObject(objectContent);
                    if (type == "blob") {
                        istringstream contentStream(content);
                        string line;
                        while (getline(contentStream, line)) {
                            cout << "-" << line << "\n";
                        }
                    }
                }
            } else {
                // Recursively show deleted directory
                showTreeDiff(entry.sha, "", fullPath);
            }
        } else if (inOld && inNew) {
            // Potentially modified file/directory
            TreeEntry& oldEntry = oldEntries[name];
            TreeEntry& newEntry = newEntries[name];
            
            if (oldEntry.sha != newEntry.sha) {
                if (oldEntry.type == "blob" && newEntry.type == "blob") {
                    // Modified file
                    cout << "diff --git a/" << fullPath << " b/" << fullPath << "\n";
                    cout << "index " << oldEntry.sha.substr(0, 7) << ".." << newEntry.sha.substr(0, 7) << " " << newEntry.mode << "\n";
                    cout << "--- a/" << fullPath << "\n";
                    cout << "+++ b/" << fullPath << "\n";
                    
                    // Simple diff: show old content with -, new content with +
                    string oldContent, newContent;
                    
                    string oldObjectContent = readObjectFile(oldEntry.sha);
                    if (!oldObjectContent.empty()) {
                        auto [type, size, content] = parseObject(oldObjectContent);
                        if (type == "blob") oldContent = content;
                    }
                    
                    string newObjectContent = readObjectFile(newEntry.sha);
                    if (!newObjectContent.empty()) {
                        auto [type, size, content] = parseObject(newObjectContent);
                        if (type == "blob") newContent = content;
                    }
                    
                    // Show simple before/after diff
                    if (!oldContent.empty()) {
                        istringstream oldStream(oldContent);
                        string line;
                        while (getline(oldStream, line)) {
                            cout << "-" << line << "\n";
                        }
                    }
                    
                    if (!newContent.empty()) {
                        istringstream newStream(newContent);
                        string line;
                        while (getline(newStream, line)) {
                            cout << "+" << line << "\n";
                        }
                    }
                } else if (oldEntry.type == "tree" && newEntry.type == "tree") {
                    // Modified directory - recurse
                    showTreeDiff(oldEntry.sha, newEntry.sha, fullPath);
                }
            }
        }
    }
    }

// Show commit information and diff
void showCommit(const string& commitSHA) {
    // Parse commit object
    CommitInfo info = parseCommitObject(commitSHA);
    if (info.commitHash.empty()) {
        cerr << "Error: Cannot parse commit " << commitSHA << "\n";
        return;
    }
    
    // Display commit information
    cout << "commit " << info.commitHash << "\n";
    
    if (!info.author.empty()) {
        cout << "Author: " << info.author << "\n";
    }
    
    if (!info.committer.empty()) {
        cout << "Date: " << info.committer;
        if (!info.timestamp.empty()) {
            cout << " " << info.timestamp;
        }
        cout << "\n";
    }
    
    cout << "\n";
    
    // Display commit message with indentation
    if (!info.message.empty()) {
        istringstream messageStream(info.message);
        string line;
        while (getline(messageStream, line)) {
            cout << "    " << line << "\n";
        }
    }
    
    cout << "\n";
    
    // Show diff
    string parentTreeSHA;
    if (!info.parentHash.empty()) {
        // Get parent's tree
        CommitInfo parentInfo = parseCommitObject(info.parentHash);
        if (!parentInfo.treeHash.empty()) {
            parentTreeSHA = parentInfo.treeHash;
        }
    }
    
    // Show differences between parent tree and current tree
    showTreeDiff(parentTreeSHA, info.treeHash,"");
}

// Show the most recent commit if no SHA is provided
void showHEAD() {
    string currentCommit = getCurrentCommit();
    if (currentCommit.empty()) {
        cerr << "Error: No commits found\n";
        return;
    }
    
    showCommit(currentCommit);
}

// Main show function
bool show(const string& commitSHA) {
    if (!fs::exists(".mygit")) {
        cerr << "Error: Not a mygit repository. Run 'mygit init' first.\n";
        return false;
    }
    
    if (commitSHA.empty()) {
        showHEAD();
    } else {
        // Validate commit SHA
        if (commitSHA.length() != 40 || !objectExists(commitSHA)) {
            cerr << "Error: Invalid or non-existent commit SHA\n";
            return false;
        }
        showCommit(commitSHA);
    }
    
    return true;
}

// Command handler for main.cpp integration
bool handleShow(int argc, char* argv[]) {
    string commitSHA;
    
    if (argc > 3) {
        cerr << "Usage: mygit show [commit-sha]\n";
        return false;
    }
    
    if (argc == 3) {
        commitSHA = argv[2];
    }
    
    return show(commitSHA);
}