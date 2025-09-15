#include <iostream>
#include <filesystem>
#include <vector>
#include <unordered_set>
#include <optional>
#include <algorithm>
#include <cctype>

// Macros for ANSI terminal output style
#define RESET   "\033[0m"
#define BOLD    "\033[1m"

namespace fs = std::filesystem;

// Filter for flags/options
std::unordered_set<std::string> excludeList;  // List for '-e'-flag
std::unordered_set<std::string> onlyList;     // List for '-o'-flag

// Depth limit (nullopt meaning unlimited)
std::optional<size_t> maxDepth;

// Style/Format
enum class Style {classic, round};
Style currentStyle = Style::classic; // Default style

std::string branch(bool isLast) {
    if (currentStyle == Style::round) {
        return isLast ? "╰── " : "├── ";
    } else {
        return isLast ? "└── " : "├── ";
    }
}

std::string vertical(bool isLast) {
    if (currentStyle == Style::round) {
        return isLast ? "    " : "│   ";
    } else {
        return isLast ? "    " : "│   ";
    }
}

// Help function
void showHelp() {
    std::cout << std::endl;
    std::cout << " " << BOLD << "✨ appletree - Directory Tree Viewer ✨\n" << RESET;
    std::cout << " Usage:\n";
    std::cout << "   appletree [path] [options]\n\n";

    std::cout << " Options:\n";
    std::cout << "   -e <name>      Exclude files or directories from the tree output\n";
    std::cout << "   -e .           Exclude hidden files or directories from the tree output\n";
    std::cout << "   -o <name>      Show only the specified files or directories\n";
    std::cout << "   -d <number>    Limit tree/recursion depth. Default: unlimited\n\n";

    std::cout << " Styles:\n";
    std::cout << "   Available options ['classic' (default), 'round']\n";
    std::cout << "   -s <style>   Change output style/format\n\n";

    std::cout << " Examples:\n";
    std::cout << "   appletree                     Show the tree of the current directory\n";
    std::cout << "   appletree /path/to/folder     Show the tree of the specified directory\n";
    std::cout << "   appletree -e node_modules     Exclude 'node_modules' from the tree\n";
    std::cout << "   appletree -o src include      Show only 'src' and 'include' directories\n";
    std::cout << "   appletree -e . -d 2           Exclude hidden files & show tree of depth 2\n";
    std::cout << "   appletree -s round            Change the style (round corners)\n\n";

    std::cout << " For more details, visit:\n";
    std::cout << "   " << BOLD << "https://github.com/mattialosz/appletree" << RESET << "\n\n";
    std::cout << " \033[47;30m Created by @mattialoszach " << RESET << std::endl;
}

// Function to display the directory as a tree structure with filter options
void printTree(const fs::path& root, const fs::path& current, const std::string& prefix = "", size_t depth = 0) {
    // Respect depth limit: if maxDepth is set and we've reached it, stop recursion
    if (maxDepth.has_value() && depth >= maxDepth.value()) {
        return;
    }

    std::vector<fs::path> entries;

    // Collect all files & folders within the root directory
    std::error_code ec;
    for (fs::directory_iterator it(current, fs::directory_options::skip_permission_denied, ec);
         !ec && it != fs::directory_iterator(); it.increment(ec)) {

        const auto& entry = *it;
        const std::string filename = entry.path().filename().string();

        // Hidden via "-e ."
        if (!filename.empty() && excludeList.count(".") && filename[0] == '.') continue;

        // Relative Path regarding root (for -e/-o)
        std::string rel;
        {
            std::error_code ec2;
            auto canon = fs::weakly_canonical(entry.path(), ec2);
            if (ec2) continue; // skip
            rel = canon.lexically_relative(root).generic_string();
        }

        // Exclude (-e)
        bool isExcluded = false;
        if (!excludeList.empty()) {
            for (const auto& ex : excludeList) {
                if (ex == ".") continue;
                if (ex.find('/') != std::string::npos) {
                    if (rel == ex || rel.rfind(ex + "/", 0) == 0) { isExcluded = true; break; }
                } else {
                    if (filename == ex) { isExcluded = true; break; }
                }
            }
        }
        if (isExcluded) continue;

        // Only (-o)
        bool isAllowed = onlyList.empty();
        if (!onlyList.empty()) {
            for (const auto& allowed : onlyList) {
                if (rel == allowed || rel.rfind(allowed + "/", 0) == 0) { 
                    isAllowed = true; break; 
                }
                if (allowed.rfind(rel + "/", 0) == 0) { 
                    isAllowed = true; break; 
                }
            }
        }
        if (!isAllowed) continue;

        entries.push_back(entry.path());
    }

    // Sort for consistent order
    std::sort(entries.begin(), entries.end());

    // Iterate through collected entries and build tree structure
    for (size_t i = 0; i < entries.size(); ++i) {
        bool isLast = (i == entries.size() - 1);

        std::cout << " " << prefix << branch(isLast) << RESET;
        if (fs::is_directory(entries[i])) {
            std::cout << BOLD << entries[i].filename().string() << "/" << RESET << "\n";
        } else {
            std::cout << entries[i].filename().string() << "\n";
        }
        

        // If directory then we call function recursively
        if (fs::is_directory(entries[i])) {
            printTree(root, entries[i], prefix + vertical(isLast), depth + 1);
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

        // When using '-d'
        else if (arg == "-d") {
            if (i + 1 >= argc || argv[i + 1][0] == '-') {
                std::cerr << "Error: Missing argument after '-d'. Specify a non-negative integer.\n";
                return false;
            }
            std::string depthStr = argv[++i];

            bool isStrDigit = std::all_of(depthStr.begin(), depthStr.end(), 
                                            [](unsigned char c) { return std::isdigit(c); });
            if (depthStr.empty() || !isStrDigit) {
                std::cerr << "Error: Depth must be a non-negative integer (got '" << depthStr << "').\n";
                return false;
            }
            try {
                size_t d = std::stoul(depthStr);
                maxDepth = d;
            } catch(...) {
                std::cerr << "Error: Failed to parse depth value '" << depthStr << "'.\n";
                return false;
            }
        }

        // When using '-s'
        else if (arg == "-s") {
            if (i + 1 >= argc || argv[i + 1][0] == '-') {
                std::cerr << "Error: Missing argument after '-s'. Specify 'classic' or 'round'.\n";
                return false;
            }
            std::string style = argv[++i];
            if (style == "classic") currentStyle = Style::classic;
            else if (style == "round") currentStyle = Style::round;
            else {
                std::cerr << "Error: Unknown style '" << style << "'. Use 'classic' or 'round'.\n";
                return false;
            }
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
    std::cout << " " << BOLD << root.filename().string() << "/" << RESET << "\n";

    // Start recursive scan
    printTree(root, root);

    return 0;
}