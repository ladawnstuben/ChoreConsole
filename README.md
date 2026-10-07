# Chore Manager

A desktop household chore manager for Windows, built with **wxWidgets** and **C++20**. Track chores, assign them to chore doers, log completions with earnings, and manage multiple households, each saved as its own JSON file.

## Features

- **Multiple households** - create, switch between, and delete independent households (own chores, doers, and history), managed from the Household menu.
- **Today view** - the app opens on a home dashboard of what's actually due right now, grouped by chore doer, with one-tap "Mark Done" and at-a-glance stats (due today, completed today, top streak).
- **Recurring chores** - each chore repeats Once, Daily, Weekly (every N weeks, optionally on specific weekdays), or Monthly. Completed recurring chores automatically reset back to Not Started when their next occurrence comes due (checked on startup, on household switch, and every 5 minutes while the app is open).
- **Chores** - create, modify, delete, search, compare, and sort chores by ID, name, earnings, or category. An inline detail/edit panel on the Chores tab lets you tweak the common fields (name, category, assignment, priority, status, earnings, location, notes) without a popup dialog. Filter the list by status (All / Not Started / In Progress / Completed), and bulk-select chores via checkboxes to assign, mark done, or delete several at once.
- **Custom categories** - manage the set of chore categories per household via "Manage Categories...".
- **Chore doers** - add/delete doers, edit their profile (notes, etc.), and view a card-based roster with streaks, total earnings, and milestone badges (e.g. "7-day streak!", "10 chores done!").
- **Assignment workflow** - assign chores randomly across doers, or assign/reassign/unassign an individual chore directly (via the "Assign To..." button, double-click, or right-click on a chore); Start / Complete / Reset a chore's status per doer.
- **Starter chores** - seed a new or empty household with a curated set of common chores spanning every recurrence type, from the Household menu or when creating a new household.
- **Themes** - choose from five built-in color themes (Warm Bubbly, Midnight, Minimal Monochrome, Scandinavian Calm, Corporate Clean) under View > Theme; the choice is global (not per-household) and persists across launches.
- **Activity history** - a day-by-day log of every assigned/unassigned/start/complete/reset action with timestamps and earnings, navigable with Prev/Today/Next.
- **Search, sort, and compare** tools for chores, plus exporting all chore-doer assignments to a file.
- **Bubbly UI** - a custom rounded-button and card-panel visual language shared across all views (`ChoreApp::Palette`, `RoundedButton`, `CardPanel`), navigated via a left-hand sidebar (Today / Chores / Chore Doers / History) instead of a top tab strip.

## Project Structure

- **main.cpp** - application entry point and all wxWidgets UI (`ChoreApp` namespace): main frame, sidebar navigation, tabs/dialogs, the theme system (`ThemeManager`), and the custom-drawn controls.
- **ChoreModel.h** - core domain model: `Chore` (with its `Recurrence` and `EasyChore`/`MediumChore`/`HardChore` subclasses), `ChoreDoer`, the generic `Container<T>` collection, `ChoreManager` (owns chores/doers/history, the due-date rollover engine, and JSON load/save for one household), and `HouseholdRegistry` (tracks known households, which one was last open, and the active theme).
- **json.hpp** - [nlohmann/json](https://github.com/nlohmann/json), used for all persistence.
- **TestData/** - runtime data directory: `app_state.json` (registry of households plus the active theme) plus a `households/` folder holding one JSON file per household.

## Building

Requires Visual Studio 2022 (v143 toolset) with the Desktop C++ workload, and [vcpkg](https://github.com/microsoft/vcpkg) in manifest mode (dependencies are declared in `vcpkg.json` and restore automatically on build).

1. Open `ChoreConsole.sln` in Visual Studio.
2. Build the `ChoreConsole` project (Debug or Release, x64). vcpkg will restore wxWidgets and its dependencies on first build.
3. Run `x64\<Configuration>\ChoreConsole.exe`.

From the command line:

```
"C:\Program Files\Microsoft Visual Studio\2022\<Edition>\MSBuild\Current\Bin\amd64\MSBuild.exe" ChoreConsole.sln /p:Configuration=Release /p:Platform=x64
```

The executable resolves its `TestData` directory relative to its own location (two levels up from `x64\<Configuration>\`), so it works whether launched from Visual Studio or by double-clicking the built `.exe`.

## License

This project is licensed under a custom license, which includes all permissions of the MIT license with an additional restriction as follows:

### Additional Restriction

The use of this software in commercial settings is exclusively granted to the original developers of this project. Any other use, reproduction, modification, distribution, or sublicense of this software in a commercial context is expressly prohibited, unless otherwise agreed upon in writing by the original developers.

### MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
