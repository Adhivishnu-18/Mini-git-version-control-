#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <vector>
#include <string>
#include <zlib.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <cstring>
#include "header.h"

namespace fs = std::filesystem;
using namespace std;

// Function to decompress zlib-compressed data (string version)
string decompressData(const string& compressedData) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    
    if (inflateInit(&zs) != Z_OK) {
        cerr << "Error: Failed to initialize decompression\n";
        return "";
    }
    
    zs.next_in = (Bytef*)compressedData.data();
    zs.avail_in = compressedData.size();
    
    int ret;
    char outbuffer[32768];
    string outstring;
    
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);
        
        ret = inflate(&zs, 0);
        
        if (outstring.size() < zs.total_out) {
            outstring.append(outbuffer, zs.total_out - outstring.size());
        }
        
    } while (ret == Z_OK);
    
    inflateEnd(&zs);
    
    if (ret != Z_STREAM_END) {
        cerr << "Error: Decompression failed\n";
        return "";
    }
    
    return outstring;
}

// Function to decompress zlib-compressed data (vector version for lstree.cpp)
string decompressData(const vector<unsigned char>& compressedData) {
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    
    if (inflateInit(&stream) != Z_OK) {
        cerr << "Error: Failed to initialize zlib stream\n";
        return "";
    }
    
    // Set up the input
    stream.avail_in = compressedData.size();
    stream.next_in = const_cast<Bytef*>(compressedData.data());
    
    // Prepare output buffer
    vector<char> decompressed;
    const size_t CHUNK_SIZE = 16384;
    vector<unsigned char> outBuffer(CHUNK_SIZE);
    
    // Decompress chunks and append to result
    do {
        stream.avail_out = CHUNK_SIZE;
        stream.next_out = outBuffer.data();
        
        int ret = inflate(&stream, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&stream);
            cerr << "Error: Decompression failed\n";
            return "";
        }
        
        size_t decompressed_bytes = CHUNK_SIZE - stream.avail_out;
        decompressed.insert(decompressed.end(), 
                          outBuffer.begin(), 
                          outBuffer.begin() + decompressed_bytes);
        
    } while (stream.avail_out == 0);
    
    inflateEnd(&stream);
    return string(decompressed.begin(), decompressed.end());
}

// Function to read object file from .mygit/objects
string readObjectFile(const string& hash) {
    string objectPath = ".mygit/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
    
    if (!fs::exists(objectPath)) {
        cerr << "Error: Object file not found: " << objectPath << "\n";
        return "";
    }
    
    ifstream file(objectPath, ios::binary);
    if (!file) {
        cerr << "Error: Cannot read object file: " << objectPath << "\n";
        return "";
    }
    
    // Read the compressed data
    string compressedData((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
    
    // Decompress the data
    return decompressData(compressedData);
}

// Function to parse object (returns type, size, content)
tuple<string, size_t, string> parseObject(const string& objectData) {
    // Find the null terminator that separates header from content
    size_t nullPos = objectData.find('\0');
    if (nullPos == string::npos) {
        cerr << "Error: Invalid object format\n";
        return {"", 0, ""};
    }
    
    string header = objectData.substr(0, nullPos);
    string content = objectData.substr(nullPos + 1);
    
    // Parse header: "type size"
    istringstream headerStream(header);
    string type;
    size_t size;
    headerStream >> type >> size;
    
    return {type, size, content};
}

// Function to parse a single tree entry from binary content
TreeEntry parseTreeEntry(const string& content, size_t& pos) {
    TreeEntry entry;
    
    if (pos >= content.length()) {
        return entry; // Return empty entry if at end
    }
    
    // Find the space that separates mode from filename
    size_t spacePos = content.find(' ', pos);
    if (spacePos == string::npos) {
        return entry;
    }
    
    // Extract mode
    entry.mode = content.substr(pos, spacePos - pos);
    
    // Determine type based on mode
    if (entry.mode == "040000" || entry.mode == "40000") {
        entry.type = "tree";
    } else {
        entry.type = "blob";
    }
    
    // Find the null terminator that separates filename from SHA
    size_t nullPos = content.find('\0', spacePos + 1);
    if (nullPos == string::npos || nullPos + 20 > content.length()) {
        return entry;
    }
    
    // Extract filename
    entry.name = content.substr(spacePos + 1, nullPos - spacePos - 1);
    
    // Extract 20-byte binary SHA and convert to hex
    stringstream ss;
    for (int i = 0; i < 20; i++) {
        unsigned char byte = content[nullPos + 1 + i];
        ss << hex << setw(2) << setfill('0') << (int)byte;
    }
    entry.sha = ss.str();
    
    // Update position to next entry
    pos = nullPos + 21; // null + 20 bytes SHA
    
    return entry;
}

// Function to compute SHA1 hash from string
string computeSHA1FromString(const string& data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
    
    stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

// Function to check if object exists
bool objectExists(const string& sha) {
    string objectPath = ".mygit/objects/" + sha.substr(0, 2) + "/" + sha.substr(2);
    return fs::exists(objectPath);
}

// Function to read the HEAD reference
string readHEAD() {
    ifstream headFile(".mygit/HEAD");
    if (!headFile) return "";
    string headContent;
    getline(headFile, headContent);
    return headContent;
}

// Function to write the HEAD reference
void writeHEAD(const string& commitHash) {
    ofstream headFile(".mygit/HEAD");
    headFile << commitHash;
}

// Function to check if a path is hidden (used by multiple files)
bool isHidden(const fs::path& path) {
    string filename = path.filename().string();
    return !filename.empty() && (filename[0] == '.' || filename == ".mygit");
}

// Function to read tree entries from tree object (used by checkout, status, show)
vector<TreeEntry> readTreeEntries(const string& treeSHA) {
    vector<TreeEntry> entries;
    
    string objectContent = readObjectFile(treeSHA);
    if (objectContent.empty()) {
        cerr << "Error: Tree object not found or cannot be read\n";
        return entries;
    }
    
    auto [type, size, content] = parseObject(objectContent);
    if (type != "tree") {
        cerr << "Error: Object is not a tree\n";
        return entries;
    }
    
    // Parse tree entries from binary content
    size_t pos = 0;
    while (pos < content.length()) {
        TreeEntry entry = parseTreeEntry(content, pos);
        if (entry.sha.empty()) break;
        entries.push_back(entry);
    }
    
    return entries;
}

// Function to get tree SHA from commit object (used by checkout, reset, show)
string getTreeSHAFromCommit(const string& commitSHA) {
    string objectContent = readObjectFile(commitSHA);
    if (objectContent.empty()) {
        cerr << "Error: Commit object not found or cannot be read\n";
        return "";
    }

    auto [type, size, content] = parseObject(objectContent);
    if (type != "commit") {
        cerr << "Error: Object is not a commit\n";
        return "";
    }

    // Parse commit content to find tree line
    istringstream iss(content);
    string line;
    while (getline(iss, line)) {
        if (line.find("tree ") == 0) {
            return line.substr(5); // Extract SHA after "tree "
        }
    }
    
    cerr << "Error: No tree found in commit object\n";
    return "";
}