#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <limits>
#include <stack>
#include <ctime>
#include <sstream>
#include <cctype>

// Include nlohmann/json for JSON handling
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// Forward declarations
class Food;
class BasicFood;
class CompositeFood;

// Base Food class
class Food {
protected:
    string name;
    vector<string> keywords;
    string type;

public:
    Food(const string& name, const vector<string>& keywords, const string& type)
        : name(name), keywords(keywords), type(type) {}
    
    virtual ~Food() = default;
    
    virtual float getCalories() const = 0;
    
    const string& getName() const { return name; }
    const vector<string>& getKeywords() const { return keywords; }
    const string& getType() const { return type; }
    
    virtual json toJson() const {
        json j;
        j["name"] = name;
        j["keywords"] = keywords;
        j["type"] = type;
        j["calories"] = getCalories();
        return j;
    }
    
    virtual void display() const {
        cout << "Name: " << name << endl;
        cout << "Type: " << type << endl;
        cout << "Calories: " << getCalories() << endl;
        cout << "Keywords: ";
        for (size_t i = 0; i < keywords.size(); ++i) {
            cout << keywords[i];
            if (i < keywords.size() - 1) cout << ", ";
        }
        cout << endl;
    }
};

// Basic Food class
class BasicFood : public Food {
private:
    float calories;

public:
    BasicFood(const string& name, const vector<string>& keywords, float calories)
        : Food(name, keywords, "basic"), calories(calories) {}
    
    float getCalories() const override { return calories; }
    
    static shared_ptr<BasicFood> fromJson(const json& j) {
        string name = j["name"];
        vector<string> keywords = j["keywords"].get<vector<string>>();
        float calories = j["calories"];
        return make_shared<BasicFood>(name, keywords, calories);
    }
};

// Component for Composite Food
struct FoodComponent {
    shared_ptr<Food> food;
    float servings;
    
    FoodComponent(shared_ptr<Food> food, float servings)
        : food(food), servings(servings) {}
    
    json toJson() const {
        json j;
        j["name"] = food->getName();
        j["servings"] = servings;
        return j;
    }
};

// Composite Food class
class CompositeFood : public Food {
private:
    vector<FoodComponent> components;

public:
    CompositeFood(const string& name, const vector<string>& keywords, const vector<FoodComponent>& components)
        : Food(name, keywords, "composite"), components(components) {}
    
    float getCalories() const override {
        float totalCalories = 0.0f;
        for (const auto& component : components) {
            totalCalories += component.food->getCalories() * component.servings;
        }
        return totalCalories;
    }
    
    json toJson() const override {
        json j = Food::toJson();
        json componentsJson = json::array();
        
        for (const auto& component : components) {
            componentsJson.push_back(component.toJson());
        }
        
        j["components"] = componentsJson;
        return j;
    }
    
    void display() const override {
        Food::display();
        cout << "Components:" << endl;
        for (const auto& component : components) {
            cout << "  - " << component.food->getName() 
                 << " (" << component.servings << " serving" 
                 << (component.servings > 1 ? "s" : "") << ")" << endl;
        }
    }
    
    static shared_ptr<CompositeFood> createFromComponents(
        const string& name, 
        const vector<string>& keywords,
        const vector<FoodComponent>& components) {
        return make_shared<CompositeFood>(name, keywords, components);
    }
};

// Food Database Manager class
class FoodDatabaseManager {
private:
    unordered_map<string, shared_ptr<Food>> foods;
    string databaseFilePath;
    bool modified;
    
    void clear() {
        foods.clear();
    }

public:
    FoodDatabaseManager(const string& filePath = "food_database.json") 
        : databaseFilePath(filePath), modified(false) {}
    
