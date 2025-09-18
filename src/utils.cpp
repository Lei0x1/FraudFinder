#include "utils.h"

namespace Utils {

void ClearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void SetColor(int color) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color + 8); // Add intensity
#else
    switch (color) {
        case RED: std::cout << "\033[1;31m"; break;
        case GREEN: std::cout << "\033[1;32m"; break;
        case YELLOW: std::cout << "\033[1;33m"; break;
        case BLUE: std::cout << "\033[1;34m"; break;
        case MAGENTA: std::cout << "\033[1;35m"; break;
        case CYAN: std::cout << "\033[1;36m"; break;
        case WHITE: std::cout << "\033[1;37m"; break;
        default: std::cout << "\033[0m"; break;
    }
#endif
}

void ResetColor() {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 7); // Default white
#else
    std::cout << "\033[0m";
#endif
}

std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

std::string ToLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool IsNumber(const std::string& str) {
    if (str.empty()) return false;
    
    size_t start = 0;
    if (str[0] == '-') start = 1;
    
    for (size_t i = start; i < str.length(); ++i) {
        if (!std::isdigit(str[i])) return false;
    }
    
    return true;
}

std::chrono::system_clock::time_point Now() {
    return std::chrono::system_clock::now();
}

// File utilities
bool FileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

// Text formatting utilities
std::string CenterText(const std::string& text, int width) {
    if (text.length() >= static_cast<size_t>(width)) return text;
    
    int padding = (width - static_cast<int>(text.length())) / 2;
    return std::string(padding, ' ') + text + std::string(width - padding - static_cast<int>(text.length()), ' ');
}

std::string PadLeft(const std::string& str, int width, char padChar) {
    if (str.length() >= static_cast<size_t>(width)) return str;
    return std::string(width - str.length(), padChar) + str;
}

std::string PadRight(const std::string& str, size_t length, char padChar) {
    if (str.size() >= length) return str;
    return str + std::string(length - str.size(), padChar);
}

std::string Truncate(const std::string& str, int maxLength, const std::string& suffix) {
    if (str.length() <= static_cast<size_t>(maxLength)) return str;
    return str.substr(0, maxLength - suffix.length()) + suffix;
}

std::string FormatNumber(int number) {
    std::string numStr = std::to_string(number);
    std::string formatted;
    
    int count = 0;
    for (int i = numStr.length() - 1; i >= 0; --i) {
        if (count == 3) {
            formatted = "," + formatted;
            count = 0;
        }
        formatted = numStr[i] + formatted;
        count++;
    }
    
    return formatted;
}

std::string FormatPercentage(double value, int precision) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value << "%";
    return ss.str();
}

std::string FormatFileSize(long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << size << " " << units[unitIndex];
    return ss.str();
}

// UI/Console Output
void PrintLine(char character, int length) {
    std::cout << std::string(length, character) << "\n";
}

void PrintBox(const std::string& content, int width) {
    PrintLine('=', width);
    std::cout << "|" << CenterText(content, width - 2) << "|\n";
    PrintLine('=', width);
}

void PrintHeader(const std::string& title) {
    SetColor(CYAN);
    PrintBox(title, 80);
    ResetColor();
    std::cout << "\n";
}

void PrintSubHeader(const std::string& subtitle) {
    SetColor(YELLOW);
    std::cout << "=== " << subtitle << " ===\n";
    ResetColor();
}

void PrintSuccess(const std::string& message) {
    SetColor(GREEN);
    std::cout << "✓ " << message << "\n";
    ResetColor();
}

void PrintError(const std::string& message) {
    SetColor(RED);
    std::cout << "✗ " << message << "\n";
    ResetColor();
}

void PrintWarning(const std::string& message) {
    SetColor(YELLOW);
    std::cout << "⚠ " << message << "\n";
    ResetColor();
}

void PrintInfo(const std::string& message) {
    SetColor(BLUE);
    std::cout << "ℹ " << message << "\n";
    ResetColor();
}

void PrintProgressBar(int current, int total, int width) {
    if (total <= 0) return;
    
    double progress = static_cast<double>(current) / total;
    int filled = static_cast<int>(progress * width);
    
    std::cout << "[";
    SetColor(GREEN);
    for (int i = 0; i < filled; ++i) {
        std::cout << "█";
    }
    ResetColor();
    
    for (int i = filled; i < width; ++i) {
        std::cout << "░";
    }
    
    std::cout << "] " << FormatPercentage(progress * 100) 
              << " (" << current << "/" << total << ")";
}

