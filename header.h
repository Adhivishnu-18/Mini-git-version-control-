#ifndef HEADER_H
#define HEADER_H

#include <string>
#include <filesystem>
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <zlib.h>

namespace fs = std::filesystem;
using namespace std;

// Structure definitions
struct FileStatus {
    string filePath;
    string status;
    string stagedHash;
    string workingHash;
};

struct TreeEntry {
    string mode;
    string type;
    string sha;
    string name;
};

// Structure for commit information
struct CommitInfo {
    string commitHash;
    string treeHash;
    string parentHash;
    string author;
    string committer;
    string timestamp;
    string message;
};

// Core Git operations
bool initialize();
bool hashObject(const string& filePath, bool writeFlag);
bool catFile(const string& hash, bool printContent, bool displaySize, bool displayType);
void add(const string& filename);
void addtoindex(const string& filePath, const string& hash);
string computeSHA1(const string& filePath);
string writeTree(const fs::path& dirPath);
bool isHidden(const fs::path& path);
void lsTree(const string& treeSHA, bool nameOnly);
int handleCommit(int argc, char* argv[]);
string computeSHA1FromString(const string& content);
string computeHashForFile(const string& filePath); // Added this declaration

// Command handlers
bool handleCheckout(int argc, char* argv[]);
bool handleStatus(int argc, char* argv[]);
bool handleLsTree(int argc, char* argv[]);
bool handleAdd(int argc, char* argv[]);
bool handleLog(int argc, char* argv[]);
bool handleReset(int argc, char* argv[]);
bool handleShow(int argc, char* argv[]);

// Status command functions
void displayStatus();
vector<FileStatus> generateStatus();
map<string, string> readIndex();
string getCurrentCommit();
map<string, string> getCommittedFiles();
void collectFilesFromTree(const string& treeSHA, const string& prefix, map<string, string>& files);
set<string> getWorkingDirectoryFiles();
void scanDirectory(const string& dirPath, const string& prefix, set<string>& files);
string computeWorkingFileHash(const string& filePath);

// Utility functions for object handling
string readObjectFile(const string& hash);
tuple<string, size_t, string> parseObject(const string& objectContent);
TreeEntry parseTreeEntry(const string& data, size_t& pos);
string decompressData(const string& compressedData);
string decompressData(const vector<unsigned char>& compressedData); // Overloaded version

// Cat-file alternative function signatures
void catFile(const string& flag, const string& hash);
void printFileContent(const string& hash);
void printFileSize(const string& hash);
void printFileType(const string& hash);

// Add/staging functions
bool addFileToStaging(const string& filePath);
bool isHiddenFile(const fs::path& path);

// HEAD and reference management
string readHEAD();
void writeHEAD(const string& commitHash);
bool objectExists(const string& sha);

// Tree operations
string getTreeSHAFromCommit(const string& commitSHA);
vector<TreeEntry> readTreeEntries(const string& treeSHA);
bool restoreTree(const string& treeSHA, const fs::path& currentPath);
void clearWorkingDirectory();

// Checkout operations
bool checkout(const string& commitSHA);

// Reset operations
bool reset(const vector<string>& args);
bool removeFromIndex(const string& filePath);
bool clearIndex(); // void return type to match commit.cpp
bool resetToCommit(const string& commitSHA);
bool resetFilesToHEAD(const vector<string>& filePaths);
map<string, string> getIndexedFiles();

// Show operations
bool show(const string& commitSHA);
void showCommit(const string& commitSHA);
void showHEAD();
void showTreeDiff(const string& oldTreeSHA, const string& newTreeSHA, const string& prefix);
CommitInfo parseCommitObject(const string& commitSHA);

// Log operations
bool handleLog(int argc, char* argv[]);

#endif