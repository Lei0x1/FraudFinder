#include "fraud_entry.h"
#include "utils.h"
#include <iostream>
#include <iomanip>
#include <sstream>

std::string FraudEntry::FraudTypeToString() const {
    switch (fraud_type) {
        case FraudType::SCAM: return "Scam";
        case FraudType::PHISHING: return "Phishing";
        case FraudType::IDENTITY_THEFT: return "Identity Theft";
        case FraudType::INVESTMENT_FRAUD: return "Investment Fraud";
        case FraudType::ROMANCE_SCAM: return "Romance Scam";
        case FraudType::TECH_SUPPORT_SCAM: return "Tech Support Scam";
        case FraudType::OTHER: return "Other";
        default: return "Unknown";
    }
}

std::string FraudEntry::PlatformToString() const {
    switch (platform) {
        case Platform::DISCORD: return "Discord";
        case Platform::TELEGRAM: return "Telegram";
        case Platform::WHATSAPP: return "WhatsApp";
        case Platform::EMAIL: return "Email";
        case Platform::PHONE: return "Phone";
        case Platform::WEBSITE: return "Website";
        case Platform::SOCIAL_MEDIA: return "Social Media";
        case Platform::OTHER: return "Other";
        default: return "Unknown";
    }
}

std::string FraudEntry::SeverityToString() const {
    switch (severity) {
        case Severity::LOW: return "Low";
        case Severity::MEDIUM: return "Medium";
        case Severity::HIGH: return "High";
        case Severity::CRITICAL: return "Critical";
        default: return "Unknown";
    }
}

std::string FraudEntry::DateToString() const {
    return Utils::TimeToString(date_reported);
}

FraudType FraudEntry::IntToFraudType(int value) {
    if (value >= 1 && value <= 7) {
        return static_cast<FraudType>(value);
    }
    return FraudType::OTHER;
}

Platform FraudEntry::IntToPlatform(int value) {
    if (value >= 1 && value <= 8) {
        return static_cast<Platform>(value);
    }
    return Platform::OTHER;
}

Severity FraudEntry::IntToSeverity(int value) {
    if (value >= 1 && value <= 4) {
        return static_cast<Severity>(value);
    }
    return Severity::LOW;
}

void FraudEntry::PrintFraudTypeOptions() {
    std::cout << "Fraud Types:\n";
    std::cout << "1. Scam\n";
    std::cout << "2. Phishing\n";
    std::cout << "3. Identity Theft\n";
    std::cout << "4. Investment Fraud\n";
    std::cout << "5. Romance Scam\n";
    std::cout << "6. Tech Support Scam\n";
    std::cout << "7. Other\n";
}

void FraudEntry::PrintPlatformOptions() {
    std::cout << "Platforms:\n";
    std::cout << "1. Discord\n";
    std::cout << "2. Telegram\n";
    std::cout << "3. WhatsApp\n";
    std::cout << "4. Email\n";
    std::cout << "5. Phone\n";
    std::cout << "6. Website\n";
    std::cout << "7. Social Media\n";
    std::cout << "8. Other\n";
}

void FraudEntry::PrintSeverityOptions() {
    std::cout << "Severity Levels:\n";
    std::cout << "1. Low\n";
    std::cout << "2. Medium\n";
    std::cout << "3. High\n";
    std::cout << "4. Critical\n";
}