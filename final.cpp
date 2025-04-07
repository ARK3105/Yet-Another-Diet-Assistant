#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <stack>
#include <ctime>
#include <sstream>
#include <cctype>
#include "json.hpp"
using json = nlohmann::json;

// Forward declarations
class Food;
class BasicFood;
class CompositeFood;
struct FoodComponent;
class FoodDatabaseManager;

// Existing code for Food, BasicFood, CompositeFood, and FoodComponent

// New class for food log entries
class LogEntry {
public:
    std::shared_ptr<Food> food;
    double servings;
    
    LogEntry(std::shared_ptr<Food> f, double s) : food(f), servings(s) {}
    
    json toJson() const {
        json j;
        j["name"] = food->getName();
        j["servings"] = servings;
        return j;
    }
    
    void display() const {
        std::cout << servings << " serving(s) of " << food->getName() 
                  << " (" << food->getCalories() * servings << " calories)" << std::endl;
    }
};

// Class for managing logs for a specific date
class DailyLog {
private:
    std::vector<LogEntry> entries;
    
public:
    DailyLog() {}
    
    void addEntry(std::shared_ptr<Food> food, double servings) {
        entries.emplace_back(food, servings);
    }
    
    void removeEntry(int index) {
        if (index >= 0 && index < entries.size()) {
            entries.erase(entries.begin() + index);
        }
    }
    
    const std::vector<LogEntry>& getEntries() const {
        return entries;
    }
    
    double getTotalCalories() const {
        double total = 0.0;
        for (const auto& entry : entries) {
            total += entry.food->getCalories() * entry.servings;
        }
        return total;
    }
    
    json toJson() const {
        json j = json::array();
        for (const auto& entry : entries) {
            j.push_back(entry.toJson());
        }
        return j;
    }
    
    void displaySummary() const {
        std::cout << "Total entries: " << entries.size() << std::endl;
        std::cout << "Total calories: " << getTotalCalories() << std::endl;
    }
    
    void displayEntries() const {
        if (entries.empty()) {
            std::cout << "No entries for this day." << std::endl;
            return;
        }
        
        std::cout << "Food Log Entries:" << std::endl;
        std::cout << "-------------------" << std::endl;
        for (size_t i = 0; i < entries.size(); ++i) {
            std::cout << "[" << i << "] ";
            entries[i].display();
        }
        std::cout << "-------------------" << std::endl;
        std::cout << "Total calories: " << getTotalCalories() << std::endl;
    }
};

// Command interface for undo functionality
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string getDescription() const = 0;
};

// Concrete commands for log operations
class AddEntryCommand : public Command {
private:
    std::unordered_map<std::string, DailyLog>& logs;
    std::string date;
    std::shared_ptr<Food> food;
    double servings;
    bool executed;
    
public:
    AddEntryCommand(std::unordered_map<std::string, DailyLog>& l, const std::string& d, 
                    std::shared_ptr<Food> f, double s)
        : logs(l), date(d), food(f), servings(s), executed(false) {}
    
    void execute() override {
        logs[date].addEntry(food, servings);
        executed = true;
    }
    
    void undo() override {
        if (executed) {
            auto& entries = const_cast<std::vector<LogEntry>&>(logs[date].getEntries());
            if (!entries.empty()) {
                entries.pop_back();
            }
            executed = false;
        }
    }
    
    std::string getDescription() const override {
        std::stringstream ss;
        ss << "Add " << servings << " serving(s) of " << food->getName() << " to " << date;
        return ss.str();
    }
};

class RemoveEntryCommand : public Command {
private:
    std::unordered_map<std::string, DailyLog>& logs;
    std::string date;
    int index;
    LogEntry removedEntry;
    bool executed;
    
public:
    RemoveEntryCommand(std::unordered_map<std::string, DailyLog>& l, const std::string& d, 
                       int idx, const LogEntry& entry)
        : logs(l), date(d), index(idx), removedEntry(entry), executed(false) {}
    
    void execute() override {
        logs[date].removeEntry(index);
        executed = true;
    }
    
    void undo() override {
        if (executed) {
            logs[date].addEntry(removedEntry.food, removedEntry.servings);
            executed = false;
        }
    }
    
    std::string getDescription() const override {
        std::stringstream ss;
        ss << "Remove " << removedEntry.servings << " serving(s) of " 
           << removedEntry.food->getName() << " from " << date;
        return ss.str();
    }
};

