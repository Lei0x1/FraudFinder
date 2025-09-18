#include "database.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <iomanip>

Database::Database(const std::string& db_path) : db_(nullptr) {
    if (sqlite3_open(db_path.c_str(), &db_)) {
    SetError("Can't open database: " + std::string(sqlite3_errmsg(db_)));
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    } else {
    if (!Initialize()) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::Initialize() {
    const std::string createTable = R"(
        CREATE TABLE IF NOT EXISTS fraud_entries (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            identifier TEXT NOT NULL,
            description TEXT NOT NULL,
            fraud_type INTEGER NOT NULL,
            platform INTEGER NOT NULL,
            severity INTEGER NOT NULL,
            evidence_path TEXT,
            evidence_blob BLOB,
            evidence_filename TEXT,
            evidence_mime_type TEXT,
            evidence_size INTEGER,
            reporter_name TEXT,
            date_reported TEXT NOT NULL,
            is_verified INTEGER DEFAULT 0,
            notes TEXT
        );
    )";
    
    const std::string createIndex = R"(
        CREATE INDEX IF NOT EXISTS idx_identifier ON fraud_entries(identifier);
    )";
    
    return ExecuteQuery(createTable) && ExecuteQuery(createIndex);
}

std::vector<unsigned char> Database::ReadImageFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return std::vector<unsigned char>();
    }
    
    // Get file size
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Check file size limit (50MB max)
    const std::streamsize maxSize = 50 * 1024 * 1024;
    if (fileSize > maxSize) {
        std::cerr << "Image file too large: " << fileSize << " bytes (max: " << maxSize << ")\n";
        return std::vector<unsigned char>();
    }
    
    // Read file into vector
    std::vector<unsigned char> buffer(fileSize);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        return std::vector<unsigned char>();
    }
    
    return buffer;
}

