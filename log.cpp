#include <bits/stdc++.h>
using namespace std;
#include "json.hpp"

using json = nlohmann::json;

// Forward declarations
class Command;
class FoodEntry;

// Basic data structures
struct Food {
    string name;
    string type;  // "basic" or "composite"
    double calories;
    vector<string> keywords;
    map<string, double> components;  // For composite foods: component name -> servings

    Food() = default;
    
    Food(const json& j) {
        name = j["name"];
        type = j["type"];
        calories = j["calories"];
        
        for (const auto& keyword : j["keywords"]) {
            keywords.push_back(keyword);
        }
        
        if (type == "composite" && j.contains("components")) {
            for (const auto& comp : j["components"]) {
                components[comp["name"]] = comp["servings"];
            }
        }
    }
};

// Food log entry for a specific date
class FoodEntry {
public:
    string foodName;
    double servings;
    double calories;

    FoodEntry(const string& name, double servs, double cals) 
        : foodName(name), servings(servs), calories(cals) {}
};

// Date handling utility
class DateUtil {
public:
    static string getCurrentDate() {
        auto now = chrono::system_clock::now();
        auto time = chrono::system_clock::to_time_t(now);
        tm tm = *localtime(&time);
        
        stringstream ss;
        ss << put_time(&tm, "%Y-%m-%d");
        return ss.str();
    }
    
    static bool isValidDate(const string& dateStr) {
        
        if (dateStr.length() != 10) return false;
        
        // Check format: YYYY-MM-DD
        for (int i = 0; i < 10; i++) {
            if ((i == 4 || i == 7) && dateStr[i] != '-') return false;
            else if (i != 4 && i != 7 && !isdigit(dateStr[i])) return false;
        }
        
        int year = stoi(dateStr.substr(0, 4));
        int month = stoi(dateStr.substr(5, 2));
        int day = stoi(dateStr.substr(8, 2));
        
        if (month < 1 || month > 12) return false;
        
        int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        
        // Adjust for leap year
        if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
            daysInMonth[2] = 29;
        }
        
        return day >= 1 && day <= daysInMonth[month];
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

// Food diary main class
class FoodDiary {
private:
    string databaseFile;
    string logFile;
    map<string, Food> foods;
    map<string, vector<FoodEntry>> dailyLogs;
    stack<shared_ptr<Command>> undoStack;
    string currentDate;

public:
    FoodDiary(const string& dbFile, const string& log) 
        : databaseFile(dbFile), logFile(log), currentDate(DateUtil::getCurrentDate()) {
        loadDatabase();
        loadLogs();
    }

    ~FoodDiary() {
        saveLogs();
    }

    // Database operations
    void loadDatabase() {
        try {
            ifstream file(databaseFile);
            if (!file.is_open()) {
                cerr << "Unable to open database file: " << databaseFile << endl;
                return;
            }

            json j;
            file >> j;
            file.close();

            for (const auto& item : j) {
                Food food(item);
                foods[food.name] = food;
            }
            
            cout << "Loaded " << foods.size() << " foods from database." << endl;
        } catch (const exception& e) {
            cerr << "Error loading database: " << e.what() << endl;
        }
    }

    // Log operations
    void loadLogs() {
        try {
            ifstream file(logFile);
            if (!file.is_open()) {
                cout << "No existing log file found. Creating a new one." << endl;
                return;
            }

            json j;
            file >> j;
            file.close();

            for (auto& [date, entries] : j.items()) {
                for (const auto& entry : entries) {
                    string foodName = entry["food"];
                    double servings = entry["servings"];
                    double calories = entry["calories"];
                    dailyLogs[date].emplace_back(foodName, servings, calories);
                }
            }
            
            cout << "Loaded food logs for " << dailyLogs.size() << " days." << endl;
        } catch (const exception& e) {
            cerr << "Error loading logs: " << e.what() << endl;
        }
    }

    void saveLogs() {
        try {
            json j;
            
            for (const auto& [date, entries] : dailyLogs) {
                json dateEntries = json::array();
                
                for (const auto& entry : entries) {
                    json entryJson;
                    entryJson["food"] = entry.foodName;
                    entryJson["servings"] = entry.servings;
                    entryJson["calories"] = entry.calories;
                    dateEntries.push_back(entryJson);
                }
                
                j[date] = dateEntries;
            }
            
            ofstream file(logFile);
            if (!file.is_open()) {
                cerr << "Unable to open log file for writing: " << logFile << endl;
                return;
            }
            
            file << setw(4) << j;
            file.close();
            
            cout << "Logs saved successfully." << endl;
        } catch (const exception& e) {
            cerr << "Error saving logs: " << e.what() << endl;
        }
    }