// Class to manage all daily logs
class LogManager {
private:
    std::unordered_map<std::string, DailyLog> logs;
    std::string currentDate;
    std::stack<std::unique_ptr<Command>> undoStack;
    std::stack<std::unique_ptr<Command>> redoStack;
    FoodDatabaseManager& dbManager;
    std::string logFilePath;
    
    std::string getCurrentDateString() {
        time_t now = time(0);
        tm* ltm = localtime(&now);
        
        std::ostringstream oss;
        oss << 1900 + ltm->tm_year << "-";
        oss << std::setw(2) << std::setfill('0') << 1 + ltm->tm_mon << "-";
        oss << std::setw(2) << std::setfill('0') << ltm->tm_mday;
        
        return oss.str();
    }
    
    bool validateDateFormat(const std::string& date) {
        // Basic validation for YYYY-MM-DD format
        if (date.length() != 10) return false;
        if (date[4] != '-' || date[7] != '-') return false;
        
        for (int i = 0; i < 10; i++) {
            if (i == 4 || i == 7) continue;
            if (!std::isdigit(date[i])) return false;
        }
        
        return true;
    }
    
public:
    LogManager(FoodDatabaseManager& manager, const std::string& logFile)
        : dbManager(manager), logFilePath(logFile) {
        currentDate = getCurrentDateString();
        loadLogs();
    }
    
    ~LogManager() {
        saveLogs();
    }
    
    void executeCommand(std::unique_ptr<Command> cmd) {
        cmd->execute();
        undoStack.push(std::move(cmd));
        
        // Clear redo stack when a new command is executed
        while (!redoStack.empty()) {
            redoStack.pop();
        }
    }
    
    void undo() {
        if (undoStack.empty()) {
            std::cout << "Nothing to undo." << std::endl;
            return;
        }
        
        std::cout << "Undoing: " << undoStack.top()->getDescription() << std::endl;
        undoStack.top()->undo();
        redoStack.push(std::move(undoStack.top()));
        undoStack.pop();
    }
    
    void redo() {
        if (redoStack.empty()) {
            std::cout << "Nothing to redo." << std::endl;
            return;
        }
        
        std::cout << "Redoing: " << redoStack.top()->getDescription() << std::endl;
        redoStack.top()->execute();
        undoStack.push(std::move(redoStack.top()));
        redoStack.pop();
    }
    
    void addFoodToLog(const std::string& date, std::shared_ptr<Food> food, double servings) {
        auto cmd = std::make_unique<AddEntryCommand>(logs, date, food, servings);
        executeCommand(std::move(cmd));
    }
    
    void removeFoodFromLog(const std::string& date, int index) {
        if (logs.find(date) != logs.end()) {
            const auto& entries = logs[date].getEntries();
            if (index >= 0 && index < entries.size()) {
                auto cmd = std::make_unique<RemoveEntryCommand>(logs, date, index, entries[index]);
                executeCommand(std::move(cmd));
            } else {
                std::cout << "Invalid entry index." << std::endl;
            }
        } else {
            std::cout << "No log found for date: " << date << std::endl;
        }
    }
    
    void setCurrentDate(const std::string& date) {
        if (validateDateFormat(date)) {
            currentDate = date;
            std::cout << "Current date set to: " << currentDate << std::endl;
        } else {
            std::cout << "Invalid date format. Please use YYYY-MM-DD format." << std::endl;
        }
    }
    
    std::string getCurrentDate() const {
        return currentDate;
    }
    
    void displayCurrentLog() const {
        std::cout << "Log for " << currentDate << ":" << std::endl;
        if (logs.find(currentDate) != logs.end()) {
            logs.at(currentDate).displayEntries();
        } else {
            std::cout << "No entries for today." << std::endl;
        }
    }
    
    void displayLogForDate(const std::string& date) const {
        std::cout << "Log for " << date << ":" << std::endl;
        if (logs.find(date) != logs.end()) {
            logs.at(date).displayEntries();
        } else {
            std::cout << "No entries for this date." << std::endl;
        }
    }
    
