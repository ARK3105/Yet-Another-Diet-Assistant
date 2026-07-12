# Calorie Tracker & Food Database Application

This is a command-line C++ application designed to help you manage a food database, log daily food consumption, and calculate personalized daily calorie recommendations.

## How to Run

Compile the program:

```bash
g++ food.cpp
```

Execute the application:

```bash
./a.out
```

> Dependency note: This program depends on the `nlohmann_json` library for JSON parsing. It is included as a header-only library via `json.hpp`. Ensure this header is in your project directory before compiling.

## Application Structure & Commands

The application is structured into three main feature sets mapped to specific command numbers:

- Commands 1-6: Food Database Management
- Commands 7-12: Food Diary Logging & Maintenance, including Undo and Date testing
- Commands 13+: User Profile & Calorie Recommendations

## Program Features

### 1. Food Database Management (Commands 1-6)

- Loading the Database: Upon startup, the program automatically attempts to load the database from `food_database.json`. If the file does not exist, a new, empty database is initialized.
- Adding Basic Foods: Add a single food item by specifying its name, keywords (separated by commas), and calorie count per serving.
- Adding Composite Foods: Create recipe-style or combined food entries. Enter the composite food's name and keywords, then add existing database items as components along with their respective number of servings.
- Searching for Foods: Search the database using keywords. You can filter results to match all specified keywords or any of them.
- Listing All Foods: View a complete list of all saved foods along with a summary of their nutritional/calorie information.
- Saving the Database: Manually save the current state of your food database back to `food_database.json`.

### 2. Food Diary Logging (Commands 7-12)

- Adding Food Entries: Log a meal by entering the food's name and the number of servings consumed. The program automatically logs it to the current date and calculates total calories using the database.
- Viewing Daily Logs: View detailed food logs for either the current date or a specific historic date. The log displays food names, servings consumed, and total calories.
- Viewing Calorie Summary: Instantly view the total accumulated calories consumed on any specific day.
- Loading & Saving Logs: Logs are automatically loaded from `food_log.json` on startup and safely written back to the file upon exiting the application.
- Undoing Actions: Made a mistake? The program supports an Undo feature to quickly revert the last added food entry.
- Date Mocking (Command 12): You can manually change the program's active date to test day-wise profile functionality and log history seamlessly.

### 3. Personalized Calorie Recommendations

- Setting User Profile: Set up or update your physical profile by entering your:
	- Gender
	- Age
	- Height
	- Weight
	- Activity Level
- Calorie Calculation Methods: Choose between two industry-standard formulas to calculate your daily energy expenditure:
	- Harris-Benedict Equation
	- Mifflin-St Jeor Equation
- Baseline Guidance: The computed daily calorie need serves as your baseline target when managing your intake through the daily food diary.

## System Configuration

| Configuration Item | Default Value | Description |
| --- | --- | --- |
| Database File Path | `food_database.json` | Stores the food directory, keywords, and calorie metrics. |
| Log File Path | `food_log.json` | Stores daily historic user consumption data. |
| Date Format | `YYYY-MM-DD` | Standardized format used for all manual date entries and validations. |
