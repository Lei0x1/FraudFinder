#ifndef CONSOLE_UI_H
#define CONSOLE_UI_H

#include "database.h"
#include "fraud_entry.h"
#include <memory>
#include <string>

class ConsoleUI {
public:
    explicit ConsoleUI(std::shared_ptr<Database> db);
    
    void run();

private:
    std::shared_ptr<Database> database_;
    
    // Main menu system
    void ShowWelcome();
    void ShowMainMenu();
    int GetMenuChoice();
    void CleanupTempFiles();
    
    // Core functionality
    void AddNewEntry();
    void ListAllEntries();
    void SearchEntries();
    void UpdateEntry();
    void DeleteEntry();
    void ShowStatistics();
    void ExportData();
    void ViewEntryEvidence(int entryId);
    bool OpenImageFile(const std::string& filePath);
    std::string GetValidatedImagePath();
    std::string EscapeCsvField(const std::string& field);
    void ViewEvidenceMenu();
    bool CopyImageFile(const std::string& source, const std::string& destination);
    void ExportSingleImage();
    
    // Display helpers
    void DisplayEntry(const FraudEntry& entry, bool detailed = false);
    void DisplayEntryTable(const std::vector<FraudEntry>& entries);
    void PrintSeparator(char ch = '-', int length = 80);
    void PrintHeader(const std::string& title);
    
    // Input helpers
    std::string GetInput(const std::string& prompt);
    int GetIntInput(const std::string& prompt, int min, int max);
    bool ConfirmAction(const std::string& action);
    
    // Entry creation helper
    FraudEntry CreateEntryFromInput();
    
    // Search menu
    void ShowSearchMenu();
    void SearchById();
    void SearchByIdentifier();
    void SearchByType();
    void SearchByPlatform();
    
    // Utility
    void ClearScreen();
    void PauseForUser();
    void ShowError(const std::string& message);
    void ShowSuccess(const std::string& message);
    void ShowInfo(const std::string& message);
};

#endif