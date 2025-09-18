#include "console_ui.h"
#include "utils.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <limits>
#include <windows.h>
#undef max

ConsoleUI::ConsoleUI(std::shared_ptr<Database> db) : database_(db) {}

void ConsoleUI::run() {
    ShowWelcome();
    
    while (true) {
        ShowMainMenu();
        int choice = GetMenuChoice();
        
        switch (choice) {
            case 1: AddNewEntry(); break;
            case 2: ListAllEntries(); break;
            case 3: SearchEntries(); break;
            case 4: UpdateEntry(); break;
            case 5: DeleteEntry(); break;
            case 6: ShowStatistics(); break;
            case 7: ExportData(); break;
            case 8: ViewEvidenceMenu(); break;
            case 9: ExportSingleImage(); break;
            case 10: 
                ShowSuccess("Thank you for using FraudFinder!");
                
                // Clean up temp files on exit
                CleanupTempFiles();
                return;
            default:
                ShowError("Invalid choice. Please try again.");
                PauseForUser();
        }
    }
}

void ConsoleUI::CleanupTempFiles() {
    std::string tempDir = "temp_evidence";
    if (Utils::DirectoryExists(tempDir)) {
        ShowInfo("Cleaning up temporary files...");
        
        auto files = Utils::ListFilesInDirectory(tempDir);
        int cleanedCount = 0;
        
        for (const auto& filename : files) {
            std::string fullPath = tempDir + "/" + filename;
            if (std::remove(fullPath.c_str()) == 0) {
                cleanedCount++;
            }
        }
        
        if (cleanedCount > 0) {
            ShowInfo("Cleaned up " + std::to_string(cleanedCount) + " temporary files.");
        }
        
        // Try to remove the directory (will only work if empty)
        #ifdef _WIN32
        RemoveDirectoryA(tempDir.c_str());
        #else
        rmdir(tempDir.c_str());
        #endif
    }
}

void ConsoleUI::ShowWelcome() {
    ClearScreen();
    Utils::SetColor(Utils::CYAN);
    Utils::PrintBox("WELCOME TO FRAUDFINDER", 80);
    Utils::ResetColor();
    
    Utils::SetColor(Utils::YELLOW);
    std::cout << Utils::CenterText("Professional Fraud Tracking & Reporting System", 80) << "\n";
    Utils::ResetColor();
    
    Utils::SetColor(Utils::GREEN);
    std::cout << Utils::CenterText("Protecting Communities Through Information Sharing", 80) << "\n";
    Utils::ResetColor();
    
    PrintSeparator('=');
    std::cout << "\n";
    PauseForUser();
}

void ConsoleUI::ShowMainMenu() {
    // Enable UTF-8 in console
    SetConsoleOutputCP(CP_UTF8);

    ClearScreen();
    PrintHeader("FRAUDFINDER - MAIN MENU");

    Utils::SetColor(Utils::WHITE);

    std::cout << "┌────────────────────────────────────┬──────────────────────────────────────┐\n";
    std::cout << "│  1. Add New Fraud Entry            │  6. View Statistics                  │\n";
    std::cout << "│  2. List All Entries               │  7. Export Data                      │\n";
    std::cout << "│  3. Search Entries                 │  8. View Evidence                    │\n";
    std::cout << "│  4. Update Entry                   │  9. Export Single Image             │\n";
    std::cout << "│  5. Delete Entry                   │  10. Exit                            │\n";
    std::cout << "└────────────────────────────────────┴──────────────────────────────────────┘\n";

    Utils::ResetColor();

    ShowInfo("Total entries in database: " + std::to_string(database_->GetTotalEntries()));
    
    // Show embedded images count
    int entriesWithImages = 0;
    auto entries = database_->GetAllEntries();
    for (const auto& entry : entries) {
        if (database_->HasEmbeddedImage(entry.id)) {
            entriesWithImages++;
        }
    }
    
    if (entriesWithImages > 0) {
        Utils::SetColor(Utils::GREEN);
        std::cout << "Info: " << entriesWithImages << " entries have embedded images\n";
        Utils::ResetColor();
    }
    
    Utils::PrintSpace();
    std::cout << "Select an option (1-10): ";
}