    // Command to add a food entry
    class AddFoodCommand : public Command {
    private:
        FoodDiary& diary;
        string date;
        string foodName;
        double servings;
        double calories;
        
    public:
        AddFoodCommand(FoodDiary& d, const string& dt, const string& name, double servs) 
            : diary(d), date(dt), foodName(name), servings(servs) {
            // Calculate calories based on food definition
            auto it = diary.foods.find(foodName);
            if (it != diary.foods.end()) {
                calories = it->second.calories * servings;
            } else {
                calories = 0;
            }
        }
        
        void execute() override {
            diary.dailyLogs[date].emplace_back(foodName, servings, calories);
        }
        
        void undo() override {
            auto& entries = diary.dailyLogs[date];
            if (!entries.empty()) {
                // Remove the latest entry with this food name
                for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
                    if (it->foodName == foodName && abs(it->servings - servings) < 0.001) {
                        entries.erase((it + 1).base());
                        break;
                    }
                }
            }
            
            // If the daily log is now empty, remove the date entry
            if (entries.empty()) {
                diary.dailyLogs.erase(date);
            }
        }
        
        string getDescription() const override {
            stringstream ss;
            ss << "Add " << servings << " serving(s) of " << foodName << " (" 
               << calories << " calories) on " << date;
            return ss.str();
        }
    };

    // Command to delete a food entry
    class DeleteFoodCommand : public Command {
    private:
        FoodDiary& diary;
        string date;
        size_t index;
        FoodEntry deletedEntry;
        
    public:
        DeleteFoodCommand(FoodDiary& d, const string& dt, size_t idx) 
            : diary(d), date(dt), index(idx), 
              deletedEntry("", 0, 0) {
            // Store the entry for potential undo
            auto& entries = diary.dailyLogs[date];
            if (index < entries.size()) {
                deletedEntry = entries[index];
            }
        }
        
        void execute() override {
            auto& entries = diary.dailyLogs[date];
            if (index < entries.size()) {
                entries.erase(entries.begin() + index);
                
                // If the daily log is now empty, remove the date entry
                if (entries.empty()) {
                    diary.dailyLogs.erase(date);
                }
            }
        }
        
        void undo() override {
            // Re-add the deleted entry
            diary.dailyLogs[date].push_back(deletedEntry);
        }
        
        string getDescription() const override {
            stringstream ss;
            ss << "Delete " << deletedEntry.servings << " serving(s) of " 
               << deletedEntry.foodName << " from " << date;
            return ss.str();
        }
    };

    // Date management
    void setCurrentDate(const string& date) {
        if (DateUtil::isValidDate(date)) {
            currentDate = date;
            cout << "Current date set to: " << currentDate << endl;
        } else {
            cerr << "Invalid date format. Please use YYYY-MM-DD." << endl;
        }
    }

    string getCurrentDate() const {
        return currentDate;
    }

    // Food search functions
    vector<string> searchFoodsByKeywords(const vector<string>& keywords, bool matchAll) {
        vector<string> results;
        
        for (const auto& [name, food] : foods) {
            bool matches = matchAll;
            
            for (const auto& keyword : keywords) {
                bool keywordFound = false;
                string lowerKeyword = toLower(keyword);
                
                // Check if keyword is in food keywords
                for (const auto& foodKeyword : food.keywords) {
                    if (toLower(foodKeyword).find(lowerKeyword) != string::npos) {
                        keywordFound = true;
                        break;
                    }
                }
                
                // Also check if keyword is in food name
                if (!keywordFound && toLower(food.name).find(lowerKeyword) != string::npos) {
                    keywordFound = true;
                }
                
                if (matchAll) {
                    matches = matches && keywordFound;
                } else {
                    matches = matches || keywordFound;
                }
                
                // Early exit for OR matching
                if (!matchAll && keywordFound) {
                    matches = true;
                    break;
                }
            }
            
            if (matches) {
                results.push_back(name);
            }
        }
        
        return results;
    }

    // Food management
    void listAllFoods() const {
        cout << "\nAll Foods in Database:\n";
        cout << setw(30) << left << "Name" 
                  << setw(15) << left << "Type"
                  << setw(15) << right << "Calories" << endl;
        cout << string(60, '-') << endl;
        
        for (const auto& [name, food] : foods) {
            cout << setw(30) << left << name 
                      << setw(15) << left << food.type
                      << setw(15) << right << food.calories << endl;
        }
        cout << endl;
    }

