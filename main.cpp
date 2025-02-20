#include <iostream>
#include <filesystem>
#include <vector>
#include <unordered_set>

// Macros for ANSI terminal output style
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define YELLOW1 "\033[38;5;220m"
#define GREY    "\033[38;5;248m"

namespace fs = std::filesystem;

// Filter for flags/options
std::unordered_set<std::string> excludeList;  // List for '-e'-flag
std::unordered_set<std::string> onlyList;     // List for '-o'-flag

// Function to display the directory as a tree structure with filter options
void printTree(const fs::path& root, const fs::path& current, const std::string& prefix = "") {
    std::vector<fs::path> entries;

    // Collect all files & folders within the root directory
    for (const auto& entry : fs::directory_iterator(current)) {
        std::string filename = entry.path().filename().string();

        // Use '-e' to ignore files/folders
        if (!excludeList.empty() && excludeList.count(filename)) continue;

        // Use '-o' to ignore files/folders
        bool isAllowed = false;
        for (const auto& allowed : onlyList) {
            if (filename == allowed || current.lexically_relative(root / allowed).string().find("..") == std::string::npos) {
                isAllowed = true;
                break;
            }
        }
        // If '-o' is set and the file is not in 'onlyList' or within an '-o' folder, ignore it
        if (!onlyList.empty() && !isAllowed) continue;          

        entries.push_back(entry.path());
    }

    // Sort for consistent order
    std::sort(entries.begin(), entries.end());

    // Iterate through collected entries and build tree structure
    for (size_t i = 0; i < entries.size(); ++i) {
        bool isLast = (i == entries.size() - 1);
        // std::cout << prefix << (isLast ? "└── " : "├── ") << entries[i].filename().string() << "\n";
        std::cout << " " << GREY << prefix << (isLast ? "└── " : "├── ") << RESET;
        if (fs::is_directory(entries[i])) {
            std::cout << BOLD << entries[i].filename().string() << RESET << "\n";
        } else {
            std::cout << entries[i].filename().string() << "\n";
        }
        

        // If directory then we call function recursively
        if (fs::is_directory(entries[i])) {
            printTree(root, entries[i], prefix + (isLast ? "    " : "│   "));
        }
    }
}

// Parsing CLI arguments
void parseArgs(int argc, char* argv[], fs::path& root) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // When using '-e'
        if (arg == "-e" && i + 1 < argc) {
            while (++i < argc && argv[i][0] != '-') { // Collect all files/folders to be ignored, stop if a new flag is encountered
                excludeList.insert(argv[i]);
            }
            --i; // Change index after loop
        }
        
        // When using '-o'
        else if (arg == "-o" && i + 1 < argc) {
            while (++i < argc && argv[i][0] != '-') { // Collect all files/folders to be displayed, stop if a new flag is encountered
                onlyList.insert(argv[i]);
            }
            --i; // Change index after loop
        }

        // If no flag is provided, it is the directory path.
        else if (root.empty()) {
            root = fs::absolute(argv[i]);
        }
    }
}

// Main program
int main(int argc, char* argv[]) {
    fs::path root;

    // Parse CLI arguments
    parseArgs(argc, argv, root);

    // General case use local directory path
    if (root.empty()) {
        root = fs::current_path();
    }

    std::cout << std::endl;

    // Display root directory
    std::cout << " " << BOLD << root.filename().string() << RESET << "\n";

    // Start recursive scan
    printTree(root, root);

    std::cout << std::endl;

    return 0;
}