int ConsoleUI::GetMenuChoice() {
    return GetIntInput("Select an option (1-10): ", 1, 10);
}

void ConsoleUI::AddNewEntry() {
    ClearScreen();
    PrintHeader("ADD NEW FRAUD ENTRY");
    
    try {
        FraudEntry entry = CreateEntryFromInput();
        entry.date_reported = Utils::Now();
        
        if (database_->AddEntry(entry)) {
            ShowSuccess("Fraud entry added successfully!");
        } else {
            ShowError("Failed to add entry: " + database_->getLastError());
        }
    } catch (const std::exception& e) {
        ShowError("Error creating entry: " + std::string(e.what()));
    }
    
    PauseForUser();
}

void ConsoleUI::ListAllEntries() {
    ClearScreen();
    PrintHeader("ALL FRAUD ENTRIES");
    
    auto entries = database_->GetAllEntries();
    
    if (entries.empty()) {
        ShowInfo("No entries found in the database.");
    } else {
        DisplayEntryTable(entries);
    }
    
    PauseForUser();
}

void ConsoleUI::SearchEntries() {
    ClearScreen();
    ShowSearchMenu();
    
    int choice = GetIntInput("Select search type (1-5): ", 1, 5);
    
    switch (choice) {
        case 1: SearchById(); break;
        case 2: SearchByIdentifier(); break;
        case 3: SearchByType(); break;
        case 4: SearchByPlatform(); break;
        case 5: return; // Back to main menu
    }
    
    PauseForUser();
}

void ConsoleUI::UpdateEntry() {
    ClearScreen();
    PrintHeader("UPDATE FRAUD ENTRY");
    
    int id = GetIntInput("Enter entry ID to update: ", 1, 999999);
    
    auto existingEntry = database_->GetEntryById(id);
    if (!existingEntry) {
        ShowError("Entry with ID " + std::to_string(id) + " not found.");
        PauseForUser();
        return;
    }
    
    std::cout << "\nCurrent entry details:\n";
    DisplayEntry(*existingEntry, true);
    
    if (!ConfirmAction("update this entry")) {
        ShowInfo("Update cancelled.");
        PauseForUser();
        return;
    }
    
    std::cout << "\nEnter new information (press Enter to keep current value):\n";
    
    FraudEntry updatedEntry = *existingEntry;
    
    std::string input = GetInput("Identifier [" + updatedEntry.identifier + "]: ");
    if (!input.empty()) updatedEntry.identifier = input;
    
    input = GetInput("Description [" + updatedEntry.description + "]: ");
    if (!input.empty()) updatedEntry.description = input;
    
    input = GetInput("Reporter Name [" + updatedEntry.reporter_name + "]: ");
    if (!input.empty()) updatedEntry.reporter_name = input;
    
    input = GetInput("Evidence Path [" + updatedEntry.evidence_path + "]: ");
    if (!input.empty()) updatedEntry.evidence_path = input;
    
    input = GetInput("Notes [" + updatedEntry.notes + "]: ");
    if (!input.empty()) updatedEntry.notes = input;
    
    if (database_->UpdateEntry(id, updatedEntry)) {
        ShowSuccess("Entry updated successfully!");
    } else {
        ShowError("Failed to update entry: " + database_->getLastError());
    }
    
    PauseForUser();
}