        bool loadDatabase() {
            clear();
            
            ifstream file(databaseFilePath);
            if (!file.is_open()) {
                cout << "No existing database found. Starting with empty database." << endl;
                return false;
            }
            
            try {
                json j;
                file >> j;
                
                // Store the entire JSON data for each food
                unordered_map<string, json> pendingFoods;
                
                // First pass: load all basic foods and catalogue composite foods
                for (const auto& foodJson : j) {
                    string type = foodJson["type"];
                    string name = foodJson["name"];
                    
                    if (type == "basic") {
                        foods[name] = BasicFood::fromJson(foodJson);
                    } else if (type == "composite") {
                        pendingFoods[name] = foodJson;
                    }
                }
                
                // Function to recursively load a composite food and its dependencies
                function<shared_ptr<Food>(const string&)> loadCompositeFood = [&] (const string& name) -> shared_ptr<Food> {
                        // If already loaded, return it
                        if (foods.find(name) != foods.end()) {
                            return foods[name];
                        }
                        
                        // If not a pending composite food, can't load it
                        if (pendingFoods.find(name) == pendingFoods.end()) {
                            cout << "Warning: Food '" << name << "' not found." << endl;
                            return nullptr;
                        }
                        
                        // Get the food's JSON
                        json foodJson = pendingFoods[name];
                        
                        // Load all components
                        vector<FoodComponent> components;
                        for (const auto& componentJson : foodJson["components"]) {
                            string componentName = componentJson["name"];
                            float servings = componentJson["servings"];
                            
                            // Recursively load component if needed
                            shared_ptr<Food> componentFood;
                            if (foods.find(componentName) != foods.end()) {
                                componentFood = foods[componentName];
                            } else {
                                componentFood = loadCompositeFood(componentName);
                            }
                            
                            if (componentFood) {
                                components.emplace_back(componentFood, servings);
                            } else {
                                cout << "Warning: Component '" << componentName 
                                     << "' not found for composite food '" << name << "'" << endl;
                            }
                        }
                        
                        // Create the composite food
                        vector<string> keywords = foodJson["keywords"].get<vector<string>>();
                        shared_ptr<Food> food = make_shared<CompositeFood>(name, keywords, components);
                        
                        // Add it to loaded foods
                        foods[name] = food;
                        
                        return food;
                    };
                
                // Second pass: load all composite foods with dependencies
                for (const auto& [name, _] : pendingFoods) {
                    loadCompositeFood(name);
                }
                
                cout << "Database loaded: " << foods.size() << " foods." << endl;
                return true;
            } catch (const exception& e) {
                cout << "Error loading database: " << e.what() << endl;
                return false;
            }
        }
    
    bool saveDatabase() {
        try {
            json j = json::array();
            
            for (const auto& [name, food] : foods) {
                j.push_back(food->toJson());
            }
            
            ofstream file(databaseFilePath);
            if (!file.is_open()) {
                cout << "Error: Unable to open file for writing." << endl;
                return false;
            }
            
            file << j.dump(4);  // Pretty print with 4 spaces
            file.close();
            
            modified = false;
            cout << "Database saved to " << databaseFilePath << endl;
            return true;
        } catch (const exception& e) {
            cout << "Error saving database: " << e.what() << endl;
            return false;
        }
    }
    
    bool addFood(shared_ptr<Food> food) {
        string name = food->getName();
        if (foods.find(name) != foods.end()) {
            cout << "Error: A food with name '" << name << "' already exists." << endl;
            return false;
        }
        
        foods[name] = food;
        modified = true;
        return true;
    }
    
    vector<shared_ptr<Food>> searchFoods(const string& query) {
        vector<shared_ptr<Food>> results;
        string lowerQuery = query;
        transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
        
        for (const auto& [name, food] : foods) {
            // Check if query matches name
            string lowerName = name;
            transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            if (lowerName.find(lowerQuery) != string::npos) {
                results.push_back(food);
                continue;
            }
            
            // Check if query matches any keyword
            for (const auto& keyword : food->getKeywords()) {
                string lowerKeyword = keyword;
                transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);
                if (lowerKeyword.find(lowerQuery) != string::npos) {
                    results.push_back(food);
                    break;
                }
            }
        }
        
