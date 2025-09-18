#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <regex>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace Utils {

// Color codes
enum Color {
    RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, DEFAULT
};

// Console
void ClearScreen();
void SetColor(int color);
void ResetColor();

// String utils
std::string Trim(const std::string& str);
std::string ToLowerCase(const std::string& str);
bool IsNumber(const std::string& str);

// Time
std::chrono::system_clock::time_point Now();
std::string GetCurrentDateTime();
std::string GetDateOnly();
std::string GetTimeOnly();
std::string TimeToString(const std::chrono::system_clock::time_point& tp);

// Files
bool FileExists(const std::string& path);

// Formatting
std::string CenterText(const std::string& text, int width);
std::string PadLeft(const std::string& str, int width, char padChar);
std::string PadRight(const std::string& str, size_t length, char padChar = ' ');
std::string Truncate(const std::string& str, int maxLength, const std::string& suffix = "...");
std::string FormatNumber(int number);
std::string FormatPercentage(double value, int precision = 2);
std::string FormatFileSize(long long bytes);

// Console output
void PrintLine(char character = '-', int length = 50);
void PrintBox(const std::string& content, int width = 50);
void PrintHeader(const std::string& title);
void PrintSubHeader(const std::string& subtitle);
void PrintSuccess(const std::string& message);
void PrintError(const std::string& message);
void PrintWarning(const std::string& message);
void PrintInfo(const std::string& message);
void PrintProgressBar(int current, int total, int width = 50);
void PrintTable(const std::vector<std::vector<std::string>>& data,
                const std::vector<std::string>& headers,
                const std::vector<int>& columnWidths);
void PrintSpace();

// Animation
void AnimateText(const std::string& text, int delayMs = 30);
void ShowSpinner(int durationMs);

// Validation
bool ValidateEmail(const std::string& email);
bool ValidatePhoneNumber(const std::string& phone);
bool ValidateUrl(const std::string& url);

// Input/Output
std::string SanitizeInput(const std::string& input);
void LogMessage(const std::string& message, const std::string& level = "INFO");

// System
bool CreateDirectoryIfNotExists(const std::string& path);
std::string GetExecutablePath();
void PauseExecution(int milliseconds);
bool CopyFile(const std::string& source, const std::string& destination);
std::string GetFileExtension(const std::string& filename);
bool IsImageFile(const std::string& filename);
long long GetFileSize(const std::string& filename);
bool IsValidImageSize(const std::string& filename, long long maxSizeMB = 50);
std::vector<std::string> ListFilesInDirectory(const std::string& directory);
bool DirectoryExists(const std::string& path);
std::string SanitizeFilename(const std::string& filename);
std::string GetUniqueFilename(const std::string& directory, const std::string& filename);
void LogImageOperation(const std::string& operation, const std::string& filename, bool success);

} // namespace Utils
