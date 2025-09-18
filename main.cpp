#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_set>
#include <optional>
#include <algorithm>
#include <cctype>
#include <system_error>

// Macros for ANSI terminal output style
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define FG_GRAY "\033[37m"

namespace fs = std::filesystem;

// Filter for flags/options
std::unordered_set<std::string> excludeList;  // List for '-e'-flag
std::unordered_set<std::string> onlyList;     // List for '-o'-flag

// Depth limit (nullopt meaning unlimited)
std::optional<size_t> maxDepth;

// Show sizes
bool showSizes = false;

// Theme/Format
enum class Theme {classic, round};
Theme currentTheme = Theme::classic; // Default theme

// Helper Functions
std::string branch(bool isLast) {
    if (currentTheme == Theme::round) {
        return isLast ? "╰── " : "├── ";
    } else {
        return isLast ? "└── " : "├── ";
    }
}

std::string vertical(bool isLast) {
    if (currentTheme == Theme::round) {
        return isLast ? "    " : "│   ";
    } else {
        return isLast ? "    " : "│   ";
    }
}

std::pair<bool, std::uintmax_t> fileSizeSafe(const fs::path& p) {
    std::error_code ec;
    if (!fs::is_regular_file(p, ec) || ec) return {false, 0};
    auto s = fs::file_size(p, ec);
    if (ec) return {false, 0};
    return {true, s};
}

std::uintmax_t dirSizeRecursive(const fs::path& dir) {
    std::uintmax_t total = 0;
    std::error_code ec;
    for (auto& entry : fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied, ec)) {
        if (fs::is_regular_file(entry.path(), ec)) {
            auto s = fs::file_size(entry.path(), ec);
            if (!ec) total += s;
        }
    }

    return total;
}

std::string formatSize(std::uintmax_t bytes) {
    static const char* units[] = {"B","KiB","MiB","GiB","TiB","PiB","EiB"};
    double value = static_cast<double>(bytes);
    int idx = 0;
    while (value >= 1024.0 && idx < 6) {
        value /= 1024.0;
        ++idx;
    }

    char buf[32];
    if (value < 10.0 && idx > 0) {
        std::snprintf(buf, sizeof(buf), "%.1f %s", value, units[idx]);
    } else {
        std::snprintf(buf, sizeof(buf), "%.0f %s", value, units[idx]);
    }

    return std::string(buf);
}