        return results;
    }
    
    shared_ptr<Food> getFood(const string& name) {
        auto it = foods.find(name);
        if (it != foods.end()) {
            return it->second;
        }
        return nullptr;
    }
    vector<shared_ptr<Food>> searchFoodsByKeywords(const vector<string>& keywords, bool matchAll) {
        vector<shared_ptr<Food>> results;

        for (const auto& [name, food] : foods) {
            const auto& foodKeywords = food->getKeywords();
            int matchCount = 0;

            for (const auto& keyword : keywords) {
                if (find(foodKeywords.begin(), foodKeywords.end(), keyword) != foodKeywords.end()) {
                    matchCount++;
                }
            }

            if ((matchAll && matchCount == keywords.size()) || (!matchAll && matchCount > 0)) {
                results.push_back(food);
            }
        }

        return results;
    }

    shared_ptr<Food> findFoodByName(const string& name) {
        return getFood(name);
    }
    
    void listAllFoods() const {
        cout << "\n=== All Foods in Database (" << foods.size() << ") ===" << endl;
        for (const auto& [name, food] : foods) {
            cout << name << " (" << food->getType() << ") - " << food->getCalories() << " calories" << endl;
        }
        cout << "===========================" << endl;
    }
    
    bool isModified() const {
        return modified;
    }
};






// New class for food log entries
class LogEntry {
    public:
        shared_ptr<Food> food;
        double servings;
        
        LogEntry(shared_ptr<Food> f, double s) : food(f), servings(s) {}
        
        json toJson() const {
            json j;
            j["name"] = food->getName();
            j["servings"] = servings;
            return j;
        }
        
