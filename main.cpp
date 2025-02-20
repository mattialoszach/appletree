#include <iostream>
#include <filesystem>
#include <vector>
#include <unordered_set>

// Macros for ANSI terminal output style
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define GREY    "\033[38;5;248m"

namespace fs = std::filesystem;

// Filter for flags/options
std::unordered_set<std::string> excludeList;  // List for '-e'-flag
std::unordered_set<std::string> onlyList;     // List for '-o'-flag

// Help function
void showHelp() {
    std::cout << std::endl;
    std::cout << " " << BOLD << "✨ appletree - Directory Tree Viewer ✨\n" << RESET;
    std::cout << " Usage:\n";
    std::cout << "   appletree [path] [options]\n\n";

    std::cout << " Options:\n";
    std::cout << "   -e <name>    Exclude files or directories from the tree output\n";
    std::cout << "   -o <name>    Show only the specified files or directories\n\n";

    std::cout << " Examples:\n";
    std::cout << "   appletree                     Show the tree of the current directory\n";
    std::cout << "   appletree /path/to/folder     Show the tree of the specified directory\n";
    std::cout << "   appletree -e node_modules     Exclude 'node_modules' from the tree\n";
    std::cout << "   appletree -o src include      Show only 'src' and 'include' directories\n\n";

    std::cout << " For more details, visit:\n";
    std::cout << "   " << BOLD << "https://github.com/mattialosz/appletree" << RESET << "\n\n";
    std::cout << " 🍏🌳" << std::endl;
    std::cout << std::endl;
}

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
bool parseArgs(int argc, char* argv[], fs::path& root) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // Show help if the argument is 'help'
        if (arg == "help") {
            showHelp();
            return false; // Stop execution after help message
        }

        // When using '-e'
        if (arg == "-e") {
            if (i + 1 >= argc || argv[i + 1][0] == '-') {
                std::cerr << "Error: Missing argument after '-e'. Specify at least one file/folder to exclude.\n";
                return false;
            }
            while (++i < argc && argv[i][0] != '-') { // Collect all files/folders to be ignored, stop if a new flag is encountered
                excludeList.insert(argv[i]);
            }
            --i; // Change index after loop
        }
        
        // When using '-o'
        else if (arg == "-o") {
            if (i + 1 >= argc || argv[i + 1][0] == '-') {
                std::cerr << "Error: Missing argument after '-o'. Specify at least one file/folder to include.\n";
                return false;
            }
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

    // Ensure that both '-e' and '-o' were used correctly
    if (excludeList.empty() && onlyList.empty() && argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-e" || arg == "-o") {
                std::cerr << "Error: Missing argument after '" << arg << "'.\n";
                return false;
            }
        }
    }

    return true;
}

// Main program
int main(int argc, char* argv[]) {
    fs::path root;

    // Parse CLI arguments and check for errors
    if (!parseArgs(argc, argv, root)) {
        return 1; // Exit on error
    }

    // General case use local directory path
    if (root.empty()) {
        root = fs::current_path();
    }

    // Check if the given path exists
    if (!fs::exists(root)) {
        std::cerr << "Error: The specified path '" << root.string() << "' does not exist. Try again with a valid path.\n";
        return 1;
    }

    std::cout << std::endl;

    // Display root directory
    std::cout << " " << BOLD << root.filename().string() << RESET << "\n";

    // Start recursive scan
    printTree(root, root);

    std::cout << std::endl;

    return 0;
}