void PrintTable(const std::vector<std::vector<std::string>>& data, 
                const std::vector<std::string>& headers,
                const std::vector<int>& columnWidths) {
    if (data.empty() || headers.empty() || columnWidths.empty()) return;
    
    auto printRow = [&](const std::vector<std::string>& row, bool isHeader = false) {
        std::cout << "│";
        for (size_t i = 0; i < row.size() && i < columnWidths.size(); ++i) {
            if (isHeader) SetColor(CYAN);
            std::cout << " " << Utils::PadRight(Truncate(row[i], columnWidths[i] - 2), columnWidths[i] - 2) << " ";
            if (isHeader) ResetColor();
            std::cout << "│";
        }
        std::cout << "\n";
    };
    
    auto printSeparator = [&](char left, char middle, char right, char fill) {
        std::cout << left;
        for (size_t i = 0; i < columnWidths.size(); ++i) {
            std::cout << std::string(columnWidths[i], fill);
            if (i < columnWidths.size() - 1) std::cout << middle;
        }
        std::cout << right << "\n";
    };
    
    // Top border
    printSeparator('┌', '┬', '┐', '─');
    
    // Header
    printRow(headers, true);
    
    // Header separator
    printSeparator('├', '┼', '┤', '─');
    
    // Data rows
    for (const auto& row : data) {
        printRow(row);
    }
    
    // Bottom border
    printSeparator('└', '┴', '┘', '─');
}

void PrintSpace() {
    std::cout << "\n";
}

void AnimateText(const std::string& text, int delayMs) {
    for (char c : text) {
        std::cout << c << std::flush;
#ifdef _WIN32
        Sleep(delayMs);
#else
        usleep(delayMs * 1000);
#endif
    }
    std::cout << std::endl;
}

void ShowSpinner(int durationMs) {
    const char* frames[] = {"|", "/", "-", "\\"};
    const int frameCount = 4;
    const int frameDelay = 100;
    
    int totalFrames = durationMs / frameDelay;
    
    for (int i = 0; i < totalFrames; ++i) {
        std::cout << "\r" << frames[i % frameCount] << " Processing..." << std::flush;
#ifdef _WIN32
        Sleep(frameDelay);
#else
        usleep(frameDelay * 1000);
#endif
    }
    
    std::cout << "\r" << std::string(20, ' ') << "\r" << std::flush;
}

// Time and Date
std::string GetCurrentDateTime() {
    return Utils::TimeToString(Now());
}

std::string TimeToString(const std::chrono::system_clock::time_point& tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_struct;
#ifdef _WIN32
    localtime_s(&tm_struct, &t);
#else
    localtime_r(&t, &tm_struct);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_struct, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string GetDateOnly() {
    auto time_t = std::chrono::system_clock::to_time_t(Now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d");
    return ss.str();
}

std::string GetTimeOnly() {
    auto time_t = std::chrono::system_clock::to_time_t(Now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    return ss.str();
}

// Validation
bool ValidateEmail(const std::string& email) {
    const std::regex pattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, pattern);
}

bool ValidatePhoneNumber(const std::string& phone) {
    // Basic phone validation - digits, spaces, dashes, parentheses, plus
    const std::regex pattern(R"(^[\+]?[0-9\s\-\(\)]{7,15}$)");
    return std::regex_match(phone, pattern);
}

bool ValidateUrl(const std::string& url) {
    const std::regex pattern(R"(^https?://[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}(/.*)?$)");
    return std::regex_match(url, pattern);
}

// Input/Output handling
std::string SanitizeInput(const std::string& input) {
    std::string sanitized = input;
    
    // Remove control characters except tab and newline
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(),
        [](char c) { return std::iscntrl(c) && c != '\t' && c != '\n'; }), 
        sanitized.end());
    
    return Trim(sanitized);
}

void LogMessage(const std::string& message, const std::string& level) {
    std::string timestamp = GetCurrentDateTime();
    std::string logEntry = "[" + timestamp + "] [" + level + "] " + message;
    
    // Also write to log file if needed
    std::ofstream logFile("fraudfinder.log", std::ios::app);
    if (logFile.is_open()) {
        logFile << logEntry << std::endl;
        logFile.close();
    }
    
    // Color code based on level
    if (level == "ERROR") {
        PrintError(message);
    } else if (level == "WARNING") {
        PrintWarning(message);
    } else if (level == "SUCCESS") {
        PrintSuccess(message);
    } else {
        PrintInfo(message);
    }
}

// Enhanced CreateDirectoryIfNotExists with error checking
bool CreateDirectoryIfNotExists(const std::string& path) {
    if (DirectoryExists(path)) return true;
    
#ifdef _WIN32
    return CreateDirectoryA(path.c_str(), nullptr) != 0;
#else
    return mkdir(path.c_str(), 0755) == 0;
#endif
}

std::string GetExecutablePath() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    std::string fullPath(buffer);
    return fullPath.substr(0, fullPath.find_last_of("\\/"));
#else
    char buffer[1024];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        std::string fullPath(buffer);
        return fullPath.substr(0, fullPath.find_last_of('/'));
    }
    return "./";
#endif
}