        void display() const {
            cout << servings << " serving(s) of " << food->getName() 
                      << " (" << food->getCalories() * servings << " calories)" << endl;
        }
    };
    
    // Class for managing logs for a specific date
    class DailyLog {
    private:
        vector<LogEntry> entries;
        
    public:
        DailyLog() {}
        
        void addEntry(shared_ptr<Food> food, double servings) {
            entries.emplace_back(food, servings);
        }
        
        void removeEntry(int index) {
            if (index >= 0 && index < entries.size()) {
                entries.erase(entries.begin() + index);
            }
        }
        
        const vector<LogEntry>& getEntries() const {
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
            cout << "Total entries: " << entries.size() << endl;
            cout << "Total calories: " << getTotalCalories() << endl;
        }
        
        void displayEntries() const {
            if (entries.empty()) {
                cout << "No entries for this day." << endl;
                return;
            }
            
            cout << "Food Log Entries:" << endl;
            cout << "-------------------" << endl;
            for (size_t i = 0; i < entries.size(); ++i) {
                cout << "[" << i << "] ";
                entries[i].display();
            }
            cout << "-------------------" << endl;
            cout << "Total calories: " << getTotalCalories() << endl;
        }
    };
    
    // Command interface for undo functionality
    class Command {
    public:
        virtual ~Command() = default;
        virtual void execute() = 0;
        virtual void undo() = 0;
        virtual string getDescription() const = 0;
    };
    
    // Concrete commands for log operations
    class AddEntryCommand : public Command {
    private:
        unordered_map<string, DailyLog>& logs;
        string date;
        shared_ptr<Food> food;
        double servings;
        bool executed;
        
    public:
        AddEntryCommand(unordered_map<string, DailyLog>& l, const string& d, 
                        shared_ptr<Food> f, double s)
            : logs(l), date(d), food(f), servings(s), executed(false) {}
        
        void execute() override {
            logs[date].addEntry(food, servings);
            executed = true;
        }
        
        void undo() override {
            if (executed) {
                auto& entries = const_cast<vector<LogEntry>&>(logs[date].getEntries());
                if (!entries.empty()) {
                    entries.pop_back();
                }
                executed = false;
            }
        }
        
        string getDescription() const override {
            stringstream ss;
            ss << "Add " << servings << " serving(s) of " << food->getName() << " to " << date;
            return ss.str();
        }
    };
    
    class RemoveEntryCommand : public Command {
    private:
        unordered_map<string, DailyLog>& logs;
        string date;
        int index;
        LogEntry removedEntry;
        bool executed;
        
    public:
        RemoveEntryCommand(unordered_map<string, DailyLog>& l, const string& d, 
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
        
        string getDescription() const override {
            stringstream ss;
            ss << "Remove " << removedEntry.servings << " serving(s) of " 
               << removedEntry.food->getName() << " from " << date;
            return ss.str();
        }
    };
    
    // Class to manage all daily logs
    class LogManager {
    private:
        unordered_map<string, DailyLog> logs;
        string currentDate;
        stack<unique_ptr<Command>> undoStack;
        stack<unique_ptr<Command>> redoStack;
        FoodDatabaseManager& dbManager;
        string logFilePath;
        
        string getCurrentDateString() {
            time_t now = time(0);
            tm* ltm = localtime(&now);
            
            ostringstream oss;
            oss << 1900 + ltm->tm_year << "-";
            oss << setw(2) << setfill('0') << 1 + ltm->tm_mon << "-";
            oss << setw(2) << setfill('0') << ltm->tm_mday;
            
            return oss.str();
        }
        
        bool validateDateFormat(const string& date) {
            // Basic validation for YYYY-MM-DD format
            if (date.length() != 10) return false;
            if (date[4] != '-' || date[7] != '-') return false;
            
            for (int i = 0; i < 10; i++) {
                if (i == 4 || i == 7) continue;
                if (!isdigit(date[i])) return false;
            }
            
            return true;
        }
        
    public:
        LogManager(FoodDatabaseManager& manager, const string& logFile)
            : dbManager(manager), logFilePath(logFile) {
            currentDate = getCurrentDateString();
            loadLogs();
        }
        
        ~LogManager() {
            saveLogs();
        }
        
        void executeCommand(unique_ptr<Command> cmd) {
            cmd->execute();
            undoStack.push(move(cmd));
            
            // Clear redo stack when a new command is executed
            while (!redoStack.empty()) {
                redoStack.pop();
            }
        }
        
        void undo() {
            if (undoStack.empty()) {
                cout << "Nothing to undo." << endl;
                return;
            }
            
            cout << "Undoing: " << undoStack.top()->getDescription() << endl;
            undoStack.top()->undo();
            redoStack.push(move(undoStack.top()));
            undoStack.pop();
        }
        
        void redo() {
            if (redoStack.empty()) {
                cout << "Nothing to redo." << endl;
                return;
            }
            
            cout << "Redoing: " << redoStack.top()->getDescription() << endl;
            redoStack.top()->execute();
            undoStack.push(move(redoStack.top()));
            redoStack.pop();
        }
        
        void addFoodToLog(const string& date, shared_ptr<Food> food, double servings) {
            auto cmd = make_unique<AddEntryCommand>(logs, date, food, servings);
            executeCommand(move(cmd));
        }
        
        void removeFoodFromLog(const string& date, int index) {
            if (logs.find(date) != logs.end()) {
                const auto& entries = logs[date].getEntries();
                if (index >= 0 && index < entries.size()) {
                    auto cmd = make_unique<RemoveEntryCommand>(logs, date, index, entries[index]);
                    executeCommand(move(cmd));
                } else {
                    cout << "Invalid entry index." << endl;
                }
            } else {
                cout << "No log found for date: " << date << endl;
            }
        }
        
        void setCurrentDate(const string& date) {
            if (validateDateFormat(date)) {
                currentDate = date;
                cout << "Current date set to: " << currentDate << endl;
            } else {
                cout << "Invalid date format. Please use YYYY-MM-DD format." << endl;
            }
        }
        
        string getCurrentDate() const {
            return currentDate;
        }
        
        void displayCurrentLog() const {
            cout << "Log for " << currentDate << ":" << endl;
            if (logs.find(currentDate) != logs.end()) {
                logs.at(currentDate).displayEntries();
            } else {
                cout << "No entries for today." << endl;
            }
        }
        
        void displayLogForDate(const string& date) const {
            cout << "Log for " << date << ":" << endl;
            if (logs.find(date) != logs.end()) {
                logs.at(date).displayEntries();
            } else {
                cout << "No entries for this date." << endl;
            }
        }
        
        void loadLogs() {
            try {
                ifstream file(logFilePath);
                if (!file.is_open()) {
                    cout << "No existing log file found. Starting with empty logs." << endl;
                    return;
                }
                
                json j;
                file >> j;
                
                for (const auto& [date, entries] : j.items()) {
                    for (const auto& entry : entries) {
                        string foodName = entry["name"];
                        double servings = entry["servings"];
                        
                        auto food = dbManager.findFoodByName(foodName);
                        if (food) {
                            logs[date].addEntry(food, servings);
                        }
                    }
                }
                
                cout << "Logs loaded successfully." << endl;
            } catch (const exception& e) {
                cout << "Error loading logs: " << e.what() << endl;
            }
        }
        
        void saveLogs() {
            try {
                json j;
                for (const auto& [date, log] : logs) {
                    j[date] = log.toJson();
                }
                
                ofstream file(logFilePath);
                file << j.dump(2);
                cout << "Logs saved successfully." << endl;
            } catch (const exception& e) {
                cout << "Error saving logs: " << e.what() << endl;
            }
        }
        
        void showUndoHistory() const {
            if (undoStack.empty()) {
                cout << "No actions to undo." << endl;
                return;
            }
            
            cout << "Actions that can be undone:" << endl;
            cout << "-------------------------" << endl;
            
            stack<unique_ptr<Command>> tempStack;
            int count = 0;
            
            // Use a temporary container to not modify the original stack
            while (!const_cast<stack<unique_ptr<Command>>&>(undoStack).empty()) {
                auto& cmd = const_cast<stack<unique_ptr<Command>>&>(undoStack).top();
                cout << count++ << ": " << cmd->getDescription() << endl;
                tempStack.push(move(const_cast<stack<unique_ptr<Command>>&>(undoStack).top()));
                const_cast<stack<unique_ptr<Command>>&>(undoStack).pop();
            }
            
            // Restore the original stack
            while (!tempStack.empty()) {
                const_cast<stack<unique_ptr<Command>>&>(undoStack).push(move(tempStack.top()));
                tempStack.pop();
            }
        }
    };
    





// Command Line Interface class
class DietAssistantCLI {
private:
    FoodDatabaseManager dbManager;
    bool running;
    LogManager logManager;
    
    void displayMenu() {
        cout << "\n===== Diet Assistant Menu =====\n";
        cout << "1. Search foods\n";
        cout << "2. View food details\n";
        cout << "3. Add basic food\n";
        cout << "4. Create composite food\n";
        cout << "5. List all foods\n";
        cout << "6. Save database\n";
        cout << "7. View today's log (" << logManager.getCurrentDate() << ")" << endl;
        cout << "8. Add food to log" << endl;
        cout << "9. Remove food from log" << endl;
        cout << "10. Change active date" << endl;
        cout << "11. View log for specific date" << endl;
        cout << "12. Undo last action" << endl;
        cout << "13. Show undo history" << endl;
        cout << "14. Save logs" << endl;
        cout << "0. Exit" << endl;
        cout << "==============================\n";
        cout << "Enter choice (0-14): ";
    }
    
    void searchFoods() {
        cout << "\nEnter search term: ";
        string query;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        getline(cin, query);
        
        vector<shared_ptr<Food>> results = dbManager.searchFoods(query);
        
        if (results.empty()) {
            cout << "No foods found matching '" << query << "'." << endl;
        } else {
            cout << "\n=== Search Results for '" << query << "' (" << results.size() << " found) ===" << endl;
            for (size_t i = 0; i < results.size(); ++i) {
                cout << i+1 << ". " << results[i]->getName() 
                     << " (" << results[i]->getType() << ") - " 
                     << results[i]->getCalories() << " calories" << endl;
            }
        }
    }
    
    void viewFoodDetails() {
        cout << "\nEnter food name: ";
        string name;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        getline(cin, name);
        
        shared_ptr<Food> food = dbManager.getFood(name);
        if (food) {
            cout << "\n=== Food Details ===" << endl;
            food->display();
        } else {
            cout << "Food '" << name << "' not found." << endl;
        }
    }
    
    void addBasicFood() {
        string name;
        vector<string> keywords;
        float calories;
        
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        
        cout << "\n=== Add Basic Food ===" << endl;
        
        cout << "Enter food name: ";
        getline(cin, name);
        
        cout << "Enter calories per serving: ";
        cin >> calories;
        
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        
        cout << "Enter keywords (comma-separated): ";
        string keywordsStr;
        getline(cin, keywordsStr);
        
        // Parse comma-separated keywords
        size_t pos = 0;
        string token;
        while ((pos = keywordsStr.find(',')) != string::npos) {
            token = keywordsStr.substr(0, pos);
            token.erase(0, token.find_first_not_of(' '));
            token.erase(token.find_last_not_of(' ') + 1);
            if (!token.empty()) keywords.push_back(token);
            keywordsStr.erase(0, pos + 1);
        }
        // Add the last keyword
        keywordsStr.erase(0, keywordsStr.find_first_not_of(' '));
        keywordsStr.erase(keywordsStr.find_last_not_of(' ') + 1);
        if (!keywordsStr.empty()) keywords.push_back(keywordsStr);
        
        auto newFood = make_shared<BasicFood>(name, keywords, calories);
        if (dbManager.addFood(newFood)) {
            cout << "Basic food '" << name << "' added successfully." << endl;
        }
    }
    
    void createCompositeFood() {
        string name;
        vector<string> keywords;
        vector<FoodComponent> components;
        
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        
        cout << "\n=== Create Composite Food ===" << endl;
        
        cout << "Enter composite food name: ";
        getline(cin, name);
        
        cout << "Enter keywords (comma-separated): ";
        string keywordsStr;
        getline(cin, keywordsStr);
        
        // Parse comma-separated keywords
        size_t pos = 0;
        string token;
        while ((pos = keywordsStr.find(',')) != string::npos) {
            token = keywordsStr.substr(0, pos);
            token.erase(0, token.find_first_not_of(' '));
            token.erase(token.find_last_not_of(' ') + 1);
            if (!token.empty()) keywords.push_back(token);
            keywordsStr.erase(0, pos + 1);
        }
        // Add the last keyword
        keywordsStr.erase(0, keywordsStr.find_first_not_of(' '));
        keywordsStr.erase(keywordsStr.find_last_not_of(' ') + 1);
        if (!keywordsStr.empty()) keywords.push_back(keywordsStr);
        
        bool addingComponents = true;
        while (addingComponents) {
            cout << "\nEnter component food name (or 'done' to finish): ";
            string componentName;
            getline(cin, componentName);
            
            if (componentName == "done") {
                addingComponents = false;
                continue;
            }
            
            shared_ptr<Food> componentFood = dbManager.getFood(componentName);
            if (!componentFood) {
                cout << "Food '" << componentName << "' not found." << endl;
                continue;
            }
            
            float servings;
            cout << "Enter number of servings: ";
            cin >> servings;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            
            components.emplace_back(componentFood, servings);
            cout << "Added " << servings << " serving" << (servings > 1 ? "s" : "") 
                 << " of '" << componentName << "'" << endl;
        }
        
        if (components.empty()) {
            cout << "No components added. Composite food creation cancelled." << endl;
            return;
        }
        
        auto newFood = CompositeFood::createFromComponents(name, keywords, components);
        if (dbManager.addFood(newFood)) {
            cout << "Composite food '" << name << "' created successfully." << endl;
            cout << "Total calories: " << newFood->getCalories() << endl;
        }
    }

    void handleViewTodaysLog() {
        logManager.displayCurrentLog();
    }
    
    void handleAddFoodToLog() {
        cout << "\n==== Add Food to Log (" << logManager.getCurrentDate() << ") ====" << endl;
        
        // Search for food first
        cout << "\n1. Search by name" << endl;
        cout << "2. Search by keywords (match any)" << endl;
        cout << "3. Search by keywords (match all)" << endl;
        cout << "0. Cancel" << endl;
        cout << "Choose an option: ";
        
        int option;
        cin >> option;
        cin.ignore();
        
        if (option == 0) return;
        
        vector<shared_ptr<Food>> results;
        
        if (option == 1) {
            cout << "Enter name to search: ";
            string name;
            getline(cin, name);
            results = dbManager.searchFoods(name);
        } else if (option == 2 || option == 3) {
            cout << "Enter keywords (separated by spaces): ";
            string keywordsStr;
            getline(cin, keywordsStr);
            
            vector<string> keywords;
            istringstream iss(keywordsStr);
            string keyword;
            while (iss >> keyword) {
                keywords.push_back(keyword);
            }
            
            bool matchAll = (option == 3);
            results = dbManager.searchFoodsByKeywords(keywords, matchAll);
        }
        
        if (results.empty()) {
            cout << "No matching foods found." << endl;
            return;
        }
        
        cout << "\nFound " << results.size() << " matching foods:" << endl;
        for (size_t i = 0; i < results.size(); ++i) {
            cout << "[" << i << "] ";
            results[i]->display();
        }
        
        cout << "\nSelect food by number: ";
        int foodIndex;
        cin >> foodIndex;
        
        if (foodIndex < 0 || foodIndex >= results.size()) {
            cout << "Invalid selection." << endl;
            cin.ignore();
            return;
        }
        
        cout << "Enter number of servings: ";
        double servings;
        cin >> servings;
        cin.ignore();
        
        logManager.addFoodToLog(logManager.getCurrentDate(), results[foodIndex], servings);
        cout << servings << " serving(s) of " << results[foodIndex]->getName() 
                  << " added to log for " << logManager.getCurrentDate() << endl;
    }
    
    void handleRemoveFoodFromLog() {
        cout << "\n==== Remove Food from Log (" << logManager.getCurrentDate() << ") ====" << endl;
        
        logManager.displayCurrentLog();
        
        cout << "\nEnter index of food to remove (or -1 to cancel): ";
        int index;
        cin >> index;
        cin.ignore();
        
        if (index == -1) return;
        
        logManager.removeFoodFromLog(logManager.getCurrentDate(), index);
    }
    
    void handleChangeActiveDate() {
        cout << "\n==== Change Active Date ====" << endl;
        cout << "Current active date: " << logManager.getCurrentDate() << endl;
        cout << "Enter new date (YYYY-MM-DD): ";
        
        string date;
        getline(cin, date);
        
        logManager.setCurrentDate(date);
    }
    
    void handleViewLogForDate() {
        cout << "\n==== View Log for Specific Date ====" << endl;
        cout << "Enter date (YYYY-MM-DD): ";
        
        string date;
        getline(cin, date);
        
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
    


    
    void handleExit() {
        if (dbManager.isModified()) {
            cout << "Database has unsaved changes. Save before exit? (y/n): ";
            char choice;
            cin >> choice;
            
            if (choice == 'y' || choice == 'Y') {
                dbManager.saveDatabase();
            }
        }
        
        running = false;
    }

public:
   // Constructor with default database and log file paths
   DietAssistantCLI(const string& databasePath = "food_database.json", const string& logFile = "food_logs.json")
   : dbManager(databasePath), logManager(dbManager, logFile), running(false) {}

    void start() {
        running = true;
        dbManager.loadDatabase();
        
        cout << "Welcome to Diet Assistant!" << endl;
        
        while (running) {
            displayMenu();
            
            int choice;
            cin >> choice;
            
            switch (choice) {
                case 1:
                    searchFoods();
                    break;
                case 2:
                    viewFoodDetails();
                    break;
                case 3:
                    addBasicFood();
                    break;
                case 4:
                    createCompositeFood();
                    break;
                case 5:
                    dbManager.listAllFoods();
                    break;
                case 6:
                    dbManager.saveDatabase();
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
                    // handleExit();
                    break;
                case 0:
                    handleExit();
                    break;
                default:
                    cout << "Invalid choice. Please try again." << endl;
            }
        }
        
        cout << "Thank you for using Diet Assistant. Goodbye!" << endl;
    }
};

int main() {
    DietAssistantCLI dietAssistant;
    dietAssistant.start();
    return 0;
}