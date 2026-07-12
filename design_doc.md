# DASS Assignment 3

## Diet Manager

### 06-04-2025

### TEAM:

- Atharva Kulkarni (2023101072)
- Kushal Balabhadruni (2023101039)

## Overview

The Diet Assistant is a command-line tool designed to help users manage their dietary habits efficiently. It combines a comprehensive food database, detailed daily logs, and personalized diet goal profiles to offer a complete nutrition tracking solution.

### Key Features:

#### Food Database:

- Basic Foods: Stores essential details for each food, including an identifying string, searchable keywords, and calories per serving.

- Composite Foods: Allows users to create new foods by combining existing basic or composite foods, with the total calorie count automatically computed from the components.

- Extensible Design: The database can be easily expanded to include additional nutritional information or integrate data from various web sources.

#### Daily Logs:

- Food Entry Management: Users can add, delete, and modify food entries, ensuring accurate tracking of daily consumption.

- Flexible Date Handling: Logs can be viewed and updated for any specific date, not just the current day.

- Undo Functionality: Supports an unlimited undo history during a session, allowing users to correct mistakes effortlessly.

#### Diet Goal Profile:

- Personalized Data: Tracks user details like gender, height, age, weight, and activity level.

- Calorie Intake Calculation: Computes daily target calorie intake using multiple methods, giving users the flexibility to choose the calculation that best suits their needs.

- Real-Time Feedback: Provides a summary of consumed calories versus the target, highlighting surpluses or deficits to help manage daily dietary goals.

This tool empowers users with clear insights into their nutrition, making it easier to stay on track with their diet and achieve their health objectives.


## Strong Aspects:

1. Clear Separation of Responsibilities:
- The design distinctly separates data handling (FoodDatabaseManager, ProfileManager) from business logic (FoodDiary, UserProfile) and user interaction via DietAssistantCLI. This adherence to the Single Responsibility Principle makes the system more modular and maintainable.

2. Effective Use of Abstraction and Composition:
- Abstract classes like Food and Command ensure that shared behavior is defined at a high level, while concrete implementations provide specific functionality. Notably, the existence of AddFoodCommand and DeleteFoodCommand as subclasses of Command makes it very easy to implement and modularize undo functionality. Additionally, the composition relationships (e.g., CompositeFood containing FoodComponent, FoodDiary containing FoodEntry) clearly express part-whole hierarchies, promoting code reuse and clarity in representing complex food structures.

## Weak Aspects:
1. Potential Over-Coupling in CLI Interactions:
- The DietAssistantCLI currently directly interacts with multiple managers (ProfileManager, FoodDatabaseManager, and FoodDiary). Moreover, the ProfileManager and FoodDiary depend on FoodDatabaseManager for certain functionalities, leading to tight coupling between the user interface and backend logic. This tight coupling could complicate future enhancements or changes. Introducing a decoupled mediator pattern could effectively isolate UI concerns from business logic, enhancing flexibility and reducing the risk of errors during refactoring.

2. Unordered Food Logs and Unclear Command Handling:
- Although AddFoodCommand and DeleteFoodCommand are implemented, food entries are stored as an unordered list, with no tracking of the order in which items were added. This limits features like time-based analysis. Additionally, undo functionality and error handling are not clearly defined, which may lead to inconsistencies. Clarifying these aspects would improve system reliability.
## Design Analysis: Balance Among Competing Criteria

### Low Coupling

The design shows efforts to maintain low coupling through:

1. *Command Pattern Implementation*: The abstract Command class and its concrete implementations (AddFoodCommand and DeleteFoodCommand) allow the FoodDiary to execute commands without knowing their specific implementations, reducing dependency.

2. *Manager Classes as Intermediaries*: ProfileManager and FoodDatabaseManager act as intermediaries between the system components, preventing direct dependencies between classes like DietAssistantCLI and UserProfile or FoodEntry.

3. *Clear Responsibility Boundaries*: Each class has well-defined responsibilities, with dependencies flowing in a controlled manner (e.g., FoodDiary uses but doesn't directly modify FoodDatabaseManager).

### High Cohesion

High cohesion is evident in:

1. *Specialized Classes*: Each class has a clear, focused purpose - UserProfile manages user data, FoodDatabaseManager handles food storage, and ProfileManager manages profile operations.

2. *Related Functionality Grouping*: Methods within classes serve related purposes (e.g., UserProfile contains methods specifically about user profile attributes and calculations).

3. *Well-Defined Class Responsibilities*: FoodEntry specifically represents food items with their properties, while DailyProfile focuses on daily activity and weight tracking.

### Separation of Concerns

The design separates concerns effectively:

1. *UI/Logic Separation*: DietAssistantCLI handles user interaction separately from business logic in managers and entity classes.

2. *Data/Behavior Separation*: Entity classes (Food, FoodEntry, UserProfile) focus on data while manager classes (ProfileManager, FoodDatabaseManager) handle operations on that data.

3. *Command Encapsulation*: The Command pattern encapsulates specific operations (add/delete food) separately from the invoker (FoodDiary).

### Information Hiding

Information hiding principles are implemented through:

1. *Encapsulated Class Structure*: Classes expose specific methods without revealing implementation details (e.g., ProfileManager exposes profile operations without exposing how profiles are stored).

2. *Protected Class Relationships*: The design uses composition and inheritance strategically (e.g., Food as an abstract class with BaseFood and CompositeFood specializations).

3. *Focused Data Access*: Classes like FoodDiary use specific methods to access data rather than manipulating internal structures directly.

### Law of Demeter

Adherence to the Law of Demeter (principle of least knowledge) is shown by:

1. *Controlled Method Access*: Classes interact primarily with their immediate collaborators (e.g., DietAssistantCLI interacts with ProfileManager, not directly with UserProfile).

2. *Method Chaining Prevention*: The design prevents excessive method chaining by having classes interact primarily with their immediate neighbors.

3. *Delegation*: Manager classes delegate operations to appropriate specialized classes rather than exposing their internal components.

### Design Patterns

Several design patterns enhance the architecture:

1. *Command Pattern*: Used for food operations, allowing for extendable command set and operation encapsulation.

2. *Composite Pattern*: Implemented with Food, BaseFood, and CompositeFood, allowing foods to be treated uniformly whether they're simple or compound.

3. *Manager Pattern*: ProfileManager and FoodDatabaseManager centralize related operations.

4. *Façade Pattern*: DietAssistantCLI acts as a simplified interface to the complex subsystem.

### Overall Balance

The design achieves balance among competing criteria by:

1. *Strategic Tradeoffs*: Accepting some coupling (like FoodDiary to FoodDatabaseManager) to achieve higher cohesion and clearer separation of concerns.

2. *Hierarchical Organization*: Using inheritance and composition relationships judiciously to promote both flexibility and structure.

3. *Clear Communication Paths*: Defining clear paths for class interactions that maintain information hiding while allowing necessary functionality.

4. *Interface-Based Design*: Using abstract classes and well-defined interfaces to allow for implementation variations while maintaining a consistent structure.