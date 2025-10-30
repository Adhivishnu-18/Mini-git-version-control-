#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <zlib.h>
#include "header.h"

using namespace std;
namespace fs = filesystem;

// Print file content (-p flag)
void printFileContent(const string& hash) {
    string objectContent = readObjectFile(hash);
    if (objectContent.empty()) return;

    auto [type, size, content] = parseObject(objectContent);
    
    if (type == "blob") {
        cout << content;
    } else if (type == "tree") {
        // For tree objects, we need to parse the binary format
        cout << "Tree object with " << content.size() << " bytes of data" << endl;
        // You might want to implement a more detailed tree parsing here
    } else {
        cout << content;
    }
}

// Print file size (-s flag)
void printFileSize(const string& hash) {
    string objectContent = readObjectFile(hash);
    if (objectContent.empty()) return;

    auto [type, size, content] = parseObject(objectContent);
    cout << size << endl;
}

// Print file type (-t flag)
void printFileType(const string& hash) {
    string objectContent = readObjectFile(hash);
    if (objectContent.empty()) return;

    auto [type, size, content] = parseObject(objectContent);
    cout << type << endl;
}

// Main cat-file function that handles different flags
void catFile(const string& flag, const string& hash) {
    if (flag == "-p") {
        printFileContent(hash);
    } else if (flag == "-s") {
        printFileSize(hash);
    } else if (flag == "-t") {
        printFileType(hash);
    } else {
        cerr << "Error: Unknown flag '" << flag << "'. Use -p, -s, or -t" << endl;
    }
}

// Alternative function signature for backward compatibility
bool catFile(const string& hash, bool printContent, bool displaySize, bool displayType) {
    string objectContent = readObjectFile(hash);
    if (objectContent.empty()) return false;

    auto [type, size, content] = parseObject(objectContent);

    if (displayType) {
        cout << "Type: " << type << endl;
    }

    if (displaySize) {
        cout << "Size: " << size << " bytes" << endl;
    }

    if (printContent) {
        if (type == "blob") {
            cout << content;
        } else {
            cout << content;
        }
    }

    return true;
}