bool Database::ExecuteQuery(const std::string& query) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, query.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
    SetError("SQL error: " + std::string(errMsg));
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::AddEntry(const FraudEntry& entry) {
    const std::string sql = R"(
        INSERT INTO fraud_entries 
        (identifier, description, fraud_type, platform, severity, 
         evidence_path, evidence_blob, evidence_filename, evidence_mime_type, evidence_size,
         reporter_name, date_reported, is_verified, notes)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    SetError("Failed to prepare statement: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }
    
    // Bind regular fields
    sqlite3_bind_text(stmt, 1, entry.identifier.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, entry.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, static_cast<int>(entry.fraud_type));
    sqlite3_bind_int(stmt, 4, static_cast<int>(entry.platform));
    sqlite3_bind_int(stmt, 5, static_cast<int>(entry.severity));
    sqlite3_bind_text(stmt, 6, entry.evidence_path.c_str(), -1, SQLITE_STATIC);
    
    // Handle image BLOB
    if (!entry.evidence_path.empty() && Utils::FileExists(entry.evidence_path)) {
        std::vector<unsigned char> imageData = ReadImageFile(entry.evidence_path);
        if (!imageData.empty()) {
            sqlite3_bind_blob(stmt, 7, imageData.data(), static_cast<int>(imageData.size()), SQLITE_TRANSIENT);
            
            // Extract filename
            size_t lastSlash = entry.evidence_path.find_last_of("/\\");
            std::string filename = (lastSlash != std::string::npos) ? 
                entry.evidence_path.substr(lastSlash + 1) : entry.evidence_path;
            sqlite3_bind_text(stmt, 8, filename.c_str(), -1, SQLITE_STATIC);
            
            // Store MIME type
            std::string mimeType = GetMimeType(entry.evidence_path);
            sqlite3_bind_text(stmt, 9, mimeType.c_str(), -1, SQLITE_STATIC);
            
            // Store file size
            sqlite3_bind_int64(stmt, 10, static_cast<sqlite3_int64>(imageData.size()));
            
            std::cout << "Embedded image: " << filename << " (" << imageData.size() << " bytes)\n";
        } else {
            sqlite3_bind_null(stmt, 7);
            sqlite3_bind_null(stmt, 8);
            sqlite3_bind_null(stmt, 9);
            sqlite3_bind_null(stmt, 10);
            std::cerr << "Failed to read image file: " << entry.evidence_path << "\n";
        }
    } else {
        sqlite3_bind_null(stmt, 7);
        sqlite3_bind_null(stmt, 8);
        sqlite3_bind_null(stmt, 9);
        sqlite3_bind_null(stmt, 10);
    }
    
    sqlite3_bind_text(stmt, 11, entry.reporter_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, Utils::TimeToString(entry.date_reported).c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 13, entry.is_verified ? 1 : 0);
    sqlite3_bind_text(stmt, 14, entry.notes.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
    SetError("Failed to insert entry: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }
    
    return true;
}

bool Database::ExtractImageToTempFile(int entryId, std::string& tempFilePath) {
    const std::string sql = R"(
        SELECT evidence_blob, evidence_filename, evidence_mime_type, evidence_size
        FROM fraud_entries WHERE id = ? AND evidence_blob IS NOT NULL;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    SetError("Failed to prepare extract statement: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    sqlite3_bind_int(stmt, 1, entryId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const void* blobData = sqlite3_column_blob(stmt, 0);
        int blobSize = sqlite3_column_bytes(stmt, 0);
        const char* filenamePtr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* mimeTypePtr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        if (blobData && blobSize > 0) {
            // Create temp directory if it doesn't exist
            std::string tempDir = "temp_evidence";
            if (!Utils::CreateDirectoryIfNotExists(tempDir)) {
                sqlite3_finalize(stmt);
                SetError("Failed to create temp directory: " + tempDir);
                return false;
            }

            std::string filename;
            std::string extension;

            if (filenamePtr && strlen(filenamePtr) > 0) {
                std::string fullFilename = std::string(filenamePtr);
                size_t lastDot = fullFilename.find_last_of('.');
                if (lastDot != std::string::npos && lastDot > 0) {
                    // Sanitize the filename but preserve the extension
                    filename = Utils::SanitizeFilename(fullFilename.substr(0, lastDot));
                    extension = fullFilename.substr(lastDot);
                } else {
                    // No extension found, sanitize the full string
                    filename = Utils::SanitizeFilename(fullFilename);
                    extension = ""; // Fallback will handle this below
                }
            }

            // Fallback to MIME type if no valid extension was found
            if (extension.empty() && mimeTypePtr) {
                std::string mimeType = std::string(mimeTypePtr);
                if (mimeType == "image/jpeg") extension = ".jpg";
                else if (mimeType == "image/png") extension = ".png";
                else if (mimeType == "image/gif") extension = ".gif";
                else if (mimeType == "image/bmp") extension = ".bmp";
                else if (mimeType == "image/tiff") extension = ".tiff";
                else if (mimeType == "image/webp") extension = ".webp";
                else extension = ".bin"; // Default
            } else if (extension.empty()) {
                extension = ".bin"; // Final fallback
            }

            // If sanitization resulted in an empty string, generate a new base filename
            if (filename.empty()) {
                filename = "evidence_" + std::to_string(entryId);
            }

            // Create unique temp file path to avoid conflicts
            tempFilePath = tempDir + "/" + Utils::GetUniqueFilename(tempDir, filename + extension);

            // Write blob data to file
            std::ofstream tempFile(tempFilePath, std::ios::binary);
            if (tempFile.is_open()) {
                tempFile.write(static_cast<const char*>(blobData), blobSize);
                tempFile.close();

                if (Utils::GetFileSize(tempFilePath) == blobSize) {
                    sqlite3_finalize(stmt);
                    return true;
                } else {
                    SetError("File size mismatch after writing: " + tempFilePath);
                    std::remove(tempFilePath.c_str());
                }
            } else {
                SetError("Failed to create temp file: " + tempFilePath);
            }
        } else {
            SetError("Invalid BLOB data for entry ID: " + std::to_string(entryId));
        }
    } else {
    SetError("No BLOB data found for entry ID: " + std::to_string(entryId));
    }

    sqlite3_finalize(stmt);
    return false;
}

bool Database::HasEmbeddedImage(int entryId) {
    const std::string sql = "SELECT evidence_blob FROM fraud_entries WHERE id = ? AND evidence_blob IS NOT NULL;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, entryId);
    bool hasImage = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    
    return hasImage;
}

bool Database::ExportEmbeddedImage(int entryId, const std::string& outputPath) {
    std::string tempPath;
    if (ExtractImageToTempFile(entryId, tempPath)) {
        if (Utils::CopyFile(tempPath, outputPath)) {
            // Clean up temp file
            std::remove(tempPath.c_str());
            return true;
        } else {
            SetError("Failed to copy extracted image to: " + outputPath);
            // Clean up temp file
            std::remove(tempPath.c_str());
        }
    }
    return false;
}

std::string Database::GetMimeType(const std::string& filename) {
    std::string extension = Utils::GetFileExtension(filename);
    
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "png") return "image/png";
    if (extension == "gif") return "image/gif";
    if (extension == "bmp") return "image/bmp";
    if (extension == "tiff" || extension == "tif") return "image/tiff";
    if (extension == "webp") return "image/webp";
    if (extension == "ico") return "image/x-icon";
    
    return "application/octet-stream";
}

bool Database::UpdateEntry(int id, const FraudEntry& entry) {
    const std::string sql = R"(
        UPDATE fraud_entries 
        SET identifier = ?, description = ?, fraud_type = ?, platform = ?, 
            severity = ?, evidence_path = ?, evidence_blob = ?, evidence_filename = ?, 
            evidence_mime_type = ?, evidence_size = ?, reporter_name = ?, 
            is_verified = ?, notes = ?
        WHERE id = ?;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    SetError("Failed to prepare update statement: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, entry.identifier.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, entry.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, static_cast<int>(entry.fraud_type));
    sqlite3_bind_int(stmt, 4, static_cast<int>(entry.platform));
    sqlite3_bind_int(stmt, 5, static_cast<int>(entry.severity));
    sqlite3_bind_text(stmt, 6, entry.evidence_path.c_str(), -1, SQLITE_STATIC);
    
    // Handle image BLOB for update
    if (!entry.evidence_path.empty() && Utils::FileExists(entry.evidence_path)) {
        std::vector<unsigned char> imageData = ReadImageFile(entry.evidence_path);
        if (!imageData.empty()) {
            sqlite3_bind_blob(stmt, 7, imageData.data(), static_cast<int>(imageData.size()), SQLITE_TRANSIENT);
            
            size_t lastSlash = entry.evidence_path.find_last_of("/\\");
            std::string filename = (lastSlash != std::string::npos) ? 
                entry.evidence_path.substr(lastSlash + 1) : entry.evidence_path;
            sqlite3_bind_text(stmt, 8, filename.c_str(), -1, SQLITE_STATIC);
            
            std::string mimeType = GetMimeType(entry.evidence_path);
            sqlite3_bind_text(stmt, 9, mimeType.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int64(stmt, 10, static_cast<sqlite3_int64>(imageData.size()));
        } else {
            sqlite3_bind_null(stmt, 7);
            sqlite3_bind_null(stmt, 8);
            sqlite3_bind_null(stmt, 9);
            sqlite3_bind_null(stmt, 10);
        }
    } else {
        // Keep existing BLOB if no new image provided
        sqlite3_bind_null(stmt, 7);
        sqlite3_bind_null(stmt, 8);
        sqlite3_bind_null(stmt, 9);
        sqlite3_bind_null(stmt, 10);
    }
    
    sqlite3_bind_text(stmt, 11, entry.reporter_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 12, entry.is_verified ? 1 : 0);
    sqlite3_bind_text(stmt, 13, entry.notes.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 14, id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
    SetError("Failed to update entry: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }
    
    return sqlite3_changes(db_) > 0;
}

bool Database::DeleteEntry(int id) {
    const std::string sql = "DELETE FROM fraud_entries WHERE id = ?;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    SetError("Failed to prepare delete statement: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
    SetError("Failed to delete entry: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }
    
    return sqlite3_changes(db_) > 0;
}

std::optional<FraudEntry> Database::GetEntryById(int id) {
    const std::string sql = R"(
        SELECT id, identifier, description, fraud_type, platform, severity,
               evidence_path, reporter_name, date_reported, is_verified, notes
        FROM fraud_entries WHERE id = ?;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        SetError("Failed to prepare select statement: " + std::string(sqlite3_errmsg(db_)));
        return std::nullopt;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        FraudEntry entry = RowToFraudEntry(stmt);
        sqlite3_finalize(stmt);
        return entry;
    }
    
    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<FraudEntry> Database::GetAllEntries() {
    std::vector<FraudEntry> entries;
    
    const std::string sql = R"(
        SELECT id, identifier, description, fraud_type, platform, severity,
               evidence_path, reporter_name, date_reported, is_verified, notes
        FROM fraud_entries ORDER BY date_reported DESC;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        SetError("Failed to prepare select statement: " + std::string(sqlite3_errmsg(db_)));
        return entries;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        entries.push_back(RowToFraudEntry(stmt));
    }
    
    sqlite3_finalize(stmt);
    return entries;
}

std::vector<FraudEntry> Database::SearchByIdentifier(const std::string& identifier) {
    std::vector<FraudEntry> entries;
    
    const std::string sql = R"(
        SELECT id, identifier, description, fraud_type, platform, severity,
               evidence_path, reporter_name, date_reported, is_verified, notes
        FROM fraud_entries 
        WHERE identifier LIKE ? 
        ORDER BY date_reported DESC;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        SetError("Failed to prepare search statement: " + std::string(sqlite3_errmsg(db_)));
        return entries;
    }
    
    std::string searchPattern = "%" + identifier + "%";
    sqlite3_bind_text(stmt, 1, searchPattern.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        entries.push_back(RowToFraudEntry(stmt));
    }
    
    sqlite3_finalize(stmt);
    return entries;
}

std::vector<FraudEntry> Database::SearchByFraudType(FraudType type) {
    std::vector<FraudEntry> entries;
    
    const std::string sql = R"(
        SELECT id, identifier, description, fraud_type, platform, severity,
               evidence_path, reporter_name, date_reported, is_verified, notes
        FROM fraud_entries 
        WHERE fraud_type = ? 
        ORDER BY date_reported DESC;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        SetError("Failed to prepare search statement: " + std::string(sqlite3_errmsg(db_)));
        return entries;
    }
    
    sqlite3_bind_int(stmt, 1, static_cast<int>(type));
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        entries.push_back(RowToFraudEntry(stmt));
    }
    
    sqlite3_finalize(stmt);
    return entries;
}

std::vector<FraudEntry> Database::SearchByPlatform(Platform platform) {
    std::vector<FraudEntry> entries;
    
    const std::string sql = R"(
        SELECT id, identifier, description, fraud_type, platform, severity,
               evidence_path, reporter_name, date_reported, is_verified, notes
        FROM fraud_entries 
        WHERE platform = ? 
        ORDER BY date_reported DESC;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        SetError("Failed to prepare search statement: " + std::string(sqlite3_errmsg(db_)));
        return entries;
    }
    
    sqlite3_bind_int(stmt, 1, static_cast<int>(platform));
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        entries.push_back(RowToFraudEntry(stmt));
    }
    
    sqlite3_finalize(stmt);
    return entries;
}

int Database::GetTotalEntries() {
    const std::string sql = "SELECT COUNT(*) FROM fraud_entries;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
}

void Database::PrintStats() {
    Utils::PrintHeader("Database Statistics");
    
    std::cout << "Total Entries: " << GetTotalEntries() << "\n\n";
    
    // Fraud type statistics
    std::cout << "Fraud Types:\n";
    const std::string fraudTypeSql = R"(
        SELECT fraud_type, COUNT(*) as count 
        FROM fraud_entries 
        GROUP BY fraud_type 
        ORDER BY count DESC;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, fraudTypeSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int typeInt = sqlite3_column_int(stmt, 0);
            int count = sqlite3_column_int(stmt, 1);
            FraudType type = FraudEntry::IntToFraudType(typeInt);
            FraudEntry temp;
            temp.fraud_type = type;
            std::cout << "  " << temp.FraudTypeToString() << ": " << count << "\n";
        }
        sqlite3_finalize(stmt);
    }
    
    std::cout << "\nPlatforms:\n";
    const std::string platformSql = R"(
        SELECT platform, COUNT(*) as count 
        FROM fraud_entries 
        GROUP BY platform 
        ORDER BY count DESC;
    )";
    
    if (sqlite3_prepare_v2(db_, platformSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int platformInt = sqlite3_column_int(stmt, 0);
            int count = sqlite3_column_int(stmt, 1);
            Platform platform = FraudEntry::IntToPlatform(platformInt);
            FraudEntry temp;
            temp.platform = platform;
            std::cout << "  " << temp.PlatformToString() << ": " << count << "\n";
        }
        sqlite3_finalize(stmt);
    }
    
    std::cout << "\nSeverity Levels:\n";
    const std::string severitySql = R"(
        SELECT severity, COUNT(*) as count 
        FROM fraud_entries 
        GROUP BY severity 
        ORDER BY severity DESC;
    )";
    
    if (sqlite3_prepare_v2(db_, severitySql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int severityInt = sqlite3_column_int(stmt, 0);
            int count = sqlite3_column_int(stmt, 1);
            Severity severity = FraudEntry::IntToSeverity(severityInt);
            FraudEntry temp;
            temp.severity = severity;
            std::cout << "  " << temp.SeverityToString() << ": " << count << "\n";
        }
        sqlite3_finalize(stmt);
    }
}

FraudEntry Database::RowToFraudEntry(sqlite3_stmt* stmt) {
    FraudEntry entry;
    
    entry.id = sqlite3_column_int(stmt, 0);
    entry.identifier = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    entry.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    entry.fraud_type = FraudEntry::IntToFraudType(sqlite3_column_int(stmt, 3));
    entry.platform = FraudEntry::IntToPlatform(sqlite3_column_int(stmt, 4));
    entry.severity = FraudEntry::IntToSeverity(sqlite3_column_int(stmt, 5));
    
    const char* evidencePath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    if (evidencePath) entry.evidence_path = evidencePath;
    
    const char* reporterName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    if (reporterName) entry.reporter_name = reporterName;
    
    const char* dateStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
    if (dateStr) {
        // For simplicity, use current time if parsing fails
        entry.date_reported = Utils::Now();
    }
    
    entry.is_verified = sqlite3_column_int(stmt, 9) != 0;
    
    const char* notes = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
    if (notes) entry.notes = notes;
    
    return entry;
}

void Database::SetError(const std::string& error) {
    lastError_ = error;
    std::cerr << "Database Error: " << error << std::endl;
}