    void displayFoodDetails(const string& foodName) const {
        auto it = foods.find(foodName);
        if (it == foods.end()) {
            cout << "Food not found: " << foodName << endl;
            return;
        }
        
        const Food& food = it->second;
        
        cout << "\nFood Details: " << food.name << endl;
        cout << string(50, '-') << endl;
        cout << "Type: " << food.type << endl;
        cout << "Calories: " << food.calories << endl;
        
        cout << "Keywords: ";
        for (size_t i = 0; i < food.keywords.size(); i++) {
            cout << food.keywords[i];
            if (i < food.keywords.size() - 1) {
                cout << ", ";
            }
        }
        cout << endl;
        
        if (food.type == "composite") {
            cout << "\nComponents:" << endl;
            for (const auto& [compName, servings] : food.components) {
                cout << "- " << compName << ": " << servings << " serving(s)" << endl;
            }
        }
        cout << endl;
    }

    // Log display
    void displayDailyLog(const string& date) const {
        auto it = dailyLogs.find(date);
        if (it == dailyLogs.end() || it->second.empty()) {
            cout << "No food entries for " << date << endl;
            return;
        }
        
        double totalCalories = 0.0;
        
        cout << "\nFood Log for " << date << ":\n";
        cout << setw(5) << left << "No."
                  << setw(30) << left << "Food" 
                  << setw(15) << left << "Servings"
                  << setw(15) << right << "Calories" << endl;
        cout << string(65, '-') << endl;
        
        int count = 1;
        for (const auto& entry : it->second) {
            cout << setw(5) << left << count++
                      << setw(30) << left << entry.foodName 
                      << setw(15) << left << entry.servings
                      << setw(15) << right << entry.calories << endl;
            
            totalCalories += entry.calories;
        }
        
        cout << string(65, '-') << endl;
        cout << setw(50) << left << "Total Calories:" 
                  << setw(15) << right << totalCalories << endl;
        cout << endl;
    }

    // Command execution with undo support
    void executeCommand(shared_ptr<Command> command) {
        command->execute();
        undoStack.push(command);
        cout << "Executed: " << command->getDescription() << endl;
    }

    void undo() {
        if (undoStack.empty()) {
            cout << "Nothing to undo." << endl;
            return;
        }
        
        auto command = undoStack.top();
        undoStack.pop();
        
        command->undo();
        cout << "Undone: " << command->getDescription() << endl;
    }

    // Utility functions
    static string toLower(const string& str) {
        string result = str;
        transform(result.begin(), result.end(), result.begin(), 
                       [](unsigned char c){ return tolower(c); });
        return result;
    }

    // Food entry management
    void addFood(const string& date, const string& foodName, double servings) {
        auto it = foods.find(foodName);
        if (it == foods.end()) {
            cerr << "Food not found: " << foodName << endl;
            return;
        }
        
        auto command = make_shared<AddFoodCommand>(*this, date, foodName, servings);
        executeCommand(command);
    }

    void deleteFood(const string& date, size_t index) {
        auto it = dailyLogs.find(date);
        if (it == dailyLogs.end() || index >= it->second.size()) {
            cerr << "Invalid food entry index." << endl;
            return;
        }
        
        auto command = make_shared<DeleteFoodCommand>(*this, date, index);
        executeCommand(command);
    }