    void loadLogs() {
        try {
            std::ifstream file(logFilePath);
            if (!file.is_open()) {
                std::cout << "No existing log file found. Starting with empty logs." << std::endl;
                return;
            }
            
            json j;
            file >> j;
            
            for (const auto& [date, entries] : j.items()) {
                for (const auto& entry : entries) {
                    std::string foodName = entry["name"];
                    double servings = entry["servings"];
                    
                    auto food = dbManager.findFoodByName(foodName);
                    if (food) {
                        logs[date].addEntry(food, servings);
                    }
                }
            }
            
            std::cout << "Logs loaded successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Error loading logs: " << e.what() << std::endl;
        }
    }
    
    void saveLogs() {
        try {
            json j;
            for (const auto& [date, log] : logs) {
                j[date] = log.toJson();
            }
            
            std::ofstream file(logFilePath);
            file << j.dump(2);
            std::cout << "Logs saved successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Error saving logs: " << e.what() << std::endl;
        }
    }
    
    void showUndoHistory() const {
        if (undoStack.empty()) {
            std::cout << "No actions to undo." << std::endl;
            return;
        }
        
        std::cout << "Actions that can be undone:" << std::endl;
        std::cout << "-------------------------" << std::endl;
        
        std::stack<std::unique_ptr<Command>> tempStack;
        int count = 0;
        
        // Use a temporary container to not modify the original stack
        while (!const_cast<std::stack<std::unique_ptr<Command>>&>(undoStack).empty()) {
            auto& cmd = const_cast<std::stack<std::unique_ptr<Command>>&>(undoStack).top();
            std::cout << count++ << ": " << cmd->getDescription() << std::endl;
            tempStack.push(std::move(const_cast<std::stack<std::unique_ptr<Command>>&>(undoStack).top()));
            const_cast<std::stack<std::unique_ptr<Command>>&>(undoStack).pop();
        }
        
        // Restore the original stack
        while (!tempStack.empty()) {
            const_cast<std::stack<std::unique_ptr<Command>>&>(undoStack).push(std::move(tempStack.top()));
            tempStack.pop();
        }
    }
};

// Extensions to DietAssistantCLI class
class DietAssistantCLI {
private:
    FoodDatabaseManager& dbManager;
    LogManager logManager;
    bool running;
    
    void displayMainMenu() {
        std::cout << "\n===== Diet Assistant Menu =====" << std::endl;
        std::cout << "1. Search foods" << std::endl;
        std::cout << "2. View food details" << std::endl;
        std::cout << "3. Add basic food" << std::endl;
        std::cout << "4. Create composite food" << std::endl;
        std::cout << "5. List all foods" << std::endl;
        std::cout << "6. Save database" << std::endl;
        std::cout << "7. View today's log (" << logManager.getCurrentDate() << ")" << std::endl;
        std::cout << "8. Add food to log" << std::endl;
        std::cout << "9. Remove food from log" << std::endl;
        std::cout << "10. Change active date" << std::endl;
        std::cout << "11. View log for specific date" << std::endl;
        std::cout << "12. Undo last action" << std::endl;
        std::cout << "13. Show undo history" << std::endl;
        std::cout << "14. Save logs" << std::endl;
        std::cout << "0. Exit" << std::endl;
        std::cout << "Choose an option: ";
    }
    
    void handleSearch() {
        std::cout << "\n==== Search Foods ====" << std::endl;
        std::cout << "1. Search by name" << std::endl;
        std::cout << "2. Search by keywords (match any)" << std::endl;
        std::cout << "3. Search by keywords (match all)" << std::endl;
        std::cout << "0. Back to main menu" << std::endl;
        std::cout << "Choose an option: ";
        
        int option;
        std::cin >> option;
        std::cin.ignore();
        
        if (option == 0) return;
        
        std::vector<std::shared_ptr<Food>> results;
        
        if (option == 1) {
            std::cout << "Enter name to search: ";
            std::string name;
            std::getline(std::cin, name);
            results = dbManager.searchFoodsByName(name);
        } else if (option == 2 || option == 3) {
            std::cout << "Enter keywords (separated by spaces): ";
            std::string keywordsStr;
            std::getline(std::cin, keywordsStr);
            
            std::vector<std::string> keywords;
            std::istringstream iss(keywordsStr);
            std::string keyword;
            while (iss >> keyword) {
                keywords.push_back(keyword);
            }
            
            bool matchAll = (option == 3);
            results = dbManager.searchFoodsByKeywords(keywords, matchAll);
        }
        
        if (results.empty()) {
            std::cout << "No matching foods found." << std::endl;
        } else {
            std::cout << "\nFound " << results.size() << " matching foods:" << std::endl;
            for (size_t i = 0; i < results.size(); ++i) {
                std::cout << "[" << i << "] ";
                results[i]->display();
            }
        }
    }
    