void ConsoleUI::DeleteEntry() {
    ClearScreen();
    PrintHeader("DELETE FRAUD ENTRY");
    
    int id = GetIntInput("Enter entry ID to delete: ", 1, 999999);
    
    auto entry = database_->GetEntryById(id);
    if (!entry) {
        ShowError("Entry with ID " + std::to_string(id) + " not found.");
        PauseForUser();
        return;
    }
    
    std::cout << "\nEntry to be deleted:\n";
    DisplayEntry(*entry, true);
    
    if (!ConfirmAction("permanently delete this entry")) {
        ShowInfo("Deletion cancelled.");
        PauseForUser();
        return;
    }
    
    if (database_->DeleteEntry(id)) {
        ShowSuccess("Entry deleted successfully!");
    } else {
        ShowError("Failed to delete entry: " + database_->getLastError());
    }
    
    PauseForUser();
}

void ConsoleUI::ShowStatistics() {
    ClearScreen();
    database_->PrintStats();
    PauseForUser();
}

void ConsoleUI::ExportData() {
    ClearScreen();
    PrintHeader("EXPORT DATA");
    
    std::string baseFilename = GetInput("Enter filename (without extension): ");
    if (baseFilename.empty()) {
        baseFilename = "fraudfinder_export";
    }
    
    std::string csvFilename = baseFilename + ".csv";
    std::string exportDir = baseFilename + "_evidence";
    
    auto entries = database_->GetAllEntries();
    
    if (entries.empty()) {
        ShowInfo("No entries to export.");
        PauseForUser();
        return;
    }
    
    std::ofstream file(csvFilename);
    if (!file.is_open()) {
        ShowError("Failed to create export file: " + csvFilename);
        PauseForUser();
        return;
    }
    
    // Ask if user wants to extract embedded images
    bool extractImages = false;
    std::string extractChoice = GetInput("Extract embedded images to files? (y/n): ");
    if (Utils::ToLowerCase(extractChoice) == "y" || Utils::ToLowerCase(extractChoice) == "yes") {
        extractImages = true;
        if (!Utils::CreateDirectoryIfNotExists(exportDir)) {
            ShowError("Failed to create export directory: " + exportDir);
            extractImages = false;
        } else {
            ShowSuccess("Created export directory: " + exportDir);
        }
    }
    
    // Write CSV header
    file << "ID,Identifier,Description,Fraud Type,Platform,Severity,"
         << "Original Evidence Path,Extracted Evidence File,Has Embedded Image,"
         << "Reporter Name,Date Reported,Verified,Notes\n";
    
    int extractedImages = 0;
    int failedExtractions = 0;
    int totalWithImages = 0;
    
    // Write data with progress
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        
        std::string extractedFilename = "";
        bool hasEmbedded = database_->HasEmbeddedImage(entry.id);
        
        if (hasEmbedded) {
            totalWithImages++;
        }
        
        // Handle image extraction
        if (extractImages && hasEmbedded) {
            std::string tempPath;
            if (database_->ExtractImageToTempFile(entry.id, tempPath)) {
                // Move from temp to export directory
                size_t lastSlash = tempPath.find_last_of("/\\");
                std::string filename = (lastSlash != std::string::npos) ? 
                    tempPath.substr(lastSlash + 1) : tempPath;
                
                std::string finalPath = exportDir + "/" + filename;
                
                if (Utils::CopyFile(tempPath, finalPath)) {
                    extractedFilename = filename;
                    extractedImages++;
                    
                    // Clean up temp file
                    std::remove(tempPath.c_str());
                } else {
                    failedExtractions++;
                    extractedFilename = "EXTRACTION_FAILED";
                }
            } else {
                failedExtractions++;
                extractedFilename = "EXTRACTION_FAILED";
            }
        }
        
        // Write CSV row
        file << entry.id << ","
             << "\"" << EscapeCsvField(entry.identifier) << "\","
             << "\"" << EscapeCsvField(entry.description) << "\","
             << "\"" << entry.FraudTypeToString() << "\","
             << "\"" << entry.PlatformToString() << "\","
             << "\"" << entry.SeverityToString() << "\","
             << "\"" << EscapeCsvField(entry.evidence_path) << "\","
             << "\"" << EscapeCsvField(extractedFilename) << "\","
             << (hasEmbedded ? "Yes" : "No") << ","
             << "\"" << EscapeCsvField(entry.reporter_name) << "\","
             << "\"" << entry.DateToString() << "\","
             << (entry.is_verified ? "Yes" : "No") << ","
             << "\"" << EscapeCsvField(entry.notes) << "\"\n";
        
        // Show progress
        if ((i + 1) % 10 == 0 || i == entries.size() - 1) {
            std::cout << "\rProcessing entries: " << (i + 1) << "/" << entries.size() << std::flush;
        }
    }
    
    file.close();
    std::cout << "\n";
    
    ShowSuccess("Data exported successfully to: " + csvFilename);
    ShowInfo("Exported " + std::to_string(entries.size()) + " entries.");
    ShowInfo("Found " + std::to_string(totalWithImages) + " entries with embedded images.");
    
    if (extractImages) {
        if (extractedImages > 0) {
            ShowSuccess("Extracted " + std::to_string(extractedImages) + " images to: " + exportDir);
        }
        if (failedExtractions > 0) {
            ShowError("Failed to extract " + std::to_string(failedExtractions) + " images.");
        }
    }
    
    PauseForUser();
}

