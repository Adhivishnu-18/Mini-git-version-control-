#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <openssl/sha.h> // OpenSSL library for SHA-1 hash
#include <zlib.h> // Include zlib for compression
#include <vector>
#include <openssl/evp.h>
#include "header.h"

using namespace std; // Use the standard namespace

namespace fs = filesystem;

string computeSHA1(const string& fileContent) {
    string header = "blob " + to_string(fileContent.size()) + '\0';
    string blobData = header + fileContent;

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha1();
    EVP_DigestInit_ex(mdctx, md, nullptr);

    EVP_DigestUpdate(mdctx, blobData.data(), blobData.size());

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    EVP_DigestFinal_ex(mdctx, hash, &hash_len);
    EVP_MD_CTX_free(mdctx);

    ostringstream oss;
    oss << hex << setfill('0');
    for (unsigned int i = 0; i < hash_len; i++) {
        oss << setw(2) << static_cast<int>(hash[i]);
    }
    return oss.str();
}

bool writeBlob(const string& filePath, const string& hash) {
    string dirPath = ".mygit/objects/" + hash.substr(0, 2);
    string fileName = hash.substr(2); // no ".gz"

    if (!fs::exists(dirPath)) fs::create_directories(dirPath);

    ifstream srcFile(filePath, ios::binary);
    ostringstream outBuffer;
    outBuffer << srcFile.rdbuf();
    string fileContent = outBuffer.str();

    string blobData = "blob " + to_string(fileContent.size()) + '\0' + fileContent;

    uLongf compressedSize = compressBound(blobData.size());
    vector<unsigned char> compressedData(compressedSize);

    if (compress(compressedData.data(), &compressedSize,
                 reinterpret_cast<const Bytef*>(blobData.data()), blobData.size()) != Z_OK) {
        cerr << "Compression failed\n";
        return false;
    }

    ofstream destFile(dirPath + "/" + fileName, ios::binary);
    destFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedSize);
    return true;
}

bool hashObject(const string& filePath, bool writeFlag) {
    ifstream srcFile(filePath, ios::binary);
    ostringstream buffer;
    buffer << srcFile.rdbuf();
    string content = buffer.str();

    string hash = computeSHA1(content);
    cout << "SHA-1: " << hash << "\n";

    if (writeFlag) {
        if (writeBlob(filePath, hash)) {
            cout << "Blob object written to: .mygit/objects/"
                 << hash.substr(0, 2) << "/" << hash.substr(2) << "\n";
        }
    }
    return true;
}