    // User interface methods
    void addFoodToLog() {
        // First, let the user choose how to select a food
        cout << "\nSelect food by:\n";
        cout << "1. Browse all foods\n";
        cout << "2. Search by keywords\n";
        cout << "Choice: ";
        
        int choice;
        cin >> choice;
        cin.ignore();
        
        vector<string> foodOptions;
        
        if (choice == 1) {
            // List all foods for selection
            listAllFoods();
            
            // Convert map to vector for indexing
            for (const auto& [name, _] : foods) {
                foodOptions.push_back(name);
            }
        } else if (choice == 2) {
            cout << "Enter keywords (separated by spaces): ";
            string keywordInput;
            getline(cin, keywordInput);
            
            // Split input into keywords
            vector<string> keywords;
            stringstream ss(keywordInput);
            string keyword;
            while (ss >> keyword) {
                keywords.push_back(keyword);
            }
            
            if (keywords.empty()) {
                cout << "No keywords provided." << endl;
                return;
            }
            
            cout << "Match: 1. All keywords or 2. Any keyword? ";
            int matchChoice;
            cin >> matchChoice;
            cin.ignore();
            
            bool matchAll = (matchChoice == 1);
            foodOptions = searchFoodsByKeywords(keywords, matchAll);
            
            if (foodOptions.empty()) {
                cout << "No foods match the given keywords." << endl;
                return;
            }
            
            // Display the matching foods
            cout << "\nMatching Foods:\n";
            for (size_t i = 0; i < foodOptions.size(); i++) {
                cout << (i + 1) << ". " << foodOptions[i] << endl;
            }
        } else {
            cout << "Invalid choice." << endl;
            return;
        }
        
        // Let the user select a food
        if (foodOptions.empty()) {
            cout << "No foods available for selection." << endl;
            return;
        }
        
        cout << "\nSelect food number (1-" << foodOptions.size() << "): ";
        int foodIndex;
        cin >> foodIndex;
        
        if (foodIndex < 1 || foodIndex > static_cast<int>(foodOptions.size())) {
            cout << "Invalid food selection." << endl;
            return;
        }
        
        string selectedFood = foodOptions[foodIndex - 1];
        
        // Ask for number of servings
        cout << "Enter number of servings: ";
        double servings;
        cin >> servings;
        cin.ignore();
        
        if (servings <= 0) {
            cout << "Invalid number of servings." << endl;
            return;
        }
        
        // Add the food to the log
        addFood(currentDate, selectedFood, servings);
    }

    void deleteFoodFromLog() {
        displayDailyLog(currentDate);
        
        auto it = dailyLogs.find(currentDate);
        if (it == dailyLogs.end() || it->second.empty()) {
            cout << "No entries to delete." << endl;
            return;
        }
        
        cout << "Enter entry number to delete: ";
        int index;
        cin >> index;
        cin.ignore();
        
        if (index < 1 || index > static_cast<int>(it->second.size())) {
            cout << "Invalid entry number." << endl;
            return;
        }
        
        deleteFood(currentDate, index - 1);
    }

    void changeDate() {
        cout << "Enter date (YYYY-MM-DD): ";
        string date;
        cin >> date;
        cin.ignore();
        
        setCurrentDate(date);
    }

    void viewFoodDetails() {
        listAllFoods();
        
        cout << "Enter food name: ";
        string foodName;
        getline(cin, foodName);
        
        displayFoodDetails(foodName);
    }

    void showUndoStack() const {
        if (undoStack.empty()) {
            cout << "Undo stack is empty." << endl;
            return;
        }
        
        cout << "\nUndo Stack (latest first):\n";
        
        // Create a temporary stack to display in reverse order
        stack<shared_ptr<Command>> tempStack = undoStack;
        int count = 1;
        
        while (!tempStack.empty()) {
            cout << count++ << ". " << tempStack.top()->getDescription() << endl;
            tempStack.pop();
        }
        
        cout << endl;
    }

    void runMainMenu() {
        bool running = true;
        
        while (running) {
            cout << "\n--- Food Diary (" << currentDate << ") ---\n";
            cout << "1. Add Food\n";
            cout << "2. View Today's Log\n";
            cout << "3. Delete Food Entry\n";
            cout << "4. View Food Details\n";
            cout << "5. Change Current Date\n";
            cout << "6. Undo Last Action\n";
            cout << "7. View Undo Stack\n";
            cout << "8. Save and Exit\n";
            cout << "Choice: ";
            
            int choice;
            cin >> choice;
            cin.ignore();
            
            switch (choice) {
                case 1:
                    addFoodToLog();
                    break;
                case 2:
                    displayDailyLog(currentDate);
                    break;
                case 3:
                    deleteFoodFromLog();
                    break;
                case 4:
                    viewFoodDetails();
                    break;
                case 5:
                    changeDate();
                    break;
                case 6:
                    undo();
                    break;
                case 7:
                    showUndoStack();
                    break;
                case 8:
                    saveLogs();
                    running = false;
                    break;
                default:
                    cout << "Invalid choice." << endl;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    string databaseFile = "food_database.json";
    string logFile = "food_log.json";
    
    // Parse command line arguments
    if (argc > 1) {
        databaseFile = argv[1];
    }
    
    if (argc > 2) {
        logFile = argv[2];
    }
    
    try {
        FoodDiary diary(databaseFile, logFile);
        diary.runMainMenu();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}