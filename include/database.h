#ifndef DATABASE_H
#define DATABASE_H

#include "fraud_entry.h"
#include <string>
#include <vector>
#include <optional>
#include <sqlite3.h>

class Database {
public:
    explicit Database(const std::string& db_path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Core operations
    bool AddEntry(const FraudEntry& entry);
    bool UpdateEntry(int id, const FraudEntry& entry);
    bool DeleteEntry(int id);
    
    // Query operations
    std::optional<FraudEntry> GetEntryById(int id);
    std::vector<FraudEntry> GetAllEntries();
    std::vector<FraudEntry> SearchByIdentifier(const std::string& identifier);
    std::vector<FraudEntry> SearchByFraudType(FraudType type);
    std::vector<FraudEntry> SearchByPlatform(Platform platform);
    
    // Statistics
    int GetTotalEntries();
    void PrintStats();
    
    bool isConnected() const { return db_ != nullptr; }
    std::string getLastError() const { return lastError_; }

    // New BLOB-related public methods
    bool ExtractImageToTempFile(int entryId, std::string& tempFilePath);
    bool HasEmbeddedImage(int entryId);
    bool ExportEmbeddedImage(int entryId, const std::string& outputPath);

private:
    sqlite3* db_;
    std::string lastError_;
    
    bool Initialize();
    bool ExecuteQuery(const std::string& query);
    FraudEntry RowToFraudEntry(sqlite3_stmt* stmt);
    void SetError(const std::string& error);

    // New BLOB-related private methods
    std::vector<unsigned char> ReadImageFile(const std::string& filePath);
    std::string GetMimeType(const std::string& filename);
};

#endif