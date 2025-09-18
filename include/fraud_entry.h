#ifndef FRAUD_ENTRY_H
#define FRAUD_ENTRY_H

#include <string>
#include <chrono>
#include <optional>

enum class FraudType {
    SCAM = 1,
    PHISHING = 2,
    IDENTITY_THEFT = 3,
    INVESTMENT_FRAUD = 4,
    ROMANCE_SCAM = 5,
    TECH_SUPPORT_SCAM = 6,
    OTHER = 7
};

enum class Platform {
    DISCORD = 1,
    TELEGRAM = 2,
    WHATSAPP = 3,
    EMAIL = 4,
    PHONE = 5,
    WEBSITE = 6,
    SOCIAL_MEDIA = 7,
    OTHER = 8
};

enum class Severity {
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4
};

struct FraudEntry {
    int id = 0;
    std::string identifier;
    std::string description;
    FraudType fraud_type = FraudType::OTHER;
    Platform platform = Platform::OTHER;
    Severity severity = Severity::LOW;
    std::string evidence_path;
    std::string reporter_name;
    std::chrono::system_clock::time_point date_reported;
    bool is_verified = false;
    std::string notes;

    // Display helpers
    std::string FraudTypeToString() const;
    std::string PlatformToString() const;
    std::string SeverityToString() const;
    std::string DateToString() const;
    
    static FraudType IntToFraudType(int value);
    static Platform IntToPlatform(int value);
    static Severity IntToSeverity(int value);
    
    static void PrintFraudTypeOptions();
    static void PrintPlatformOptions();
    static void PrintSeverityOptions();
};

#endif