bool ConsoleUI::CopyImageFile(const std::string& source, const std::string& destination) {
    std::ifstream src(source, std::ios::binary);
    if (!src.is_open()) {
        Utils::LogMessage("Failed to open source file: " + source, "ERROR");
        return false;
    }
    
    std::ofstream dst(destination, std::ios::binary);
    if (!dst.is_open()) {
        Utils::LogMessage("Failed to create destination file: " + destination, "ERROR");
        return false;
    }
    
    // Copy the file
    dst << src.rdbuf();
    
    bool success = src.good() && dst.good();
    if (success) {
        Utils::LogMessage("Successfully copied: " + source + " -> " + destination, "INFO");
    } else {
        Utils::LogMessage("Failed to copy: " + source + " -> " + destination, "ERROR");
    }
    
    return success;
}

std::string ConsoleUI::EscapeCsvField(const std::string& field) {
    std::string escaped = field;
    
    // Replace quotes with double quotes
    size_t pos = 0;
    while ((pos = escaped.find("\"", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\"\"");
        pos += 2;
    }
    
    return escaped;
}

void ConsoleUI::ViewEvidenceMenu() {
    ClearScreen();
    PrintHeader("VIEW EVIDENCE");
    
    // Show entries with embedded evidence
    auto entries = database_->GetAllEntries();
    std::vector<FraudEntry> entriesWithEvidence;
    
    for (const auto& entry : entries) {
        if (database_->HasEmbeddedImage(entry.id)) {
            entriesWithEvidence.push_back(entry);
        }
    }
    
    if (entriesWithEvidence.empty()) {
        ShowInfo("No entries with embedded evidence images found.");
        PauseForUser();
        return;
    }
    
    std::cout << "Entries with Embedded Evidence (" << entriesWithEvidence.size() << " total):\n\n";
    DisplayEntryTable(entriesWithEvidence);
    
    int id = GetIntInput("Enter entry ID to view evidence (0 to cancel): ", 0, 999999);
    if (id == 0) return;
    
    ViewEntryEvidence(id);
    PauseForUser();
}

void ConsoleUI::ViewEntryEvidence(int entryId) {
    auto entry = database_->GetEntryById(entryId);
    if (!entry) {
        ShowError("Entry with ID " + std::to_string(entryId) + " not found.");
        return;
    }
    
    if (!database_->HasEmbeddedImage(entryId)) {
        ShowError("No embedded evidence image found for this entry.");
        return;
    }
    
    ShowInfo("Extracting embedded evidence image...");
    
    std::string tempFilePath;
    if (database_->ExtractImageToTempFile(entryId, tempFilePath)) {
        ShowSuccess("Evidence extracted to: " + tempFilePath);
        
        if (OpenImageFile(tempFilePath)) {
            ShowSuccess("Evidence image opened successfully.");
            
            // Ask if user wants to keep the temp file
            std::string keepChoice = GetInput("Keep extracted file? (y/n): ");
            if (Utils::ToLowerCase(keepChoice) != "y" && Utils::ToLowerCase(keepChoice) != "yes") {
                if (std::remove(tempFilePath.c_str()) == 0) {
                    ShowInfo("Temporary file cleaned up.");
                } else {
                    ShowError("Failed to remove temporary file: " + tempFilePath);
                }
            }
        } else {
            ShowError("Failed to open evidence image. File saved at: " + tempFilePath);
        }
    } else {
        ShowError("Failed to extract embedded evidence image.");
    }
}

bool ConsoleUI::OpenImageFile(const std::string& filePath) {
#ifdef _WIN32
    // Windows - use default associated program
    std::string command = "start \"\" \"" + filePath + "\"";
    int result = system(command.c_str());
    return result == 0;
#elif defined(__APPLE__)
    // macOS
    std::string command = "open \"" + filePath + "\"";
    int result = system(command.c_str());
    return result == 0;
#else
    // Linux - try common image viewers
    std::vector<std::string> viewers = {"xdg-open", "eog", "feh", "display"};
    
    for (const auto& viewer : viewers) {
        std::string command = viewer + " \"" + filePath + "\" 2>/dev/null";
        int result = system(command.c_str());
        if (result == 0) return true;
    }
    return false;
#endif
}

void ConsoleUI::DisplayEntry(const FraudEntry& entry, bool detailed) {
    Utils::SetColor(Utils::CYAN);
    std::cout << "┌─ Entry ID: " << entry.id << " ";
    PrintSeparator('─', 70 - std::to_string(entry.id).length());
    Utils::ResetColor();
    
    std::cout << "Identifier:    " << entry.identifier << "\n";
    std::cout << "Type:          " << entry.FraudTypeToString() << "\n";
    std::cout << "Platform:      " << entry.PlatformToString() << "\n";
    
    Utils::SetColor(Utils::YELLOW);
    std::cout << "Severity:      " << entry.SeverityToString() << "\n";
    Utils::ResetColor();
    
    std::cout << "Reported:      " << entry.DateToString() << "\n";
    
    if (entry.is_verified) {
        Utils::SetColor(Utils::GREEN);
        std::cout << "Status:        VERIFIED\n";
        Utils::ResetColor();
    } else {
        Utils::SetColor(Utils::RED);
        std::cout << "Status:        UNVERIFIED\n";
        Utils::ResetColor();
    }
    
    if (detailed) {
        std::cout << "Description:   " << entry.description << "\n";
        if (!entry.reporter_name.empty())
            std::cout << "Reporter:      " << entry.reporter_name << "\n";
        
        // Check for embedded evidence
        if (database_->HasEmbeddedImage(entry.id)) {
            Utils::SetColor(Utils::GREEN);
            std::cout << "Evidence:      [Embedded Image Available]";
            Utils::ResetColor();
            std::cout << "\n";
            
            if (!entry.evidence_path.empty()) {
                std::cout << "Original Path: " << entry.evidence_path << "\n";
            }
            
            // Ask if user wants to view the evidence
            std::string viewChoice = GetInput("View embedded evidence? (y/n): ");
            if (Utils::ToLowerCase(viewChoice) == "y" || Utils::ToLowerCase(viewChoice) == "yes") {
                ViewEntryEvidence(entry.id);
            }
        } else if (!entry.evidence_path.empty()) {
            Utils::SetColor(Utils::RED);
            std::cout << "Evidence:      " << entry.evidence_path << " [No embedded image]";
            Utils::ResetColor();
            std::cout << "\n";
        }
        
        if (!entry.notes.empty())
            std::cout << "Notes:         " << entry.notes << "\n";
    }
    
    PrintSeparator('─', 80);
    std::cout << "\n";
}

std::string ConsoleUI::GetValidatedImagePath() {
    std::string path;
    bool validPath = false;
    
    while (!validPath) {
        path = GetInput("Evidence file path (image will be embedded in database - press Enter to skip): ");
        
        if (path.empty()) {
            return path; // Empty path is valid (optional)
        }
        
        // Check if file exists
        if (!Utils::FileExists(path)) {
            ShowError("File not found: " + path);
            std::string retry = GetInput("Try another path? (y/n): ");
            if (Utils::ToLowerCase(retry) != "y" && Utils::ToLowerCase(retry) != "yes") {
                return ""; // User chose not to retry
            }
            continue;
        }
        
        // Check file size (warn if too large)
        if (!Utils::IsValidImageSize(path, 50)) { // 50MB limit
            ShowError("Image file is very large (>50MB). This may slow down the database.");
            std::string proceed = GetInput("Continue anyway? (y/n): ");
            if (Utils::ToLowerCase(proceed) != "y" && Utils::ToLowerCase(proceed) != "yes") {
                continue;
            }
        }
        
        // Check if it's an image file (basic extension check)
        if (!Utils::IsImageFile(path)) {
            ShowError("File doesn't appear to be an image file.");
            std::string proceed = GetInput("Use this file anyway? (y/n): ");
            if (Utils::ToLowerCase(proceed) != "y" && Utils::ToLowerCase(proceed) != "yes") {
                continue;
            }
        }
        
        // Show file info
        long long fileSize = Utils::GetFileSize(path);
        if (fileSize > 0) {
            ShowInfo("File size: " + Utils::FormatFileSize(fileSize));
        }
        
        validPath = true;
    }
    
    return path;
}

void ConsoleUI::DisplayEntryTable(const std::vector<FraudEntry>& entries) {
    if (entries.empty()) return;
    
    std::cout << std::left;
    std::cout << std::setw(4) << "ID" << " | "
              << std::setw(20) << "Identifier" << " | "
              << std::setw(15) << "Type" << " | "
              << std::setw(12) << "Platform" << " | "
              << std::setw(8) << "Severity" << " | "
              << std::setw(10) << "Date" << "\n";
    
    PrintSeparator('-', 80);
    
    for (const auto& entry : entries) {
        Utils::SetColor(entry.is_verified ? Utils::GREEN : Utils::RED);
        std::cout << std::setw(4) << entry.id << " | ";
        Utils::ResetColor();
        
        std::cout << std::setw(20) << (entry.identifier.length() > 20 ? 
                                      entry.identifier.substr(0, 17) + "..." : 
                                      entry.identifier) << " | ";
        
        std::cout << std::setw(15) << entry.FraudTypeToString() << " | ";
        std::cout << std::setw(12) << entry.PlatformToString() << " | ";
        
        // Color code severity
        switch (entry.severity) {
            case Severity::CRITICAL:
                Utils::SetColor(Utils::RED);
                break;
            case Severity::HIGH:
                Utils::SetColor(Utils::YELLOW);
                break;
            case Severity::MEDIUM:
                Utils::SetColor(Utils::BLUE);
                break;
            default:
                Utils::SetColor(Utils::DEFAULT);
        }
        std::cout << std::setw(8) << entry.SeverityToString() << " | ";
        Utils::ResetColor();
        
        std::cout << std::setw(10) << entry.DateToString().substr(0, 10) << "\n";
    }
    
    std::cout << std::right;
    PrintSeparator('-', 80);
    std::cout << "Total: " << entries.size() << " entries\n\n";
}

void ConsoleUI::PrintSeparator(char ch, int length) {
    std::cout << std::string(length, ch) << "\n";
}

void ConsoleUI::PrintHeader(const std::string& title) {
    Utils::SetColor(Utils::CYAN);
    Utils::PrintBox(title, 80);
    Utils::ResetColor();
    std::cout << "\n";
}

std::string ConsoleUI::GetInput(const std::string& prompt) {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return Utils::Trim(input);
}

int ConsoleUI::GetIntInput(const std::string& prompt, int min, int max) {
    int value;
    std::string input;
    
    do {
        std::cout << prompt;
        std::getline(std::cin, input);
        
        if (Utils::IsNumber(input)) {
            value = std::stoi(input);
            if (value >= min && value <= max) {
                return value;
            }
        }
        
        ShowError("Please enter a number between " + std::to_string(min) + 
                 " and " + std::to_string(max) + ".");
    } while (true);
}

bool ConsoleUI::ConfirmAction(const std::string& action) {
    std::string input = GetInput("Are you sure you want to " + action + "? (y/n): ");
    return Utils::ToLowerCase(input) == "y" || Utils::ToLowerCase(input) == "yes";
}

FraudEntry ConsoleUI::CreateEntryFromInput() {
    FraudEntry entry;
    
    entry.identifier = GetInput("Enter identifier (username/email/phone): ");
    entry.description = GetInput("Enter description: ");
    
    std::cout << "\n";
    FraudEntry::PrintFraudTypeOptions();
    int fraudType = GetIntInput("Select fraud type (1-7): ", 1, 7);
    entry.fraud_type = FraudEntry::IntToFraudType(fraudType);
    
    std::cout << "\n";
    FraudEntry::PrintPlatformOptions();
    int platform = GetIntInput("Select platform (1-8): ", 1, 8);
    entry.platform = FraudEntry::IntToPlatform(platform);
    
    std::cout << "\n";
    FraudEntry::PrintSeverityOptions();
    int severity = GetIntInput("Select severity (1-4): ", 1, 4);
    entry.severity = FraudEntry::IntToSeverity(severity);
    
    std::cout << "\n";
    ShowInfo("The image will be embedded directly into the database.");
    entry.evidence_path = GetValidatedImagePath();
    
    if (!entry.evidence_path.empty()) {
        ShowSuccess("Image will be embedded when entry is saved.");
    }
    
    entry.reporter_name = GetInput("Your name (optional): ");
    entry.notes = GetInput("Additional notes (optional): ");
    
    std::string verifiedInput = GetInput("Is this entry verified? (y/n): ");
    entry.is_verified = (Utils::ToLowerCase(verifiedInput) == "y" || 
                        Utils::ToLowerCase(verifiedInput) == "yes");
    
    return entry;
}

void ConsoleUI::ExportSingleImage() {
    ClearScreen();
    PrintHeader("EXPORT SINGLE IMAGE");
    
    int id = GetIntInput("Enter entry ID to export image: ", 1, 999999);
    
    if (!database_->HasEmbeddedImage(id)) {
        ShowError("Entry " + std::to_string(id) + " has no embedded image.");
        PauseForUser();
        return;
    }
    
    std::string outputPath = GetInput("Enter output file path (with extension): ");
    if (outputPath.empty()) {
        ShowError("Output path cannot be empty.");
        PauseForUser();
        return;
    }
    
    // Validate output directory exists
    size_t lastSlash = outputPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        std::string outputDir = outputPath.substr(0, lastSlash);
        if (!Utils::DirectoryExists(outputDir)) {
            ShowError("Output directory does not exist: " + outputDir);
            PauseForUser();
            return;
        }
    }
    
    // Check if output file already exists
    if (Utils::FileExists(outputPath)) {
        std::string overwrite = GetInput("File already exists. Overwrite? (y/n): ");
        if (Utils::ToLowerCase(overwrite) != "y" && Utils::ToLowerCase(overwrite) != "yes") {
            ShowInfo("Export cancelled.");
            PauseForUser();
            return;
        }
    }
    
    ShowInfo("Extracting and exporting image...");
    
    if (database_->ExportEmbeddedImage(id, outputPath)) {
        ShowSuccess("Image exported successfully to: " + outputPath);
        
        // Show file info
        long long fileSize = Utils::GetFileSize(outputPath);
        if (fileSize > 0) {
            ShowInfo("File size: " + Utils::FormatFileSize(fileSize));
        }
    } else {
        ShowError("Failed to export image: " + database_->getLastError());
        
        // Try manual extraction as fallback
        ShowInfo("Attempting manual extraction...");
        std::string tempPath;
        if (database_->ExtractImageToTempFile(id, tempPath)) {
            ShowInfo("Image extracted to temporary file: " + tempPath);
            ShowInfo("You can manually copy this file to your desired location.");
        }
    }
    
    PauseForUser();
}