    void handleViewFoodDetails() {
        std::cout << "\nEnter the name of the food to view: ";
        std::string name;
        std::getline(std::cin, name);
        
        auto food = dbManager.findFoodByName(name);
        if (food) {
            std::cout << "\n==== Food Details ====" << std::endl;
            food->display();
        } else {
            std::cout << "Food not found: " << name << std::endl;
        }
    }
    
    void handleAddBasicFood() {
        std::cout << "\n==== Add Basic Food ====" << std::endl;
        
        std::string name;
        double calories;
        std::string keywordsStr;
        
        std::cout << "Name: ";
        std::getline(std::cin, name);
        
        std::cout << "Calories per serving: ";
        std::cin >> calories;
        std::cin.ignore();
        
        std::cout << "Keywords (separated by spaces): ";
        std::getline(std::cin, keywordsStr);
        
        std::vector<std::string> keywords;
        std::istringstream iss(keywordsStr);
        std::string keyword;
        while (iss >> keyword) {
            keywords.push_back(keyword);
        }
        
        if (dbManager.addBasicFood(name, calories, keywords)) {
            std::cout << "Basic food added successfully: " << name << std::endl;
        } else {
            std::cout << "Failed to add food (might already exist)." << std::endl;
        }
    }
    
    void handleCreateCompositeFood() {
        std::cout << "\n==== Create Composite Food ====" << std::endl;
        
        std::string name;
        std::string keywordsStr;
        std::vector<FoodComponent> components;
        
        std::cout << "Name: ";
        std::getline(std::cin, name);
        
        std::cout << "Keywords (separated by spaces): ";
        std::getline(std::cin, keywordsStr);
        
        std::vector<std::string> keywords;
        std::istringstream iss(keywordsStr);
        std::string keyword;
        while (iss >> keyword) {
            keywords.push_back(keyword);
        }
        
        std::cout << "\nAdd components (enter empty name to finish):" << std::endl;
        while (true) {
            std::string componentName;
            double servings;
            
            std::cout << "Component name: ";
            std::getline(std::cin, componentName);
            
            if (componentName.empty()) break;
            
            auto component = dbManager.findFoodByName(componentName);
            if (!component) {
                std::cout << "Food not found: " << componentName << std::endl;
                continue;
            }
            
            std::cout << "Servings: ";
            std::cin >> servings;
            std::cin.ignore();
            
            components.push_back({component, servings});
            std::cout << "Component added: " << componentName << " (" << servings << " servings)" << std::endl;
        }
        
        if (components.empty()) {
            std::cout << "No components added. Operation cancelled." << std::endl;
            return;
        }
        
        if (dbManager.addCompositeFood(name, keywords, components)) {
            std::cout << "Composite food added successfully: " << name << std::endl;
        } else {
            std::cout << "Failed to add food (might already exist)." << std::endl;
        }
    }
    
    void handleListAllFoods() {
        dbManager.listAllFoods();
    }
    
    void handleSaveDatabase() {
        dbManager.saveDatabase();
    }
    
    void handleViewTodaysLog() {
        logManager.displayCurrentLog();
    }
    
    void handleAddFoodToLog() {
        std::cout << "\n==== Add Food to Log (" << logManager.getCurrentDate() << ") ====" << std::endl;
        
        // Search for food first
        std::cout << "\n1. Search by name" << std::endl;
        std::cout << "2. Search by keywords (match any)" << std::endl;
        std::cout << "3. Search by keywords (match all)" << std::endl;
        std::cout << "0. Cancel" << std::endl;
        std::cout << "Choose an option: ";
        
        int option;
        std::cin >> option;
        std::cin.ignore();
        
        if (option == 0) return;
        
        std::vector<std::shared_ptr<Food>> results;
        
        if (option == 1) {
            std::cout << "Enter name to search: ";
            std::string name;
            std::getline(std::cin, name);
            results = dbManager.searchFoodsByName(name);
        } else if (option == 2 || option == 3) {
            std::cout << "Enter keywords (separated by spaces): ";
            std::string keywordsStr;
            std::getline(std::cin, keywordsStr);
            
            std::vector<std::string> keywords;
            std::istringstream iss(keywordsStr);
            std::string keyword;
            while (iss >> keyword) {
                keywords.push_back(keyword);
            }
            
            bool matchAll = (option == 3);
            results = dbManager.searchFoodsByKeywords(keywords, matchAll);
        }
        
        if (results.empty()) {
            std::cout << "No matching foods found." << std::endl;
            return;
        }
        
        std::cout << "\nFound " << results.size() << " matching foods:" << std::endl;
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "[" << i << "] ";
            results[i]->display();
        }
        
