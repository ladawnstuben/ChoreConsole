#pragma once

// Model layer for ChoreConsole: client profile, chores, chore doers, categories,
// households, and their JSON persistence. Deliberately free of any wx* dependency
// so it can be exercised/reused independently of the GUI.

#include "json.hpp"
#include <string>
#include <vector>
#include <memory>
#include <ctime>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <functional>
#include <cctype>
#include <cstdio>
#include <filesystem>

using json = nlohmann::json;
using namespace std;
namespace fs = std::filesystem;

namespace ChoreApp
{
  enum class STATUS { NOT_STARTED, IN_PROGRESS, COMPLETED };
  enum class PRIORITY { LOW, MODERATE, HIGH };

  // Capitalizes the first letter and lowercases the rest, e.g. "easy" -> "Easy".
  // Used only for migrating legacy "difficulty" values into category names.
  inline string Titlecase(const string& s) {
    if (s.empty()) return s;
    string result = s;
    result[0] = (char)toupper((unsigned char)result[0]);
    for (size_t i = 1; i < result.size(); i++) {
      result[i] = (char)tolower((unsigned char)result[i]);
    }
    return result;
  }

  inline bool EqualsIgnoreCase(const string& a, const string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) {
      if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return false;
    }
    return true;
  }

  // Zero-padded "YYYY-MM-DD" for the current local date — used for history-log day
  // filtering. Must stay format-identical to main.cpp's TodayDateString()/
  // AddDaysToDateString(), since day lookups round-trip by plain string equality.
  inline string FormatDateNow() {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    stringstream ss;
    ss << put_time(&timeinfo, "%Y-%m-%d");
    return ss.str();
  }

  inline string FormatTimestampNow() {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    stringstream ss;
    ss << put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    return ss.str();
  }

  // A fixed set of friendly, saturated colors used as default chore-doer avatar
  // colors (cycled by id) and available for the user to change via a color picker.
  inline const vector<string> kBubblyPalette = {
    "#FF6B6B", "#4ECDC4", "#FFD93D", "#6C5CE7",
    "#00B894", "#FF9F1C", "#FF6FB5", "#54A0FF"
  };

  class Client {
  private:
    struct Preferences {
      bool notify;
      string theme;  // Using a simple string

      Preferences(const json& j)
        : notify(j.contains("notify") && !j["notify"].is_null() ? j["notify"].get<bool>() : false),
        theme(j.contains("theme") && !j["theme"].is_null() ? j["theme"].get<string>() : "Default") {}
    };

    struct UserProfile {
      string username;
      string last_logged_in; // Always set to the current date and time
      Preferences preferences;

      UserProfile(const json& j)
        : username(j.contains("username") && !j["username"].is_null() ? j["username"].get<string>() : "DefaultUser"),
        last_logged_in(getCurrentDateTime()),
        preferences(j.contains("preferences") && !j["preferences"].is_null() ? j["preferences"] : json{}) {}

      string getCurrentDateTime() {
        time_t now = time(nullptr);
        struct tm timeinfo;
        if (localtime_s(&timeinfo, &now) != 0) {
          throw runtime_error("Failed to convert time to local time.");
        }
        stringstream ss;
        ss << put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
        return ss.str();
      }
    };

    UserProfile userProfile;

  public:
    Client(const json& j)
      : userProfile(j.contains("user_profile") ? j["user_profile"] : json{}) {}

    json toJSON() const {
      return json{
          {"username", userProfile.username},
          {"last_logged_in", userProfile.last_logged_in},
          {"preferences", {{"notify", userProfile.preferences.notify}, {"theme", userProfile.preferences.theme}}}
      };
    }

    string printProfile() const {
      string result;
      result += "Username: " + userProfile.username + "\n";
      result += "Last Logged In: " + (!userProfile.last_logged_in.empty() ? userProfile.last_logged_in : "Never") + "\n";
      result += "Notifications: " + string(userProfile.preferences.notify ? "Enabled" : "Disabled") + "\n";
      result += "Theme: " + (!userProfile.preferences.theme.empty() ? userProfile.preferences.theme : "Default") + "\n";
      return result;
    }

    string getUserName() const { return userProfile.username; }
    string getLastLoggedIn() const { return userProfile.last_logged_in; }
    string getNotify() const { return userProfile.preferences.notify ? "Enabled" : "Disabled"; }
    string getTheme() const { return userProfile.preferences.theme.empty() ? "Default" : userProfile.preferences.theme; }

    void setUsername(const string& newUserName) {
      userProfile.username = newUserName.empty() ? "DefaultUser" : newUserName;
      userProfile.last_logged_in = userProfile.getCurrentDateTime();
    }

    void toggleNotify() {
      userProfile.preferences.notify = !userProfile.preferences.notify;
    }

    void toggleTheme() {
      userProfile.preferences.theme = (userProfile.preferences.theme == "dark") ? "light" : "dark";
    }
  };

  //*********************************************************************************

  class Chore {
  private:
    int id;
    int earnings;

    string name;
    string description;
    string frequency;
    string estimated_time;
    string notes;
    string location;
    string category; // Free-form, user-managed category name (see CategoryRegistry)

    vector<string> days;
    vector<string> tools_required;
    vector<string> materials_needed;
    vector<string> tags;

    STATUS Status;
    PRIORITY Priority;

  public:
    Chore(const json& j) {
      id = j["id"].is_null() ? -1 : j["id"].get<int>();
      name = j["name"].is_null() ? "" : j["name"].get<string>();
      description = j["description"].is_null() ? "" : j["description"].get<string>();
      frequency = j["frequency"].is_null() ? "" : j["frequency"].get<string>();
      estimated_time = j["estimated_time"].is_null() ? "" : j["estimated_time"].get<string>();
      earnings = j["earnings"].is_null() ? 0 : j["earnings"].get<int>();

      days = j["days"].is_null() ? vector<string>() : j["days"].get<vector<string>>();
      location = j["location"].is_null() ? "" : j["location"].get<string>();
      tools_required = j["tools_required"].is_null() ? vector<string>() : j["tools_required"].get<vector<string>>();
      materials_needed = j["materials_needed"].is_null() ? vector<string>() : j["materials_needed"].get<vector<string>>();
      notes = j["notes"].is_null() ? "" : j["notes"].get<string>();
      tags = j["tags"].is_null() ? vector<string>() : j["tags"].get<vector<string>>();
      category = (j.contains("category") && !j["category"].is_null()) ? j["category"].get<string>() : "Uncategorized";

      Priority = parsePriority(j);
      Status = parseStatus(j);
    }

    Chore() = default;
    virtual ~Chore() {}

    virtual string startChore() {
      if (Status == STATUS::NOT_STARTED) {
        Status = STATUS::IN_PROGRESS;
        return "In Progress: " + name;
      }
      else if (Status == STATUS::IN_PROGRESS) {
        return "Chore already started " + name;
      }
      else {
        Status = STATUS::IN_PROGRESS;
        return "In Progress: " + name;
      }
    }

    virtual string completeChore() {
      if (Status == STATUS::NOT_STARTED || Status == STATUS::IN_PROGRESS) {
        Status = STATUS::COMPLETED;
        return "Completed: " + name;
      }
      else {
        return "Chore already completed: " + name;
      }
    }

    virtual string resetChore() {
      if (Status == STATUS::NOT_STARTED) {
        return "Chore not started: " + name;
      }
      else {
        Status = STATUS::NOT_STARTED;
        return "Resetting: " + name;
      }
    }

    virtual json toJSON() const {
      return json{
          {"id", id},
          {"name", name},
          {"description", description},
          {"frequency", frequency},
          {"estimated_time", estimated_time},
          {"earnings", earnings},
          {"days", days},
          {"location", location},
          {"tools_required", tools_required},
          {"materials_needed", materials_needed},
          {"notes", notes},
          {"tags", tags},
          {"category", category},
          {"priority", toStringP(Priority)},
          {"status", toStringS(Status)}
      };
    }

    virtual string PrettyPrintClassAttributes() const {
      string result = "Chore ID: " + to_string(id) + "\n"
        "Name: " + name + "\n"
        "Description: " + description + "\n"
        "Frequency: " + frequency + "\n"
        "Estimated Time: " + estimated_time + "\n"
        "Earnings: " + to_string(earnings) + "\n"
        "Days: " + formatVector(days) + "\n"
        "Location: " + location + "\n"
        "Tools Required: " + formatVector(tools_required) + "\n"
        "Materials Needed: " + formatVector(materials_needed) + "\n"
        "Notes: " + notes + "\n"
        "Tags: " + formatVector(tags) + "\n"
        "Status: " + toStringS(Status) + "\n"
        "Priority: " + toStringP(Priority) + "\n"
        "Category: " + category;

      return result;
    }

    string simplePrint() const {
      return "Chore ID: " + to_string(id) + "\n"
        + "Name: " + name + "\n"
        + "Description: " + description + "\n"
        + "Earnings: " + to_string(earnings) + "\n";
    }

    friend ostream& operator<<(ostream& os, const Chore& chore);

    virtual bool operator==(const Chore& other) const {
      return (this->id == other.id &&
        this->name == other.name &&
        this->description == other.description &&
        this->frequency == other.frequency &&
        this->estimated_time == other.estimated_time &&
        this->earnings == other.earnings &&
        this->days == other.days &&
        this->location == other.location &&
        this->tools_required == other.tools_required &&
        this->materials_needed == other.materials_needed &&
        this->notes == other.notes &&
        this->tags == other.tags &&
        this->category == other.category &&
        this->Status == other.Status &&
        this->Priority == other.Priority);
    }

    bool operator!=(const Chore& other) const {
      return !(*this == other);
    }

    int getId() const { return id; }
    void setId(int newId) { id = newId; }

    string getName() const { return name; }
    void setName(const string& newName) { name = newName; }

    string getDescription() const { return description; }
    void setDescription(const string& newDescription) { description = newDescription; }

    string getFrequency() const { return frequency; }
    void setFrequency(const string& newFrequency) { frequency = newFrequency; }

    string getEstimatedTime() const { return estimated_time; }
    void setEstimatedTime(const string& newTime) { estimated_time = newTime; }

    int getEarnings() const { return earnings; }
    void setEarnings(int newEarnings) { earnings = newEarnings; }

    vector<string> getDays() const { return days; }
    void setDays(const vector<string>& newDays) { days = newDays; }

    string getLocation() const { return location; }
    void setLocation(const string& newLocation) { location = newLocation; }

    vector<string> getToolsRequired() const { return tools_required; }
    void setToolsRequired(const vector<string>& newTools) { tools_required = newTools; }

    vector<string> getMaterialsNeeded() const { return materials_needed; }
    void setMaterialsNeeded(const vector<string>& newMaterials) { materials_needed = newMaterials; }

    string getNotes() const { return notes; }
    void setNotes(const string& newNotes) { notes = newNotes; }

    vector<string> getTags() const { return tags; }
    void setTags(const vector<string>& newTags) { tags = newTags; }

    string getCategory() const { return category; }
    void setCategory(const string& newCategory) { category = newCategory.empty() ? "Uncategorized" : newCategory; }

    STATUS getStatus() const { return Status; }
    void setStatus(STATUS newStatus) { Status = newStatus; }

    PRIORITY getPriority() const { return Priority; }
    void setPriority(PRIORITY newPriority) { Priority = newPriority; }

    static string formatVector(const vector<string>& vec) {
      string result;
      for (const auto& item : vec) {
        if (!result.empty()) result += ", ";
        result += item;
      }
      return result.empty() ? "None" : result;
    }

    PRIORITY parsePriority(const json& j) {
      if (!j.contains("priority") || j["priority"].is_null()) return PRIORITY::LOW;
      string pri = j["priority"].get<string>();
      if (pri == "low") return PRIORITY::LOW;
      if (pri == "moderate") return PRIORITY::MODERATE;
      if (pri == "high") return PRIORITY::HIGH;
      return PRIORITY::LOW;
    }

    STATUS parseStatus(const json& j) {
      if (!j.contains("status") || j["status"].is_null()) return STATUS::NOT_STARTED;
      string stat = j["status"].get<string>();
      if (stat == "not started") return STATUS::NOT_STARTED;
      if (stat == "in progress") return STATUS::IN_PROGRESS;
      if (stat == "completed") return STATUS::COMPLETED;
      return STATUS::NOT_STARTED;
    }

    string toStringS(STATUS s) const {
      switch (s) {
      case STATUS::NOT_STARTED: return "not started";
      case STATUS::IN_PROGRESS: return "in progress";
      case STATUS::COMPLETED: return "completed";
      default: return "not started";
      }
    }

    string toStringP(PRIORITY p) const {
      switch (p) {
      case PRIORITY::LOW: return "low";
      case PRIORITY::MODERATE: return "moderate";
      case PRIORITY::HIGH: return "high";
      default: return "low";
      }
    }

    static STATUS statusFromString(const string& s) {
      if (s == "in progress") return STATUS::IN_PROGRESS;
      if (s == "completed") return STATUS::COMPLETED;
      return STATUS::NOT_STARTED;
    }

    static PRIORITY priorityFromString(const string& p) {
      if (p == "moderate") return PRIORITY::MODERATE;
      if (p == "high") return PRIORITY::HIGH;
      return PRIORITY::LOW;
    }
  };

  inline ostream& operator<<(ostream& os, const Chore& chore) {
    os << chore.PrettyPrintClassAttributes();
    return os;
  }

  //*********************************************************************************

  // A user-managed category name plus an adjustable sort order (categories are free
  // strings now, not a fixed enum, so nothing else can supply a meaningful order).
  struct CategoryEntry {
    string name;
    int sortWeight;
  };

  class CategoryRegistry {
  private:
    vector<CategoryEntry> categories;

    void swapWeights(const string& a, const string& b) {
      int wa = 0, wb = 0;
      for (auto& c : categories) {
        if (c.name == a) wa = c.sortWeight;
        if (c.name == b) wb = c.sortWeight;
      }
      for (auto& c : categories) {
        if (c.name == a) c.sortWeight = wb;
        if (c.name == b) c.sortWeight = wa;
      }
    }

  public:
    CategoryRegistry() {
      categories.push_back({ "Uncategorized", -1 });
    }

    void loadFromJSON(const json& j) {
      if (j.is_null() || !j.is_array() || j.empty()) return; // keep the default Uncategorized-only registry
      categories.clear();
      for (const auto& entry : j) {
        string name = entry.value("name", string("Uncategorized"));
        int weight = entry.value("sort_weight", 0);
        categories.push_back({ name, weight });
      }
      if (!exists("Uncategorized")) {
        categories.push_back({ "Uncategorized", -1 });
      }
    }

    json toJSON() const {
      json arr = json::array();
      for (const auto& c : categories) {
        arr.push_back(json{ {"name", c.name}, {"sort_weight", c.sortWeight} });
      }
      return arr;
    }

    // Returns categories ordered by sortWeight ascending (display order).
    vector<CategoryEntry> listSorted() const {
      vector<CategoryEntry> sorted = categories;
      sort(sorted.begin(), sorted.end(), [](const CategoryEntry& a, const CategoryEntry& b) {
        return a.sortWeight < b.sortWeight;
        });
      return sorted;
    }

    bool exists(const string& name) const {
      for (const auto& c : categories) {
        if (EqualsIgnoreCase(c.name, name)) return true;
      }
      return false;
    }

    // Unknown categories sort last.
    int getSortWeight(const string& name) const {
      for (const auto& c : categories) {
        if (c.name == name) return c.sortWeight;
      }
      return INT_MAX;
    }

    // Returns "" on success, or a user-facing error message.
    string addCategory(const string& name) {
      if (name.empty()) return "Category name cannot be empty.";
      if (exists(name)) return "Category '" + name + "' already exists.";
      int maxWeight = -1;
      for (const auto& c : categories) maxWeight = max(maxWeight, c.sortWeight);
      categories.push_back({ name, maxWeight + 1 });
      return "";
    }

    // Renames the category entry itself; does NOT touch any Chore — callers that need
    // every chore using this category updated should use ChoreManager::renameCategory.
    string renameCategoryEntry(const string& oldName, const string& newName) {
      if (oldName == "Uncategorized") return "'Uncategorized' cannot be renamed.";
      if (newName.empty()) return "New name cannot be empty.";
      if (!EqualsIgnoreCase(oldName, newName) && exists(newName)) return "Category '" + newName + "' already exists.";
      for (auto& c : categories) {
        if (c.name == oldName) { c.name = newName; return ""; }
      }
      return "Category '" + oldName + "' not found.";
    }

    bool removeCategory(const string& name) {
      if (name == "Uncategorized") return false;
      auto it = find_if(categories.begin(), categories.end(), [&](const CategoryEntry& c) { return c.name == name; });
      if (it == categories.end()) return false;
      categories.erase(it);
      return true;
    }

    bool moveUp(const string& name) {
      auto sorted = listSorted();
      for (size_t i = 1; i < sorted.size(); i++) {
        if (sorted[i].name == name) {
          swapWeights(sorted[i].name, sorted[i - 1].name);
          return true;
        }
      }
      return false;
    }

    bool moveDown(const string& name) {
      auto sorted = listSorted();
      for (size_t i = 0; i + 1 < sorted.size(); i++) {
        if (sorted[i].name == name) {
          swapWeights(sorted[i].name, sorted[i + 1].name);
          return true;
        }
      }
      return false;
    }
  };

  // Comparison structures for sorting chores based on specific attributes
  struct CompareEarnings {
    bool operator()(const shared_ptr<Chore>& a, const shared_ptr<Chore>& b) const {
      return a->getEarnings() < b->getEarnings();
    }
  };

  struct CompareCategory {
    const CategoryRegistry& registry;
    CompareCategory(const CategoryRegistry& reg) : registry(reg) {}
    bool operator()(const shared_ptr<Chore>& a, const shared_ptr<Chore>& b) const {
      return registry.getSortWeight(a->getCategory()) < registry.getSortWeight(b->getCategory());
    }
  };

  struct CompareID {
    bool operator()(const shared_ptr<Chore>& a, const shared_ptr<Chore>& b) const {
      return a->getId() < b->getId();
    }
  };

  struct CompareName {
    bool operator()(const shared_ptr<Chore>& a, const shared_ptr<Chore>& b) const {
      return a->getName() < b->getName();
    }
  };

  inline bool matchById(const shared_ptr<Chore>& chore, const int& id) {
    return chore->getId() == id;
  }

  inline bool matchByName(const shared_ptr<Chore>& chore, const string& name) {
    return chore->getName() == name;
  }

  inline bool matchByEarnings(const shared_ptr<Chore>& chore, const int& earnings) {
    return chore->getEarnings() == earnings;
  }

  //*********************************************************************************

  template<typename T>
  class Container {
  private:
    vector<shared_ptr<T>> items;

  public:
    Container() {}

    template<typename Comparator>
    void sortItems(Comparator comp, bool ascending = true) {
      try {
        bool swapped;
        do {
          swapped = false;
          for (size_t i = 1; i < items.size(); i++) {
            if ((ascending && comp(items[i - 1], items[i])) || (!ascending && comp(items[i], items[i - 1]))) {
              swap(items[i - 1], items[i]);
              swapped = true;
            }
          }
        } while (swapped);
      }
      catch (const exception&) {
        // Comparators aren't expected to throw; swallow defensively rather than leave the container half-sorted.
      }
    }

    template<typename Key>
    vector<shared_ptr<T>> searchItem(const Key& key, function<bool(const shared_ptr<T>&, const Key&)> matchCriteria) {
      vector<shared_ptr<T>> results;
      for (auto& item : items) {
        if (matchCriteria(item, key)) {
          results.push_back(item);
        }
      }
      return results;
    }

    void deleteItem(int id) {
      for (auto it = items.begin(); it != items.end(); ++it) {
        if ((*it)->getId() == id) {
          items.erase(it);
          break;
        }
      }
    }

    void push_back(const shared_ptr<T>& item) {
      items.push_back(item);
    }

    size_t size() const { return items.size(); }

    shared_ptr<T>& operator[](size_t index) { return items[index]; }

    string returnAllItems() {
      string info;
      for (const auto& item : items) {
        info += "Chore: " + item->getName() + " (ID: " + std::to_string(item->getId()) + ")\n";
      }
      return info;
    }

    string displayAllItemsAllAttributes() {
      string info;
      for (const auto& item : items) {
        info += item->PrettyPrintClassAttributes() + "\n\n";
      }
      return info;
    }

    bool empty() const { return items.empty(); }
    void clear() { items.clear(); }

    const vector<shared_ptr<T>>& item() const { return items; }

    auto begin() -> decltype(items.begin()) { return items.begin(); }
    auto end() -> decltype(items.end()) { return items.end(); }
    auto begin() const -> decltype(items.begin()) const { return items.begin(); }
    auto end() const -> decltype(items.end()) const { return items.end(); }
  };

  //*********************************************************************************

  class ChoreDoer {
  public:
    Container<Chore> assignedChores;

    ChoreDoer(const string& name) : name(name) {
      age = 0;
      choreAmount = 0;
      totalEarnings = 0;
      assignedChores = Container<Chore>();
      id = 0;
      avatarColorHex = "";
      notes = "";
    }

    void assignChore(const shared_ptr<Chore>& chore) {
      assignedChores.push_back(chore);
      choreAmount++;
    }

    void removeChore(int choreId) {
      size_t before = assignedChores.size();
      assignedChores.deleteItem(choreId);
      if (assignedChores.size() < before) {
        decreaseChoreAmount();
      }
    }

    int getId() const { return id; }
    void setId(int newId) { id = newId; }

    int getAge() const { return age; }

    void setTotalEarnings(int newTotal) { totalEarnings = newTotal; }

    string getName() const { return name; }
    void setName(const string& newName) { name = newName; }

    int getTotalEarnings() const { return totalEarnings; }
    int getChoreAmount() const { return choreAmount; }

    string getAvatarColor() const { return avatarColorHex; }
    void setAvatarColor(const string& hex) { avatarColorHex = hex; }

    string getNotes() const { return notes; }
    void setNotes(const string& newNotes) { notes = newNotes; }

    void resetChoreAmount() { choreAmount = 0; }

    void decreaseChoreAmount() {
      if (choreAmount > 0) choreAmount--;
    }

    string printChoreDoer() const {
      string result = "Chore Doer: " + name + "\n";
      result += "Total Earnings: $" + to_string(totalEarnings) + "\n";
      result += "Chore Amount: " + to_string(choreAmount) + "\n";
      return result;
    }

    void sortAssignedChoresByEarnings(bool ascending = true) {
      if (!assignedChores.empty()) assignedChores.sortItems(CompareEarnings(), ascending);
    }

    void sortAssignedChoresByCategory(const CategoryRegistry& registry, bool ascending = true) {
      if (!assignedChores.empty()) assignedChores.sortItems(CompareCategory(registry), ascending);
    }

    void sortAssignedChoresByID(bool ascending = true) {
      if (!assignedChores.empty()) assignedChores.sortItems(CompareID(), ascending);
    }

    void sortAssignedChoresByName(bool ascending = true) {
      if (!assignedChores.empty()) assignedChores.sortItems(CompareName(), ascending);
    }

    string startChore(int choreId) {
      for (auto& chore : assignedChores) {
        if (chore->getId() == choreId) return chore->startChore();
      }
      return "Chore ID " + to_string(choreId) + " not found among assigned chores.";
    }

    string completeChore(int choreId) {
      for (auto& chore : assignedChores) {
        if (chore->getId() == choreId) {
          if (chore->getStatus() == STATUS::IN_PROGRESS || chore->getStatus() == STATUS::NOT_STARTED) {
            string result = chore->completeChore();
            totalEarnings += chore->getEarnings();
            result += "\nTotal earnings now: $" + to_string(totalEarnings);
            return result;
          }
          return "Chore " + chore->getName() + " is already completed. No action taken.";
        }
      }
      return "Chore ID " + to_string(choreId) + " not found among assigned chores.";
    }

    string resetChore(int choreId) {
      for (auto& chore : assignedChores) {
        if (chore->getId() == choreId) return chore->resetChore();
      }
      return "Chore ID " + to_string(choreId) + " not found among assigned chores.";
    }

    string returnAssignedChores() { return assignedChores.returnAllItems(); }

    friend ostream& operator<<(ostream& os, const ChoreDoer& chDoer);

  private:
    string name;
    int choreAmount;
    int age;
    int totalEarnings;
    int id;
    string avatarColorHex;
    string notes;
  };

  inline ostream& operator<<(ostream& os, const ChoreDoer& chDoer) {
    os << chDoer.printChoreDoer();
    return os;
  }

  //*********************************************************************************

  // A single dated entry in a household's status timeline. doerName/choreName are
  // snapshots taken at logging time, so the log still reads sensibly even if a doer
  // or chore is later renamed or deleted.
  struct HistoryEvent {
    string date;      // "YYYY-MM-DD" — for day filtering
    string timestamp;  // "YYYY-MM-DD HH:MM:SS" — for within-day ordering/display
    string action;     // "assigned" | "started" | "completed" | "reset"
    int doerId;
    string doerName;
    int choreId;
    string choreName;
    int earnings;

    HistoryEvent() : doerId(0), choreId(0), earnings(0) {}

    HistoryEvent(const string& action, const shared_ptr<ChoreDoer>& doer, const shared_ptr<Chore>& chore)
      : date(FormatDateNow()), timestamp(FormatTimestampNow()), action(action),
      doerId(doer->getId()), doerName(doer->getName()),
      choreId(chore->getId()), choreName(chore->getName()), earnings(chore->getEarnings()) {}

    json toJSON() const {
      return json{
        {"date", date}, {"timestamp", timestamp}, {"action", action},
        {"doer_id", doerId}, {"doer_name", doerName},
        {"chore_id", choreId}, {"chore_name", choreName}, {"earnings", earnings}
      };
    }

    static HistoryEvent fromJSON(const json& j) {
      HistoryEvent e;
      e.date = j.value("date", string(""));
      e.timestamp = j.value("timestamp", string(""));
      e.action = j.value("action", string(""));
      e.doerId = j.value("doer_id", 0);
      e.doerName = j.value("doer_name", string(""));
      e.choreId = j.value("chore_id", 0);
      e.choreName = j.value("chore_name", string(""));
      e.earnings = j.value("earnings", 0);
      return e;
    }
  };

  class ChoreManager {
  private:
    json j;
    int choreCount;
    Container<Chore> Chores;
    Container<ChoreDoer> ChoreDoers;
    unique_ptr<Client> client;
    string dynamicFile;
    string householdName;
    int nextChoreDoerId;
    int nextChoreId;
    CategoryRegistry registry;
    vector<HistoryEvent> history;
    static const size_t kMaxHistoryEvents = 10000;

    // Appends a dated event and trims the oldest entries if the log has grown past
    // the retention cap. saveData() fully re-serializes the whole file on nearly every
    // action, so an uncapped log would make every save progressively slower over time,
    // not just grow disk usage — hence the cap here rather than "no limit for now".
    void logEvent(const string& action, const shared_ptr<ChoreDoer>& doer, const shared_ptr<Chore>& chore) {
      if (!doer || !chore) return;
      history.push_back(HistoryEvent(action, doer, chore));
      if (history.size() > kMaxHistoryEvents) {
        history.erase(history.begin(), history.begin() + (history.size() - kMaxHistoryEvents));
      }
    }

    void clearAll() {
      try {
        clearChores();
        clearAllAssignedChores();
        clearAllChoreDoers();
      }
      catch (const exception&) {
      }
    }

    void clearChores() {
      try {
        if (!Chores.empty()) Chores.clear();
      }
      catch (const exception&) {
      }
    }

    void clearAllAssignedChores() {
      try {
        if (!ChoreDoers.empty()) {
          for (auto& doer : ChoreDoers) {
            if (!doer->assignedChores.empty()) doer->assignedChores.clear();
          }
        }
      }
      catch (const exception&) {
      }
    }

    void clearAllChoreDoers() {
      if (!ChoreDoers.empty()) ChoreDoers.clear();
    }

    // Detects a pre-category-system household file (no "categories" key) and migrates
    // it in place, in memory, before loadChores() ever sees it: seeds the category
    // registry from whatever "difficulty" values are present, folds each chore's old
    // multitasking_tips/variations/subtasks into its notes, and drops the legacy keys.
    // Returns true if a migration actually ran (so the caller can persist it right away
    // instead of leaving the on-disk file in the old format until some unrelated save).
    bool migrateLegacyDataIfNeeded() {
      if (j.contains("categories")) {
        return false; // already on the new model
      }
      if (!j.contains("chores") || !j["chores"].is_array()) {
        j["categories"] = json::array();
        return true;
      }

      vector<CategoryEntry> seeded;
      int nextWeight = 0;

      for (auto& choreJson : j["chores"]) {
        string legacyDifficulty = choreJson.value("difficulty", string(""));
        string categoryName = legacyDifficulty.empty() ? "Uncategorized" : Titlecase(legacyDifficulty);

        if (!legacyDifficulty.empty() &&
          find_if(seeded.begin(), seeded.end(), [&](const CategoryEntry& e) { return e.name == categoryName; }) == seeded.end()) {
          seeded.push_back({ categoryName, nextWeight++ });
        }

        choreJson["category"] = categoryName;
        choreJson.erase("difficulty");

        string notes = choreJson.value("notes", string(""));

        if (choreJson.contains("multitasking_tips") && choreJson["multitasking_tips"].is_string()) {
          string tips = choreJson["multitasking_tips"].get<string>();
          if (!tips.empty()) {
            if (!notes.empty()) notes += "\n";
            notes += "Multitasking Tips: " + tips;
          }
          choreJson.erase("multitasking_tips");
        }

        if (choreJson.contains("variations") && choreJson["variations"].is_array()) {
          vector<string> variations = choreJson["variations"].get<vector<string>>();
          if (!variations.empty()) {
            string joined;
            for (const auto& v : variations) {
              if (!joined.empty()) joined += ", ";
              joined += v;
            }
            if (!notes.empty()) notes += "\n";
            notes += "Variations: " + joined;
          }
          choreJson.erase("variations");
        }

        if (choreJson.contains("subtasks") && choreJson["subtasks"].is_array()) {
          for (const auto& subtaskJson : choreJson["subtasks"]) {
            string subName = subtaskJson.value("name", string(""));
            string subTime = subtaskJson.value("estimated_time", string(""));
            int subEarnings = subtaskJson.value("earnings", 0);
            if (!notes.empty()) notes += "\n";
            notes += "Subtask: " + subName + " (Time: " + subTime + ", Earnings: $" + to_string(subEarnings) + ")";
          }
          choreJson.erase("subtasks");
        }

        choreJson["notes"] = notes;
      }

      json registryJson = json::array();
      registryJson.push_back(json{ {"name", "Uncategorized"}, {"sort_weight", -1} });
      for (const auto& entry : seeded) {
        registryJson.push_back(json{ {"name", entry.name}, {"sort_weight", entry.sortWeight} });
      }
      j["categories"] = registryJson;
      return true;
    }

  public:
    shared_ptr<ChoreDoer> findChoreDoerByName(const string& name) {
      auto it = find_if(ChoreDoers.begin(), ChoreDoers.end(), [&name](const shared_ptr<ChoreDoer>& d) {
        return d->getName() == name;
        });
      return (it != ChoreDoers.end()) ? *it : nullptr;
    }

    shared_ptr<Chore> findChoreById(int choreId) const {
      auto it = find_if(Chores.begin(), Chores.end(), [choreId](const shared_ptr<Chore>& chore) {
        return chore->getId() == choreId;
        });
      return (it != Chores.end()) ? *it : nullptr;
    }

    const Container<Chore>& getChores() const { return Chores; }
    const Container<ChoreDoer>& getChoreDoers() const { return ChoreDoers; }
    const CategoryRegistry& getCategoryRegistry() const { return registry; }

    Client& getClient() const { return *client; }

    string getHouseholdName() const { return householdName; }
    void setHouseholdName(const string& newName) { householdName = newName.empty() ? "Household" : newName; }

    ChoreManager(string fileName) {
      try {
        dynamicFile = fileName;

        ifstream file(fileName);
        if (file.is_open()) {
          if (j.is_null()) {
            j = json::parse(file);
          }
          file.close();
        }
        // If the file doesn't exist yet, j simply stays null and the manager starts
        // empty — the next saveData() call creates it.

        client = make_unique<Client>(j);
        householdName = j.value("household_name", string("Household"));
        choreCount = 0;
        nextChoreDoerId = 1;
        nextChoreId = 1;

        bool migrated = migrateLegacyDataIfNeeded();
        registry.loadFromJSON(j.value("categories", json::array()));

        loadChores();
        loadChoreDoers();
        loadHistory();

        for (const auto& chore : Chores) {
          if (chore->getId() >= nextChoreId) nextChoreId = chore->getId() + 1;
        }

        // Persist the migration immediately so the on-disk file reflects the new
        // category-based model right away, rather than staying in the old format
        // until some unrelated later action happens to trigger a save.
        if (migrated) {
          saveData();
        }
      }
      catch (const json::exception&) {
        throw; // Rethrow for the caller to handle (the GUI shows this in a dialog)
      }
      catch (const ifstream::failure&) {
        throw;
      }
    }

    ~ChoreManager() {
      clearAll();
    }

    void loadChores() {
      try {
        if (j.contains("chores") && j["chores"].is_array()) {
          for (auto& choreJson : j["chores"]) {
            addChore(choreJson);
            choreCount++;
          }
        }
      }
      catch (const json::exception&) {
        // A malformed "chores" entry leaves Chores partially loaded rather than crashing.
      }
    }

    // Must run after loadChores() so assigned chore IDs can be resolved to the same
    // shared_ptr<Chore> instances already held by the Chores container.
    void loadChoreDoers() {
      try {
        if (j.contains("chore_doers") && j["chore_doers"].is_array()) {
          for (auto& doerJson : j["chore_doers"]) {
            string name = doerJson.value("name", string("Unnamed"));
            auto doer = make_shared<ChoreDoer>(name);

            int storedId = doerJson.value("id", 0);
            doer->setId(storedId);
            doer->setTotalEarnings(doerJson.value("total_earnings", 0));
            doer->setNotes(doerJson.value("notes", string("")));

            // Legacy household files predate avatar colors — self-migrate to a
            // palette default on first load, same as any other doer would get.
            string color = doerJson.value("avatar_color", string(""));
            if (color.empty()) {
              color = kBubblyPalette[(storedId > 0 ? storedId - 1 : 0) % kBubblyPalette.size()];
            }
            doer->setAvatarColor(color);

            if (doerJson.contains("assigned_chore_ids") && doerJson["assigned_chore_ids"].is_array()) {
              for (auto& idVal : doerJson["assigned_chore_ids"]) {
                int choreId = idVal.get<int>();
                auto matches = Chores.searchItem<int>(choreId, matchById);
                if (!matches.empty()) {
                  doer->assignChore(matches[0]); // Shares the same shared_ptr<Chore> as Chores
                }
              }
            }

            if (storedId >= nextChoreDoerId) {
              nextChoreDoerId = storedId + 1;
            }

            ChoreDoers.push_back(doer);
          }
        }
      }
      catch (const json::exception&) {
        // A malformed "chore_doers" entry leaves ChoreDoers partially loaded rather than crashing.
      }
    }

    void loadHistory() {
      try {
        if (j.contains("history") && j["history"].is_array()) {
          for (auto& eventJson : j["history"]) {
            history.push_back(HistoryEvent::fromJSON(eventJson));
          }
        }
      }
      catch (const json::exception&) {
        // A malformed "history" entry leaves the log partially loaded rather than crashing.
      }
    }

    // Returns an empty string on success, or a description of why nothing was written.
    // Throws runtime_error if the output file genuinely can't be opened.
    string outputChoreAssignmentsToFile(const string& outputPath) {
      if (outputPath.empty()) {
        return "Output path is empty!";
      }
      if (ChoreDoers.empty()) {
        return "No Chore Doers available to output!";
      }

      bool hasAssignedChores = false;
      for (const auto& doer : ChoreDoers) {
        if (!doer->assignedChores.empty()) {
          hasAssignedChores = true;
          break;
        }
      }
      if (!hasAssignedChores) {
        return "No chores assigned to any Chore Doer!";
      }

      json output;
      output["user_profile"] = client->toJSON();
      output["chore_doers"] = json::array();
      for (const auto& doer : ChoreDoers) {
        json doerJson = json::object({
          {"chore_count", doer->getChoreAmount() },
          {"name", doer->getName()},
          {"chores", json::array()}
          });
        for (const auto& chore : doer->assignedChores) {
          doerJson["chores"].push_back(chore->toJSON());
        }
        output["chore_doers"].push_back(doerJson);
      }

      ofstream outFile(outputPath);
      if (!outFile.is_open()) {
        throw runtime_error("Could not open file to write chore assignments.\n");
      }
      outFile << setw(4) << output;
      outFile.close();
      return "";
    }

    json toJSON() const {
      json output;
      output["household_name"] = householdName;
      output["chores"] = json::array();
      for (const auto& chore : Chores) {
        output["chores"].push_back(chore->toJSON());
      }
      if (client) {
        output["user_profile"] = client->toJSON();
      }
      output["chore_doers"] = json::array();
      for (const auto& doer : ChoreDoers) {
        json assignedIds = json::array();
        for (const auto& chore : doer->assignedChores) {
          assignedIds.push_back(chore->getId());
        }
        output["chore_doers"].push_back(json{
          {"id", doer->getId()},
          {"name", doer->getName()},
          {"age", doer->getAge()},
          {"total_earnings", doer->getTotalEarnings()},
          {"chore_amount", doer->getChoreAmount()},
          {"assigned_chore_ids", assignedIds},
          {"avatar_color", doer->getAvatarColor()},
          {"notes", doer->getNotes()}
          });
      }
      output["categories"] = registry.toJSON();
      output["history"] = json::array();
      for (const auto& event : history) {
        output["history"].push_back(event.toJSON());
      }
      return output;
    }

    void saveData() {
      j.clear();
      j = toJSON();

      ofstream file(dynamicFile);
      if (file) {
        file << setw(4) << j << endl;
      }
      // If the file can't be opened for writing (permissions, disk full, etc.) the
      // save is silently skipped rather than crashing — a pre-existing limitation.
      file.close();
    }

    string displayChoreDoerList() {
      if (ChoreDoers.empty()) return "No chore doers available.\n";
      ostringstream info;
      for (const auto& doer : ChoreDoers) {
        info << *doer << "\n";
      }
      return info.str();
    }

    string displayChoreList() {
      if (Chores.empty()) return "No chores available.\n";
      string info;
      for (const auto& chore : Chores) {
        info += chore->simplePrint() + "\n";
      }
      return info;
    }

    string displayAssignedChores(const string& doerName) {
      auto doer = find_if(ChoreDoers.begin(), ChoreDoers.end(), [&doerName](shared_ptr<ChoreDoer>& d) {
        return d->getName() == doerName;
        });
      if (doer != ChoreDoers.end()) {
        string info = "Chore Doer: " + (*doer)->getName() + " has chores:\n";
        info += (*doer)->returnAssignedChores();
        return info;
      }
      return "Chore Doer not found.\n";
    }

    string displayAllChoresAllAttributes() {
      if (Chores.empty()) return "No chores available.\n";
      return Chores.displayAllItemsAllAttributes();
    }

    string displayAllChoreAssignments() {
      if (ChoreDoers.empty()) return "No chore doers available.\n";

      string info;
      bool anyChoresAssigned = false;

      for (auto& doer : ChoreDoers) {
        auto choresInfo = doer->returnAssignedChores();
        if (!choresInfo.empty()) {
          anyChoresAssigned = true;
          info += "Chore Doer: " + doer->getName() + " has chores:\n";
          info += choresInfo;
          info += "\n";
        }
        else {
          info += "Chore Doer: " + doer->getName() + " has no assigned chores.\n\n";
        }
      }

      if (!anyChoresAssigned) return "No chores have been assigned to any chore doer.\n";
      return info;
    }

    // Returns true if the chore was found and deleted.
    bool deleteChoreFromAvailable(int choreId) {
      if (Chores.empty() || choreId < 0) return false;
      size_t sizeBefore = Chores.size();
      Chores.deleteItem(choreId);
      if (Chores.size() < sizeBefore) {
        choreCount--;
        return true;
      }
      return false;
    }

    void addChoreDoer(const string& name) {
      auto doer = make_shared<ChoreDoer>(name);
      doer->setId(nextChoreDoerId++);
      doer->setAvatarColor(kBubblyPalette[(doer->getId() - 1) % kBubblyPalette.size()]);
      ChoreDoers.push_back(doer);
    }

    // Returns true if a chore doer with this name was found and deleted.
    bool deleteChoreDoer(const string& name) {
      for (const auto& doer : ChoreDoers) {
        if (doer->getName() == name) {
          ChoreDoers.deleteItem(doer->getId());
          return true;
        }
      }
      return false;
    }

    void addChore(const json& choreJson) {
      if (choreJson.is_null()) return;
      Chores.push_back(make_shared<Chore>(choreJson));
    }

    // Creates a brand-new chore from a fields blob (as produced by the GUI's chore
    // editor), stamping a fresh ID and defaulting status if not supplied. Auto-registers
    // an unrecognized category defensively (the GUI is expected to have already
    // confirmed adding it via ChoreEditorDialog). Returns the new chore's ID.
    int createChore(json fields) {
      fields["id"] = nextChoreId;
      string status = fields.value("status", string(""));
      if (status.empty()) fields["status"] = "not started";

      string categoryName = fields.value("category", string("Uncategorized"));
      if (!registry.exists(categoryName)) {
        registry.addCategory(categoryName);
      }

      addChore(fields);
      return nextChoreId++;
    }

    int countChoresInCategory(const string& name) const {
      int count = 0;
      for (const auto& chore : Chores) {
        if (chore->getCategory() == name) count++;
      }
      return count;
    }

    string createCategory(const string& name) {
      return registry.addCategory(name);
    }

    string renameCategory(const string& oldName, const string& newName) {
      string err = registry.renameCategoryEntry(oldName, newName);
      if (!err.empty()) return err;
      for (auto& chore : Chores) {
        if (chore->getCategory() == oldName) chore->setCategory(newName);
      }
      return "";
    }

    string deleteCategory(const string& name) {
      if (name == "Uncategorized") return "'Uncategorized' cannot be deleted.";
      int usage = countChoresInCategory(name);
      if (usage > 0) {
        return to_string(usage) + " chore(s) use this category — reassign them first.";
      }
      if (!registry.removeCategory(name)) return "Category '" + name + "' not found.";
      return "";
    }

    bool moveCategoryUp(const string& name) { return registry.moveUp(name); }
    bool moveCategoryDown(const string& name) { return registry.moveDown(name); }

    // Returns an empty string on success, or a description of why nothing was assigned.
    string assignChoresRandomly() {
      try {
        if (Chores.empty()) return "No chores to assign.";
        if (ChoreDoers.empty()) return "No chore doers available.";

        std::random_device rd;
        std::mt19937 g(rd());
        shuffle(Chores.begin(), Chores.end(), g);

        size_t choreIndex = 0;
        while (choreIndex < Chores.size()) {
          for (auto& doer : ChoreDoers) {
            if (choreIndex < Chores.size()) {
              auto& chore = Chores[choreIndex++];
              doer->assignChore(chore);
              logEvent("assigned", doer, chore);
            }
            else {
              break;
            }
          }
        }
        return "";
      }
      catch (const std::exception& e) {
        return string("Exception thrown in assignChoresRandomly: ") + e.what();
      }
    }

    vector<shared_ptr<Chore>> searchByID(int id) {
      auto resultsById = Chores.searchItem<int>(id, matchById);
      if (resultsById.empty()) {
        for (auto& doer : ChoreDoers) {
          auto doerResults = doer->assignedChores.searchItem<int>(id, matchById);
          if (!doerResults.empty()) resultsById.insert(resultsById.end(), doerResults.begin(), doerResults.end());
        }
      }
      return resultsById;
    }

    vector<shared_ptr<Chore>> searchByName(const string& name) {
      auto resultsName = Chores.searchItem<string>(name, matchByName);
      if (resultsName.empty()) {
        for (auto& doer : ChoreDoers) {
          auto doerResults = doer->assignedChores.searchItem<string>(name, matchByName);
          if (!doerResults.empty()) resultsName.insert(resultsName.end(), doerResults.begin(), doerResults.end());
        }
      }
      return resultsName;
    }

    vector<shared_ptr<Chore>> searchByEarnings(int earnings) {
      auto resultsEarned = Chores.searchItem<int>(earnings, matchByEarnings);
      if (resultsEarned.empty()) {
        for (auto& doer : ChoreDoers) {
          auto doerResults = doer->assignedChores.searchItem<int>(earnings, matchByEarnings);
          if (!doerResults.empty()) resultsEarned.insert(resultsEarned.end(), doerResults.begin(), doerResults.end());
        }
      }
      return resultsEarned;
    }

    void sortChoresByEarnings(bool ascending = true) {
      if (!Chores.empty()) Chores.sortItems(CompareEarnings(), ascending);
    }

    void sortChoresByCategory(bool ascending = true) {
      if (!Chores.empty()) Chores.sortItems(CompareCategory(registry), ascending);
    }

    void sortChoresByID(bool ascending = true) {
      if (!Chores.empty()) Chores.sortItems(CompareID(), ascending);
    }

    void sortChoresByName(bool ascending = true) {
      if (!Chores.empty()) Chores.sortItems(CompareName(), ascending);
    }

    void sortAllChoreDoersChoresByEarnings(bool ascending = true) {
      for (auto& doer : ChoreDoers) doer->sortAssignedChoresByEarnings(ascending);
    }

    void sortAllChoreDoersChoresByCategory(bool ascending = true) {
      for (auto& doer : ChoreDoers) doer->sortAssignedChoresByCategory(registry, ascending);
    }

    void sortAllChoreDoersChoresByID(bool ascending = true) {
      for (auto& doer : ChoreDoers) doer->sortAssignedChoresByID(ascending);
    }

    void sortAllChoreDoersChoresByName(bool ascending = true) {
      for (auto& doer : ChoreDoers) doer->sortAssignedChoresByName(ascending);
    }

    string compareChores(int id1, int id2) {
      if (id1 < 0 || id2 < 0) return "Invalid chore ID provided. ID must be non-negative.";
      auto chore1 = find_if(Chores.begin(), Chores.end(), [id1](const shared_ptr<Chore>& chore) {
        return chore->getId() == id1;
        });
      auto chore2 = find_if(Chores.begin(), Chores.end(), [id2](const shared_ptr<Chore>& chore) {
        return chore->getId() == id2;
        });
      if (chore1 != Chores.end() && chore2 != Chores.end()) {
        return (**chore1 == **chore2) ? "The two chores are identical." : "The two chores are not identical.";
      }
      return "One or both chore IDs not found.";
    }

    // Start/complete/reset an assigned chore, logging a history event whenever the
    // chore's status actually changes (not on a no-op like re-completing an already
    // completed chore). This is the layer where logging belongs, since only
    // ChoreManager owns the history log — ChoreDoer/Chore stay unaware of it.
    string startDoerChore(const string& doerName, int choreId) {
      auto doer = findChoreDoerByName(doerName);
      auto chore = findChoreById(choreId);
      if (!doer) return "Chore Doer '" + doerName + "' not found.";
      STATUS before = chore ? chore->getStatus() : STATUS::NOT_STARTED;
      string result = doer->startChore(choreId);
      if (chore && chore->getStatus() != before) {
        logEvent("started", doer, chore);
      }
      return result;
    }

    string completeDoerChore(const string& doerName, int choreId) {
      auto doer = findChoreDoerByName(doerName);
      auto chore = findChoreById(choreId);
      if (!doer) return "Chore Doer '" + doerName + "' not found.";
      STATUS before = chore ? chore->getStatus() : STATUS::NOT_STARTED;
      string result = doer->completeChore(choreId);
      if (chore && chore->getStatus() != before) {
        logEvent("completed", doer, chore);
      }
      return result;
    }

    string resetDoerChore(const string& doerName, int choreId) {
      auto doer = findChoreDoerByName(doerName);
      auto chore = findChoreById(choreId);
      if (!doer) return "Chore Doer '" + doerName + "' not found.";
      STATUS before = chore ? chore->getStatus() : STATUS::NOT_STARTED;
      string result = doer->resetChore(choreId);
      if (chore && chore->getStatus() != before) {
        logEvent("reset", doer, chore);
      }
      return result;
    }

    vector<HistoryEvent> getEventsForDate(const string& date) const {
      vector<HistoryEvent> results;
      for (const auto& event : history) {
        if (event.date == date) results.push_back(event);
      }
      return results;
    }

    string getEarliestHistoryDate() const {
      if (history.empty()) return "";
      string earliest = history[0].date;
      for (const auto& event : history) {
        if (event.date < earliest) earliest = event.date;
      }
      return earliest;
    }

    string getLatestHistoryDate() const {
      if (history.empty()) return "";
      string latest = history[0].date;
      for (const auto& event : history) {
        if (event.date > latest) latest = event.date;
      }
      return latest;
    }

    // Completion streak: the number of consecutive calendar days, walking backward
    // from today (or from yesterday if today has no completed event yet, so a streak
    // survives until the day is actually over), on which this doer has at least one
    // logged "completed" event. There is no daily-reset mechanic in this app — a
    // chore stays assigned until manually changed — so this is a deliberately simple,
    // precisely-defined approximation of "how consistently has this doer been
    // finishing chores lately," not a claim about any particular chore recurring.
    int getDoerStreak(int doerId) const {
      auto hasCompletedOn = [&](const string& date) {
        for (const auto& event : history) {
          if (event.doerId == doerId && event.action == "completed" && event.date == date) return true;
        }
        return false;
        };

      string cursor = FormatDateNow();
      if (!hasCompletedOn(cursor)) {
        cursor = AddDaysToDate(cursor, -1);
        if (!hasCompletedOn(cursor)) return 0;
      }

      int streak = 0;
      while (hasCompletedOn(cursor)) {
        streak++;
        cursor = AddDaysToDate(cursor, -1);
      }
      return streak;
    }

  private:
    // Adds (or subtracts) whole days from a "YYYY-MM-DD" string, correctly normalizing
    // month/year rollovers via mktime(). Local to ChoreManager since it's only needed
    // for streak calculation here — the GUI has its own copy for date-nav display.
    static string AddDaysToDate(const string& yyyyMMdd, int deltaDays) {
      struct tm timeinfo = {};
      sscanf_s(yyyyMMdd.c_str(), "%d-%d-%d", &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday);
      timeinfo.tm_year -= 1900;
      timeinfo.tm_mon -= 1;
      timeinfo.tm_hour = 12; // noon, to sidestep DST edge cases
      timeinfo.tm_mday += deltaDays;
      time_t asTime = mktime(&timeinfo);
      struct tm normalized;
      localtime_s(&normalized, &asTime);
      stringstream ss;
      ss << put_time(&normalized, "%Y-%m-%d");
      return ss.str();
    }
  };

  //*********************************************************************************

  // A household on disk: its display name plus the file path holding its data.
  struct HouseholdInfo {
    string displayName;
    string filePath;
  };

  // Owns the set of known households (one JSON file per household under a shared
  // directory) plus which one was last open, persisted across restarts. ChoreManager
  // itself stays unaware that "household" exists as a cross-file concept — it only
  // ever opens the single path it's handed.
  class HouseholdRegistry {
  private:
    vector<HouseholdInfo> households;
    string householdsDir;
    string appStatePath;
    string lastOpenPath;

    static string Slugify(const string& name) {
      string result;
      for (char c : name) {
        if (isalnum((unsigned char)c)) {
          result += (char)tolower((unsigned char)c);
        }
        else if (!result.empty() && result.back() != '_') {
          result += '_';
        }
      }
      while (!result.empty() && result.back() == '_') result.pop_back();
      if (result.empty()) result = "household";
      return result;
    }

    static string ReadHouseholdName(const string& filePath) {
      ifstream file(filePath);
      if (!file.is_open()) return "";
      try {
        json data = json::parse(file);
        return data.value("household_name", string(""));
      }
      catch (...) {
        return "";
      }
    }

    void rescan() {
      households.clear();
      if (!fs::exists(householdsDir)) return;
      for (const auto& entry : fs::directory_iterator(householdsDir)) {
        if (entry.path().extension() == ".json") {
          string path = entry.path().string();
          string name = ReadHouseholdName(path);
          if (name.empty()) name = entry.path().stem().string();
          households.push_back({ name, path });
        }
      }
    }

  public:
    // householdsDirectory: e.g. "<TestData>/households". appStateFilePath: e.g.
    // "<TestData>/app_state.json". legacyDataFile: e.g. "<TestData>/data.json" — if
    // present and no households exist yet, it's migrated into the first household
    // (named "Default") so upgrading users keep everything they had.
    HouseholdRegistry(const string& householdsDirectory, const string& appStateFilePath, const string& legacyDataFile) {
      householdsDir = householdsDirectory;
      appStatePath = appStateFilePath;

      fs::create_directories(householdsDir);
      rescan();

      if (households.empty()) {
        string defaultPath = householdsDir + "/default.json";
        json fresh;
        if (fs::exists(legacyDataFile)) {
          ifstream in(legacyDataFile);
          try {
            fresh = json::parse(in);
          }
          catch (...) {
            fresh = json{};
          }
        }
        fresh["household_name"] = "Default";
        if (!fresh.contains("chores")) fresh["chores"] = json::array();
        if (!fresh.contains("chore_doers")) fresh["chore_doers"] = json::array();
        ofstream out(defaultPath);
        out << setw(4) << fresh;
        out.close();
        rescan();
      }

      ifstream stateIn(appStatePath);
      if (stateIn.is_open()) {
        try {
          json state = json::parse(stateIn);
          lastOpenPath = state.value("last_open_household_file", string(""));
        }
        catch (...) {
        }
      }
      if (lastOpenPath.empty() || !fs::exists(lastOpenPath)) {
        lastOpenPath = households.empty() ? "" : households[0].filePath;
      }
    }

    const vector<HouseholdInfo>& listHouseholds() const { return households; }

    string getLastOpenHouseholdPath() const { return lastOpenPath; }

    void setLastOpenHousehold(const string& filePath) {
      lastOpenPath = filePath;
      ofstream out(appStatePath);
      out << setw(4) << json{ {"last_open_household_file", filePath} };
    }

    // Returns the new household's file path on success, or "" with errorMsg set.
    string createHousehold(const string& displayName, string& errorMsg) {
      if (displayName.empty()) {
        errorMsg = "Household name cannot be empty.";
        return "";
      }
      string slug = Slugify(displayName);
      string path = householdsDir + "/" + slug + ".json";
      int suffix = 2;
      while (fs::exists(path)) {
        path = householdsDir + "/" + slug + "_" + to_string(suffix++) + ".json";
      }

      json fresh = {
        {"household_name", displayName},
        {"chores", json::array()},
        {"chore_doers", json::array()},
        {"categories", json::array({ json{{"name", "Uncategorized"}, {"sort_weight", -1}} })}
      };
      ofstream out(path);
      if (!out) {
        errorMsg = "Could not create household file.";
        return "";
      }
      out << setw(4) << fresh;
      out.close();
      rescan();
      return path;
    }

    // Renames a household that is NOT currently open (direct file read-modify-write —
    // the currently-open one should be renamed via ChoreManager::setHouseholdName +
    // saveData() instead, since it holds the live in-memory copy).
    string renameHousehold(const string& filePath, const string& newName) {
      ifstream in(filePath);
      if (!in.is_open()) return "Could not open household file.";
      json data;
      try {
        data = json::parse(in);
      }
      catch (...) {
        return "Household file is corrupted.";
      }
      in.close();
      data["household_name"] = newName;
      ofstream out(filePath);
      if (!out) return "Could not write household file.";
      out << setw(4) << data;
      out.close();
      rescan();
      return "";
    }

    // Returns "" on success, or a user-facing error message. Blocks deleting the
    // currently-open household — the caller should already prevent this in the UI,
    // this is a defensive backstop against a stale file path.
    string deleteHousehold(const string& filePath) {
      if (filePath == lastOpenPath) {
        return "Cannot delete the currently open household — switch to another one first.";
      }
      error_code ec;
      fs::remove(filePath, ec);
      if (ec) return "Could not delete household file.";
      rescan();
      return "";
    }
  };
}
