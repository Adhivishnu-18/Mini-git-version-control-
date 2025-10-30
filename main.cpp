#include <iostream>
#include <bits/stdc++.h>
#include "header.h"
#include <filesystem>

#include <sstream>
using namespace std;
namespace fs = filesystem;




bool isValidSHA1(const string& sha) {
    // Check if the SHA is exactly 40 characters long and is hexadecimal
    return sha.size() == 40 && regex_match(sha, regex("^[a-fA-F0-9]{40}$"));
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Error: No command provided.\n";
        return 1;
    }

    const string command = argv[1];
    if (command == "init") {
        if (argc != 2) {
            cerr << "Usage: .mygit init\n";
            return 1;
        }

        
        bool success = initialize();
        if (!success) {
            cerr << "Error: Failed to initialize the git directory or the git is already initialized./\n";
        } else {
            cout << "Git directory created successfully\n";
        }
    }
   
    else if (command == "hash-object") {
        if (argc < 3) {
            cerr << "Usage: .mygit hash-object [-w] <file>\n";
            return 1;
        }

        bool writeFlag = false;
        string filePath;

        if (argc == 4 && string(argv[2]) == "-w") {
            writeFlag = true;
            filePath = argv[3];
        } else {
            filePath = argv[2];
        }

        if (!hashObject(filePath, writeFlag)) {
            cerr << "Error: Failed to execute hash-object command\n";
            return 1;
        }
    }
    else if (command == "cat-file") {
        if (argc < 4) {
            cerr << "Usage: .mygit cat-file <options> <SHA>\n";
            return 1;
        }

        bool printContent = false, showSize = false, showType = false;
        string sha;

        for (int i = 2; i < argc - 1; ++i) {
            string option = argv[i];
            if (option == "-p") printContent = true;
            else if (option == "-s") showSize = true;
            else if (option == "-t") showType = true;
        }

        sha = argv[argc - 1];

        if (!isValidSHA1(sha)) {
            cerr << "Error: Invalid SHA. Must be 40 hexadecimal characters.\n";
            return 1;
        }

        if (!catFile(sha, printContent, showSize, showType)) {
            cerr << "Error: Failed to execute cat-file command\n";
            return 1;
        }
    } 
     else if (command == "add") 
    {
         if (argc < 3) {
            cerr << "Usage:./mygit add files\n";
            return 1;
        }
        for(int i=2;i<argc;i++)
        {
            add(argv[i]);
        }
    }
    else if (command == "write-tree") 
   {
    if (argc != 2) {
        cerr << "Usage: ./mygit write-tree\n";
        return 1;
    } 
    else {
        fs::path rootDir = fs::current_path();
        cout << "Creating tree structure for: " << rootDir << '\n';

        
        string rootTreeHash = writeTree(rootDir);
        cout << "Root tree hash: " << rootTreeHash << '\n';
    }
  }
   else if (command == "ls-tree") 
   {
   if (argc > 4) {
        cerr << "Error: Too many arguments. Usage: ./mygit ls-tree [--name-only] <tree_sha>" << endl;
        return 1;
    }

    if (argc < 3) {
        cerr << "Error: Missing arguments. Usage: ./mygit ls-tree [--name-only] <tree_sha>" << endl;
        return 1;
    }

    bool nameOnly = false;
    string treeSHA; 
    if (string(argv[2]) == "--name-only") {
        nameOnly = true;
        treeSHA = argv[3];
    } else {
        treeSHA = argv[2];
    }
    lsTree(treeSHA, nameOnly);
  }
  else  if (command=="commit") {
        if(argc<2||argc >4)
        {
        cerr << "Usage: " << argv[0] << " commit [-m message]" << endl;
        return 1;
        }
       return handleCommit(argc, argv); 
    }

    else if (command=="log")
    {
        if(argc!=2)
        {
            cerr<<"Usage: "<<argv[0]<<" "<<"log"<<endl;
        }
        return handleLog(argc, argv);
    }


    else if (command == "status") {
        if (argc != 2) {
            cerr << "Usage: " << argv[0] << " status\n";
            return 1;
        }
        
        if (!handleStatus(argc, argv)) {
            cerr << "Error: Failed to execute status command\n";
            return 1;
        }
    }


   else if (command == "checkout") {
        if(argc < 3) {
            cout << "Usage: ./mygit checkout <commit-sha>\n";
            return 1;
        }
        
        string commitSha = argv[2];
        if (!isValidSHA1(commitSha)) {
            cerr << "Error: Invalid commit SHA format.\n";
            return 1;
        }
        
        if (!checkout(commitSha)) {  // Now matches function name
            return 1;
        }
    }

    else if (command == "reset") {
    if (!handleReset(argc, argv)) {
        cerr << "Error: Failed to execute reset command\n";
        return 1;
    }
}

else if (command == "show") {
    if (!handleShow(argc, argv)) {
        cerr << "Error: Failed to execute show command\n";
        return 1;
    }
}

// Also add help command for better user experience:
else if (command == "help" || command == "--help") {
    cout << "MyGit - A simple Git implementation\n\n";
    cout << "Available commands:\n";
    cout << "  init                     - Initialize a new repository\n";
    cout << "  add <file>              - Add file to staging area\n";
    cout << "  commit [-m message]     - Create a new commit\n";
    cout << "  status                  - Show working tree status\n";
    cout << "  log                     - Show commit history\n";
    cout << "  show [commit-sha]       - Show commit details and diff\n";
    cout << "  checkout <commit-sha>   - Switch to a commit\n";
    cout << "  reset [options]         - Reset changes\n";
    cout << "    reset                 - Unstage all files\n";
    cout << "    reset <file>          - Unstage specific file\n";
    cout << "    reset --hard <sha>    - Reset to commit (destructive)\n";
    cout << "  hash-object [-w] <file> - Create object from file\n";
    cout << "  cat-file <options> <sha>- Show object contents\n";
    cout << "  write-tree              - Create tree from index\n";
    cout << "  ls-tree [--name-only] <tree-sha> - List tree contents\n";
    cout << "\nFor more information on a specific command, try: mygit <command> --help\n";
}
    
     else {
        cerr << "Error: Unknown command\n";
        return 1;
    }
    

    return 0;
}
