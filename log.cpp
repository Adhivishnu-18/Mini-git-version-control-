#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include "header.h"

namespace fs = std::filesystem;
using namespace std;

// Function to read the commit log and return commit details
vector<string> readCommitLog() {
    vector<string> commits;
    ifstream logFile(".mygit/logs/HEAD");
    if (!logFile) {
        cout << "No commits found (log file doesn't exist).\n";
        return commits;
    }

    string line;
    while (getline(logFile, line)) {
        if (!line.empty()) {
            commits.push_back(line);
        }
    }

    return commits;
}

void displayCommitLog() {
    vector<string> commits = readCommitLog();
    
    if (commits.empty()) {
        cout << "No commits found.\n";
        return;
    }

    // Display commits from latest to oldest (reverse order)
    for (auto it = commits.rbegin(); it != commits.rend(); ++it) {
        string line = *it;
        
        // Parse log format: "oldHash newHash committerInfo timestamp commit: message"
        istringstream ss(line);
        string oldHash, newHash, committerInfo, timestamp, commitPrefix, message;
        
        // Read the components
        ss >> oldHash >> newHash >> committerInfo >> timestamp >> commitPrefix;
        
        // Get the rest as message (after "commit: ")
        getline(ss, message);
        if (!message.empty() && message[0] == ' ') {
            message = message.substr(1); // Remove leading space
        }

        // Display commit information
        cout << "Commit: " << newHash << "\n";
        
        // Don't show parent if it's the initial commit (all zeros)
        if (oldHash != string(40, '0')) {
            cout << "Parent: " << oldHash << "\n";
        }
        
        cout << "Committer: " << committerInfo << "\n";
        cout << "Date: " << timestamp << "\n";
        cout << "Message: " << message << "\n";
        cout << "\n"; // Blank line between commits
    }
}

// Main function to handle the log command
bool handleLog(int argc, char* argv[]) {
    // Check if repository exists
    if (!fs::exists(".mygit")) {
        cerr << "Error: Not a mygit repository. Run 'mygit init' first.\n";
        return false;
    }
    
    displayCommitLog();
    return true;
}