void PauseExecution(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

bool CopyFile(const std::string& source, const std::string& destination) {
    std::ifstream src(source, std::ios::binary);
    if (!src.is_open()) return false;
    
    std::ofstream dst(destination, std::ios::binary);
    if (!dst.is_open()) return false;
    
    dst << src.rdbuf();
    return src.good() && dst.good();
}

std::string GetFileExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) return "";
    
    std::string ext = filename.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool IsImageFile(const std::string& filename) {
    std::vector<std::string> imageExtensions = {
        "jpg", "jpeg", "png", "gif", "bmp", "tiff", "tif", "webp", "ico", "svg"
    };
    
    std::string ext = GetFileExtension(filename);
    return std::find(imageExtensions.begin(), imageExtensions.end(), ext) != imageExtensions.end();
}

long long GetFileSize(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return -1;
    
    return file.tellg();
}

bool IsValidImageSize(const std::string& filename, long long maxSizeMB) {
    long long size = GetFileSize(filename);
    if (size == -1) return false;
    
    long long maxSizeBytes = maxSizeMB * 1024 * 1024;
    return size <= maxSizeBytes;
}

// Directory operations
std::vector<std::string> ListFilesInDirectory(const std::string& directory) {
    std::vector<std::string> files;
    
#ifdef _WIN32
    WIN32_FIND_DATAA findFileData;
    std::string searchPath = directory + "\\*";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findFileData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                files.push_back(findFileData.cFileName);
            }
        } while (FindNextFileA(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(directory.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) { // Regular file
                files.push_back(entry->d_name);
            }
        }
        closedir(dir);
    }
#endif
    
    return files;
}

bool DirectoryExists(const std::string& path) {
#ifdef _WIN32
    DWORD attributes = GetFileAttributesA(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && 
            (attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat statbuf;
    return (stat(path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
#endif
}

// Image metadata extraction (basic)
struct ImageInfo {
    int width = 0;
    int height = 0;
    std::string format;
    long long fileSize = 0;
    bool isValid = false;
};

ImageInfo GetImageInfo(const std::string& filename) {
    ImageInfo info;
    info.fileSize = GetFileSize(filename);
    info.format = GetFileExtension(filename);
    
    // Basic validation - file exists and is reasonable size
    if (info.fileSize > 0 && info.fileSize < 100 * 1024 * 1024 && // Max 100MB
        IsImageFile(filename)) {
        info.isValid = true;
    }
    
    // For more detailed image info, you'd need a library like SOIL, FreeImage, etc.
    // This is a basic implementation
    
    return info;
}

// Safe filename generation
std::string Utils::SanitizeFilename(const std::string& filename) {
    std::string sanitized = filename;
    
    // Replace invalid characters with underscores
    const std::string invalidChars = "<>:\"/\\|?*";
    for (char c : invalidChars) {
        std::replace(sanitized.begin(), sanitized.end(), c, '_');
    }
    
    // Remove control characters
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(),
        [](char c) { return std::iscntrl(c); }), sanitized.end());
    
    // Trim whitespace
    sanitized = Trim(sanitized);
    
    // Ensure it's not empty and not too long
    if (sanitized.empty()) {
        sanitized = "unnamed_file";
    }
    if (sanitized.length() > 200) {
        sanitized = sanitized.substr(0, 200);
    }
    
    // Remove leading/trailing dots (Windows doesn't like these)
    while (!sanitized.empty() && sanitized.front() == '.') {
        sanitized.erase(0, 1);
    }
    while (!sanitized.empty() && sanitized.back() == '.') {
        sanitized.pop_back();
    }
    
    // Final check for empty
    if (sanitized.empty()) {
        sanitized = "unnamed_file";
    }
    
    return sanitized;
}

// Generate unique filename if file already exists
std::string Utils::GetUniqueFilename(const std::string& directory, const std::string& filename) {
    std::string fullPath = directory + "/" + filename;
    
    if (!FileExists(fullPath)) {
        return filename;
    }
    
    // Extract name and extension
    size_t dotPos = filename.find_last_of('.');
    std::string name, extension;
    
    if (dotPos != std::string::npos) {
        name = filename.substr(0, dotPos);
        extension = filename.substr(dotPos);
    } else {
        name = filename;
        extension = "";
    }
    
    // Try numbered variations
    for (int i = 1; i <= 999; ++i) {
        std::string newFilename = name + "_" + std::to_string(i) + extension;
        fullPath = directory + "/" + newFilename;
        
        if (!FileExists(fullPath)) {
            return newFilename;
        }
    }
    
    // If all numbered variations exist, use timestamp
    return name + "_" + std::to_string(std::time(nullptr)) + extension;
}

// Enhanced logging for image operations
void LogImageOperation(const std::string& operation, const std::string& filename, bool success) {
    std::string message = operation + " - " + filename + (success ? " [SUCCESS]" : " [FAILED]");
    LogMessage(message, success ? "INFO" : "ERROR");
}

} // Utils namespace