void ConsoleUI::ShowSearchMenu() {
    PrintHeader("SEARCH FRAUD ENTRIES");
    
    std::cout << "1. Search by ID\n";
    std::cout << "2. Search by Identifier\n";
    std::cout << "3. Search by Fraud Type\n";
    std::cout << "4. Search by Platform\n";
    std::cout << "5. Back to Main Menu\n\n";
}

void ConsoleUI::SearchById() {
    ClearScreen();
    PrintHeader("SEARCH BY ID");
    
    int id = GetIntInput("Enter entry ID: ", 1, 999999);
    auto entry = database_->GetEntryById(id);
    
    if (entry) {
        DisplayEntry(*entry, true);
    } else {
        ShowError("Entry with ID " + std::to_string(id) + " not found.");
    }
}

void ConsoleUI::SearchByIdentifier() {
    ClearScreen();
    PrintHeader("SEARCH BY IDENTIFIER");
    
    std::string identifier = GetInput("Enter identifier to search for: ");
    auto entries = database_->SearchByIdentifier(identifier);
    
    if (entries.empty()) {
        ShowInfo("No entries found matching: " + identifier);
    } else {
        DisplayEntryTable(entries);
    }
}

void ConsoleUI::SearchByType() {
    ClearScreen();
    PrintHeader("SEARCH BY FRAUD TYPE");
    
    FraudEntry::PrintFraudTypeOptions();
    int type = GetIntInput("Select fraud type (1-7): ", 1, 7);
    
    auto entries = database_->SearchByFraudType(FraudEntry::IntToFraudType(type));
    
    if (entries.empty()) {
        ShowInfo("No entries found for selected fraud type.");
    } else {
        DisplayEntryTable(entries);
    }
}

void ConsoleUI::SearchByPlatform() {
    ClearScreen();
    PrintHeader("SEARCH BY PLATFORM");
    
    FraudEntry::PrintPlatformOptions();
    int platform = GetIntInput("Select platform (1-8): ", 1, 8);
    
    auto entries = database_->SearchByPlatform(FraudEntry::IntToPlatform(platform));
    
    if (entries.empty()) {
        ShowInfo("No entries found for selected platform.");
    } else {
        DisplayEntryTable(entries);
    }
}

void ConsoleUI::ClearScreen() {
    Utils::ClearScreen();
}

void ConsoleUI::PauseForUser() {
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void ConsoleUI::ShowError(const std::string& message) {
    Utils::SetColor(Utils::RED);
    std::cout << "Error: " << message << "\n";
    Utils::ResetColor();
}

void ConsoleUI::ShowSuccess(const std::string& message) {
    Utils::SetColor(Utils::GREEN);
    std::cout << "Success: " << message << "\n";
    Utils::ResetColor();
}

void ConsoleUI::ShowInfo(const std::string& message) {
    Utils::SetColor(Utils::BLUE);
    std::cout << "Info: " << message << "\n";
    Utils::ResetColor();
}