// Help function
void showHelp() {
    std::cout << "\n";
    std::cout << " " << BOLD << "✨ appletree – Directory Tree Viewer ✨" << RESET << "\n\n";
    std::cout << BOLD << " Usage:" << RESET << "\n";
    std::cout << "   appletree [path] [options]\n\n";

    std::cout << BOLD << " Options:" << RESET << "\n";
    std::cout << "   -e <pattern>     Exclude files or directories from the output.\n";
    std::cout << "                      • If <pattern> is just a name (e.g. 'node_modules'),\n";
    std::cout << "                        all entries with that basename are excluded anywhere.\n";
    std::cout << "                      • If <pattern> contains '/' (e.g. 'src/main.cpp'),\n";
    std::cout << "                        only that relative path (or subtree) is excluded.\n";
    std::cout << "                      • Use '.' to exclude hidden files/dirs.\n\n";

    std::cout << "   -o <pattern>     Show only the specified files or directories.\n";
    std::cout << "                      • Works like -e, but in reverse: restricts output to\n";
    std::cout << "                        matching paths and their subtrees.\n";
    std::cout << "                      • Parent folders are shown automatically so you can\n";
    std::cout << "                        navigate to deep matches.\n\n";

    std::cout << "   -d <number>      Limit recursion depth.\n";
    std::cout << "                      • 0 = only show the root directory name.\n";
    std::cout << "                      • 1 = root + its direct children.\n";
    std::cout << "                      • n = root + n levels deep.\n";
    std::cout << "                      • If omitted, the full tree is shown.\n\n";

    std::cout << "   -s               Show file and directory sizes.\n";
    std::cout << "                      • Regular files: actual file size.\n";
    std::cout << "                      • Directories: recursive total size (like du -sh).\n\n";

    std::cout << "   -t <theme>       Change the drawing theme of the tree.\n";
    std::cout << "                      • 'classic' (default): ├── └── │\n";
    std::cout << "                      • 'round':             ├── ╰── │ (rounded corners)\n\n";

    std::cout << BOLD << " Examples:" << RESET << "\n";
    std::cout << "   appletree                        Show the tree of the current directory\n";
    std::cout << "   appletree /path/to/folder        Show the tree of the specified directory\n";
    std::cout << "   appletree -e node_modules        Exclude all 'node_modules' folders\n";
    std::cout << "   appletree -e src/main.cpp        Exclude only 'src/main.cpp'\n";
    std::cout << "   appletree -o src                 Show only the 'src' subtree\n";
    std::cout << "   appletree -o src/util/log.h      Show only that single file and its parents\n";
    std::cout << "   appletree -e . -d 2              Exclude hidden files and limit depth to 2\n";
    std::cout << "   appletree -s                     Show file & folder sizes (like du -sh)\n";
    std::cout << "   appletree -t round               Use round corners for the tree\n\n";

    std::cout << BOLD << " Notes:" << RESET << "\n";
    std::cout << " • Multiple -e or -o patterns can be given in sequence.\n";
    std::cout << " • Excludes take precedence over includes.\n";
    std::cout << " • Hidden files: use -e . to skip them globally.\n\n";

    std::cout << " For more details, visit:\n";
    std::cout << "   " << BOLD << "https://github.com/mattialosz/appletree" << RESET << "\n\n";
    std::cout << " \033[47;30m Created by @mattialoszach " << RESET << "\n";
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

        // Name + optional size suffix
        std::string name = entries[i].filename().string();
        std::string sizeSuffix;

        if (showSizes) {
            std::error_code ec2;
            if (fs::is_directory(entries[i], ec2)) {
                auto total = dirSizeRecursive(entries[i]);
                sizeSuffix = " (" + formatSize(total) + ")";
            } else {
                auto [hasSize, bytes] = fileSizeSafe(entries[i]);
                if (hasSize) {
                    sizeSuffix = " (" + formatSize(bytes) + ")";
                }
            }

        }

        std::cout << " " << prefix << branch(isLast) << RESET;
        if (fs::is_directory(entries[i])) {
            std::cout << BOLD << name << "/" << RESET << FG_GRAY << sizeSuffix << RESET <<"\n";
        } else {
            std::cout << name << FG_GRAY << sizeSuffix << RESET << "\n";
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

        // When using '-t'
        else if (arg == "-t") {
            if (i + 1 >= argc || argv[i + 1][0] == '-') {
                std::cerr << "Error: Missing argument after '-t'. Specify 'classic' or 'round'.\n";
                return false;
            }
            std::string theme = argv[++i];
            if (theme == "classic") currentTheme = Theme::classic;
            else if (theme == "round") currentTheme = Theme::round;
            else {
                std::cerr << "Error: Unknown theme '" << theme << "'. Use 'classic' or 'round'.\n";
                return false;
            }
        }
        
        // When using '-s'
        else if (arg == "-s") {
            showSizes = true;
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

    std::string sizeSuffix;
    if (showSizes) {
        std::error_code ec;
        if (fs::is_directory(root, ec)) {
            auto total = dirSizeRecursive(root);
            sizeSuffix = " (" + formatSize(total) + ")";
        } else {
            auto [hasSize, bytes] = fileSizeSafe(root);
            if (hasSize) {
                sizeSuffix = " (" + formatSize(bytes) + ")";
            }
        }
    }

    // Display root directory
    std::cout << " " << BOLD << root.filename().string() << "/" << RESET << FG_GRAY << sizeSuffix << RESET <<"\n";

    // Start recursive scan
    printTree(root, root);

    return 0;
}