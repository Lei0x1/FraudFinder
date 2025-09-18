#include "console_ui.h"
#include "database.h"
#include "utils.h"
#include <iostream>
#include <memory>
#include <exception>
#include <io.h>
#include <fcntl.h>

int main() {
    try {
        // Initialize database
        auto database = std::make_shared<Database>("fraudfinder.db");
        
        if (!database->isConnected()) {
            Utils::SetColor(Utils::RED);
            std::cerr << "Failed to connect to database: " 
                      << database->getLastError() << std::endl;
            Utils::ResetColor();
            
            std::cout << "Press Enter to exit...";
            std::cin.get();
            return 1;
        }
        
        // Start console UI
        ConsoleUI ui(database);
        ui.run();
        
    } catch (const std::exception& e) {
        Utils::SetColor(Utils::RED);
        std::cerr << "Fatal error: " << e.what() << std::endl;
        Utils::ResetColor();
        
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return 1;
    } catch (...) {
        Utils::SetColor(Utils::RED);
        std::cerr << "Unknown fatal error occurred." << std::endl;
        Utils::ResetColor();
        
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }
    
    return 0;
}
