# Chore Manager System

## Overview
The Chore Manager System is designed to manage and track household chores. It allows users to create chore doers, assign chores randomly, and manage chore details interactively. This system is built using C++ and utilizes JSON for data handling.

## Features

- **User Profiles**: Manage user preferences and profiles.
- **Chore Assignment**: Assign chores randomly or manually to chore doers.
- **Data Management**: Load, modify, and save chore data from/to JSON files.
- **Sorting and Searching**: Sort chores by various attributes and search for specific chores.
- **Interactive Menus**: Easy-to-use text-based menus for navigating through the application's features.

## Getting Started

1. **Compile the Code**:
        Ensure you have a C++ compiler that supports C++17 or above. Compile the project using:
        ```
        g++ -std=c++17 -o ChoreManager main.cpp
        ```
2. **Run the Application**:
        Once compiled, you can run the application by executing:
        ```
        ./ChoreManager
        ```

3. **Follow the On-Screen Prompts**:
        Navigate through the application using the numbered menus to manage chores and chore doers.

## File Structure

- **main.cpp**: Contains the main function and is the entry point of the application.
- **json.hpp**: Core application logic for loading and saving json data to and from json file.

## Dependencies

- **[JSON for Modern C++](https://github.com/nlohmann/json)**: Used for handling JSON data within the application. Ensure to include this in your project directory or install it system-wide.

## Documentation

### Classes and Functions

- **Client**
    - Manages user profiles and settings.
    - Methods for serializing and modifying user data.

- **Chore**
    - Base class for chores with attributes like ID, name, difficulty, and earnings.
    - Functions for serialization and pretty printing.

- **EasyChore, MediumChore, HardChore**
    - Derived classes from `Chore` with specialized attributes and methods suitable for each difficulty level.

- **Container<T>**
    - Templated class to manage collections of objects like chores or chore doers.
    - Includes methods for adding, deleting, sorting, and searching items.

- **ChoreManager**
    - Main class that integrates user profiles, chores, and chore doers.
    - Handles file operations and user interactions through various menus.

### Main Functions

## Features

- **Add Chore Doer**: Add a new person responsible for doing chores.
- **Assign Chores Randomly**: Distribute chores to the chore doers randomly.
- **Show All Chores**: Display a list of all the chores that need to be done.
- **Display All Assigned Chores**: Show all chores that have already been assigned to chore doers.
- **Display All Chores with All Data**: Show detailed information for every chore, including who is responsible for it.
- **Print All Chore Doers Assigned Chores to File**: Save a list of all chore doers and their respective chores to a file.
- **Delete a Chore by ID**: Remove a chore from the list using its unique identifier.
- **Delete a Chore Doer by Name**: Remove a chore doer from the system by entering their name.
- **Display All Chore Doers**: List all the individuals responsible for chores.
- **Search Chores**: Look up chores based on specific criteria like ID, name, or other attributes.
- **Compare Two Chores**: Check if two chores are identical or compare their details.
- **Sort Chores**: Organize the list of chores based on certain attributes, such as difficulty or time required.
- **Exit**: Leave the chore management system.

- *Many functions have not been implemented yet*

## Usage

- Launch the application and navigate through the menu options to manage chores and chore doers.

## Requirements

- C++ compiler with support for C++11 or higher.
- Standard Template Library (STL).

## Compilation

To compile the Chore Manager system, ensure your environment is set up with a compatible C++ compiler and run the following command:

```bash
g++ -o ChoreManager main.cpp -std=c++17
```

## License

This project is licensed under a custom license, which includes all permissions of the MIT license with an additional restriction as follows:

### Additional Restriction

The use of this software in commercial settings is exclusively granted to the original developers of this project. Any other use, reproduction, modification, distribution, or sublicense of this software in a commercial context is expressly prohibited, unless otherwise agreed upon in writing by the original developers.

### MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.