        std::cout << "\nSelect food by number: ";
        int foodIndex;
        std::cin >> foodIndex;
        
        if (foodIndex < 0 || foodIndex >= results.size()) {
            std::cout << "Invalid selection." << std::endl;
            std::cin.ignore();
            return;
        }
        
        std::cout << "Enter number of servings: ";
        double servings;
        std::cin >> servings;
        std::cin.ignore();
        
        logManager.addFoodToLog(logManager.getCurrentDate(), results[foodIndex], servings);
        std::cout << servings << " serving(s) of " << results[foodIndex]->getName() 
                  << " added to log for " << logManager.getCurrentDate() << std::endl;
    }
    
    void handleRemoveFoodFromLog() {
        std::cout << "\n==== Remove Food from Log (" << logManager.getCurrentDate() << ") ====" << std::endl;
        
        logManager.displayCurrentLog();
        
        std::cout << "\nEnter index of food to remove (or -1 to cancel): ";
        int index;
        std::cin >> index;
        std::cin.ignore();
        
        if (index == -1) return;
        
        logManager.removeFoodFromLog(logManager.getCurrentDate(), index);
    }
    
    void handleChangeActiveDate() {
        std::cout << "\n==== Change Active Date ====" << std::endl;
        std::cout << "Current active date: " << logManager.getCurrentDate() << std::endl;
        std::cout << "Enter new date (YYYY-MM-DD): ";
        
        std::string date;
        std::getline(std::cin, date);
        
        logManager.setCurrentDate(date);
    }
    
    void handleViewLogForDate() {
        std::cout << "\n==== View Log for Specific Date ====" << std::endl;
        std::cout << "Enter date (YYYY-MM-DD): ";
        
        std::string date;
        std::getline(std::cin, date);
        
        logManager.displayLogForDate(date);
    }
    
    void handleUndo() {
        logManager.undo();
    }
    
    void handleShowUndoHistory() {
        logManager.showUndoHistory();
    }
    
    void handleSaveLogs() {
        logManager.saveLogs();
    }
    
public:
    DietAssistantCLI(FoodDatabaseManager& manager, const std::string& logFile)
        : dbManager(manager), logManager(manager, logFile), running(true) {}
    
    void run() {
        while (running) {
            displayMainMenu();
            
            int choice;
            std::cin >> choice;
            std::cin.ignore();
            
            switch (choice) {
                case 0:
                    running = false;
                    std::cout << "Saving logs and database before exit..." << std::endl;
                    logManager.saveLogs();
                    dbManager.saveDatabase();
                    std::cout << "Goodbye!" << std::endl;
                    break;
                case 1:
                    handleSearch();
                    break;
                case 2:
                    handleViewFoodDetails();
                    break;
                case 3:
                    handleAddBasicFood();
                    break;
                case 4:
                    handleCreateCompositeFood();
                    break;
                case 5:
                    handleListAllFoods();
                    break;
                case 6:
                    handleSaveDatabase();
                    break;
                case 7:
                    handleViewTodaysLog();
                    break;
                case 8:
                    handleAddFoodToLog();
                    break;
                case 9:
                    handleRemoveFoodFromLog();
                    break;
                case 10:
                    handleChangeActiveDate();
                    break;
                case 11:
                    handleViewLogForDate();
                    break;
                case 12:
                    handleUndo();
                    break;
                case 13:
                    handleShowUndoHistory();
                    break;
                case 14:
                    handleSaveLogs();
                    break;
                default:
                    std::cout << "Invalid option. Please try again." << std::endl;
            }
        }
    }
};

// Main function (assuming you already have it)
int main() {
    std::string dbFile = "foods.json";
    std::string logFile = "food_logs.json";
    
    FoodDatabaseManager dbManager(dbFile);
    dbManager.loadDatabase();
    
    DietAssistantCLI cli(dbManager, logFile);
    cli.run();
    
    return 0;
}