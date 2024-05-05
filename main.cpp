
#pragma warning( push )
#pragma warning(disable:26819)
#include "json.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cstdlib>  // For rand() and srand()
#include <ctime>    // For time()
#include <fstream>
#include <random>
#include <sstream>
#pragma warning( pop )

using json = nlohmann::json;
using namespace std;

//// Define the starting path to the test data file
const string DATA_FILE_PATH = "TestData\\";

namespace ChoreApp
{

  //ENUMS for handing data that should never change only shifted
  enum class DIFFICULTY { EASY, MEDIUM, HARD };
  enum class STATUS { NOT_STARTED, IN_PROGRESS, COMPLETED };
  enum class PRIORITY { LOW, MODERATE, HIGH };


  class Client {
  private:
    struct Preferences {
      bool notify;
      string theme;  // Using a simple string

      Preferences(const json& j)
        : notify(j.contains("notify") && !j["notify"].is_null() ? j["notify"].get<bool>() : false),
        theme(j.contains("theme") && !j["theme"].is_null() ? j["theme"].get<string>() : "Default") {}  // Use an empty string as the default if theme is not present
    };

    struct UserProfile {
      string username;
      string last_logged_in; // Always set to the current date and time
      Preferences preferences;

      UserProfile(const json& j)
        : username(j.contains("username") && !j["username"].is_null() ? j["username"].get<string>() : "DefaultUser"), // Prompt the user for their username
        last_logged_in(getCurrentDateTime()), // Set to current datetime
        preferences(j.contains("preferences") && !j["preferences"].is_null() ? j["preferences"] : json{}) {}
      // Directly pass the JSON object

      string getCurrentDateTime() {
        time_t now = time(nullptr);  // Get the current time as time_t
        struct tm timeinfo;  // Create a tm struct to hold local time

        // Use localtime_s for safe conversion from time_t to tm.
        // localtime_s returns zero on success.
        // localtime is deprecated in C++14 and later, so it's better to use localtime_s.
        if (localtime_s(&timeinfo, &now) != 0) {
          throw runtime_error("Failed to convert time to local time.");
        }

        stringstream ss;
        ss << put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");  // Format time to a readable string
        return ss.str();
      }
    };

    UserProfile userProfile;


  public:

    // Constructor checks if "user_profile" key exists and handles it
    // The constructor initializes the userProfile struct using the JSON object properly
    Client(const json& j)
      : userProfile(j.contains("user_profile") ? j["user_profile"] : json{}) {}

    json toJSON() const {
      return json{
          {"username", userProfile.username},
          {"last_logged_in", userProfile.last_logged_in},
          {"preferences", {{"notify", userProfile.preferences.notify}, {"theme", userProfile.preferences.theme}}}
      };
    }

    // NOT IMPLEMENTED
    /*
    void modifyProfile() {
      cout << "Modifying Profile..." << endl;
      cout << "Current UserName is: " << getUserName() << "\nWould you like to change it? If yes, enter (y) otherwise enter any other character.";
      char usrNm_input;
      cin.get(usrNm_input);
      if (usrNm_input == 'y')
      {
        getline(cin, userProfile.username);
      }

      cout << "Current theme is: " << getTheme() << "\nWould you like to toggle it? If yes, enter (y) otherwise enter any other character.";
      char pref_input;
      cin.get(pref_input);

      if (pref_input == 'y')
      {
        toggleNotify();
      }
    }
    */

    // Print out the client's profile information
    string printProfile() const {
      string result;
      result += "Username: " + userProfile.username + "\n";
      result += "Last Logged In: " + (!userProfile.last_logged_in.empty() ? userProfile.last_logged_in : "Never") + "\n";
      result += "Notifications: " + string(userProfile.preferences.notify ? "Enabled" : "Disabled") + "\n";
      result += "Theme: " + (!userProfile.preferences.theme.empty() ? userProfile.preferences.theme : "Default") + "\n";
      return result;
    }

    // NOT IMPLEMENTED
    /*
    void setUsername() {
      string result;
      getline(cin, userProfile.username);
      if (userProfile.username.empty()) {
        //cout << "Username cannot be empty. Please try again." << endl;
        setUsername();  // Recursive call if input is empty
      }
    }
    */

    /*
    void setNotify()
    {
      // needs to work with wxWidgets
      //cout << "Please enter your notification preference (true/false): ";
      string notify;
      getline(cin, notify);
      if (notify != "true" && notify != "false")
      {
        cout << "Invalid input. Please enter 'true' or 'false'." << endl;
        setNotify();  // Recursive call if input is invalid
      }
      userProfile.preferences.notify = notify == "true";
    }
    */

    /*
    void setTheme()
    {
      // needs to work with wxWidgets
      cout << "Please enter your theme (dark/light): ";
      getline(cin, userProfile.preferences.theme);
      if (userProfile.preferences.theme != "dark" && userProfile.preferences.theme != "light")
      {
        cout << "Invalid theme. Please enter 'dark' or 'light'." << endl;
        setTheme();  // Recursive call if input is invalid
      }
    }
    */

    
    string getUserName() const
    {
      return userProfile.username;
    }
    //NOT IMPLEMENTED
    /*
    string getLastLoggedIn() const
    {
      return userProfile.last_logged_in;
    }

    string getNotify() const
    {
      return userProfile.preferences.notify ? "Enabled" : "Disabled";
    }

    string getTheme() const
    {
      return userProfile.preferences.theme.empty() ? "Default" : userProfile.preferences.theme;
    }
    */
    // before calling, tell user to enter a username with wxWidgets
    // needs to work with wxWidgets
    void setUsername(const string& newUserName) {
      if (newUserName.empty())
        userProfile.username = "DefaultUser";
      else
        userProfile.username = newUserName;
      userProfile.last_logged_in = userProfile.getCurrentDateTime();
    }

    // NOT IMPLEMENTED
    /*
    void toggleNotify()
    {
      userProfile.preferences.notify = !userProfile.preferences.notify;
    }

    void toggleTheme()
    {
      if (userProfile.preferences.theme == "dark")
      {
        userProfile.preferences.theme = "light";
      }
      else
      {
        userProfile.preferences.theme = "dark";
      }
    }
    
    // Friend declaration for the insertion operator
    friend ostream& operator<<(ostream& os, const Client& client);
    */
  };

  // NOT IMPLEMENTED
  /*
  ostream& operator<<(ostream& os, const Client& client) {
    os << client.printProfile();  // Assuming printProfile() returns the formatted string
    return os;
  }
  */

  //*********************************************************************************

  // Definition of the Chore class
  class Chore {
  private:
    // Member variables to store chore details
    int id;  // Unique identifier for the chore
    int earnings;  // Monetary reward for completing the chore

    // Descriptive attributes of the chore
    string name;  // Name of the chore
    string description;  // Description of what the chore entails
    string frequency;  // How often the chore needs to be done
    string estimated_time;  // Estimated time to complete the chore
    string notes;  // Additional notes about the chore
    string location;  // Location where the chore needs to be done

    // Lists that describe when and how the chore is to be done
    vector<string> days;  // Days of the week the chore is active
    vector<string> tools_required;  // Tools required to complete the chore
    vector<string> materials_needed;  // Materials needed to complete the chore
    vector<string> tags;  // Tags for categorizing or searching for the chore

    // Enumerated types for managing chore metadata
    DIFFICULTY Difficulty;  // Difficulty level of the chore
    STATUS Status;  // Current status of the chore (e.g., not started, in progress)
    PRIORITY Priority;  // Priority level of the chore

  public:
    // Constructor that initializes a Chore object from a JSON object
    Chore(const json& j) {
      // Parsing JSON to initialize member variables
      id = j["id"].is_null() ? -1 : j["id"].get<int>();
      name = j["name"].is_null() ? "" : j["name"].get<string>();
      description = j["description"].is_null() ? "" : j["description"].get<string>();
      frequency = j["frequency"].is_null() ? "" : j["frequency"].get<string>();
      estimated_time = j["estimated_time"].is_null() ? "" : j["estimated_time"].get<string>();
      earnings = j["earnings"].is_null() ? 0 : j["earnings"].get<int>();

      // Initializing vectors from JSON arrays, defaulting to empty if not provided
      days = j["days"].is_null() ? vector<string>() : j["days"].get<vector<string>>();
      location = j["location"].is_null() ? "" : j["location"].get<string>();
      tools_required = j["tools_required"].is_null() ? vector<string>() : j["tools_required"].get<vector<string>>();
      materials_needed = j["materials_needed"].is_null() ? vector<string>() : j["materials_needed"].get<vector<string>>();
      notes = j["notes"].is_null() ? "" : j["notes"].get<string>();
      tags = j["tags"].is_null() ? vector<string>() : j["tags"].get<vector<string>>();

      // Initialize enum values using helper functions
      Difficulty = parseDifficulty(j);
      Priority = parsePriority(j);
      Status = parseStatus(j);
    }

    // Default constructor
    Chore() = default;

    // Virtual destructor
    virtual ~Chore() {}

    // NOT IMPLEMENTED
    /*
    // Method to start a chore, modifying its status based on current state
    virtual void startChore() {
      if (Status == STATUS::NOT_STARTED) {
        cout << "In Progress: " << name << endl;
        Status = STATUS::IN_PROGRESS;
      }
      else if (Status == STATUS::IN_PROGRESS) {
        cout << "Chore already started " << name << endl;
      }
      else if (Status == STATUS::COMPLETED) {
        cout << "In Progress: " << name << endl;
        Status = STATUS::IN_PROGRESS;
      }
    }

    // Method to mark a chore as completed, modifying its status based on current state
    virtual void completeChore() {
      if (Status == STATUS::NOT_STARTED || Status == STATUS::IN_PROGRESS) {
        cout << "Completed: " << name << endl;
        Status = STATUS::COMPLETED;
      }
      else if (Status == STATUS::COMPLETED) {
        cout << "Chore already completed: " << name << endl;
      }
    }

    // Method to reset a chore to not started, regardless of its current state
    virtual void resetChore() {
      if (Status == STATUS::NOT_STARTED) {
        cout << "Chore not started: " << name << endl;
      }
      else if (Status == STATUS::IN_PROGRESS || Status == STATUS::COMPLETED) {
        cout << "Resetting: " << name << endl;
        Status = STATUS::NOT_STARTED;
      }
    }
    */

    // Serialize the chore object to JSON
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
          {"difficulty", toStringD(Difficulty)},
          {"priority", toStringP(Priority)},
          {"status", toStringS(Status)}
      };
    }

    // Method to return a formatted string containing all chore attributes for display
    // Uses helper functions to format enum values and vectors
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
        "Difficulty: " + toStringD(Difficulty);

      return result;
    }

    // Method to modify chore name and description interactively
    // NOT IMPLEMENTED
    /*
    virtual void modifyChore() {
      cout << "Modifying Chore..." << endl;
      cout << "Enter new chore name: ";
      getline(cin, name);
      cout << "Enter new chore description: ";
      getline(cin, description);
    }
    */

    // Method to print a simple representation of the chore
    string simplePrint() const {
      string result = "Chore ID: " + to_string(id) + "\n"
        + "Name: " + name + "\n"  // Convert wxString to string
        + "Description: " + description + "\n"  // Convert wxString to string
        + "Earnings: " + to_string(earnings) + "\n";

      return result;
    }


    // Friend declaration for operator<<
    friend ostream& operator<<(ostream& os, const Chore& chore);

    // Comparison operator for checking equality between two chores
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
        this->Difficulty == other.Difficulty &&
        this->Status == other.Status &&
        this->Priority == other.Priority);
    }

    // New overloaded inequality operator
    bool operator!=(const Chore& other) const {
      return !(*this == other);
    }

    //ONLY FEW GETTERS AND SETTERS ARE IMPLEMENTED IN THIS VERSION
    // getter and setter methods incorporating triggerUpdate where needed
    int getId() const {
      return id;
    }

    void setId(int newId) {
      id = newId;
    }

    string getName() const {
      return name;
    }

    void setName(const string& newName) {
      name = newName;
    }

    string getDescription() const {
      return description;
    }

    void setDescription(const string& newDescription) {
      description = newDescription;
      
    }

    string getFrequency() const {
      return frequency;
    }

    void setFrequency(const string& newFrequency) {
      frequency = newFrequency;
      
    }

    string getEstimatedTime() const {
      return estimated_time;
    }

    void setEstimatedTime(const string& newTime) {
      estimated_time = newTime;
      
    }

    int getEarnings() const {
      return earnings;
    }

    void setEarnings(int newEarnings) {
      earnings = newEarnings;
      
    }

    vector<string> getDays() const {
      return days;
    }

    void setDays(const vector<string>& newDays) {
      days = newDays;
    }

    string getLocation() const {
      return location;
    }

    void setLocation(const string& newLocation) {
      location = newLocation;
      
    }

    vector<string> getToolsRequired() const {
      return tools_required;
    }

    void setToolsRequired(const vector<string>& newTools) {
      tools_required = newTools;
      
    }

    vector<string> getMaterialsNeeded() const {
      return materials_needed;
    }

    void setMaterialsNeeded(const vector<string>& newMaterials) {
      materials_needed = newMaterials;
      
    }

    string getNotes() const {
      return notes;
    }

    void setNotes(const string& newNotes) {
      notes = newNotes;
      
    }

    vector<string> getTags() const {
      return tags;
    }

    void setTags(const vector<string>& newTags) {
      tags = newTags;
      
    }

    DIFFICULTY getDifficulty() const {
      return Difficulty;
    }

    void setDifficulty(DIFFICULTY newDifficulty) {
      Difficulty = newDifficulty;
    }

    STATUS getStatus() const {
      return Status;
    }

    void setStatus(STATUS newStatus)
    {
      Status = newStatus;
    }

    PRIORITY getPriority() const
    {
      return Priority;
    }

    void setPriority(PRIORITY newPriority)
    {
      Priority = newPriority;
    }

    // Helper method to format a vector of strings for display
    static string formatVector(const vector<string>& vec) {
      string result;
      for (const auto& item : vec) {
        if (!result.empty()) result += ", ";
        result += item;
      }
      return result.empty() ? "None" : result;
    }

    // Helper methods to parse enumeration values from JSON
    DIFFICULTY parseDifficulty(const json& j) {
      if (!j.contains("difficulty") || j["difficulty"].is_null()) {
        return DIFFICULTY::EASY;  // Default to EASY if not specified
      }
      string dif = j["difficulty"].get<string>();
      if (dif == "easy") return DIFFICULTY::EASY;
      if (dif == "medium") return DIFFICULTY::MEDIUM;
      if (dif == "hard") return DIFFICULTY::HARD;
      return DIFFICULTY::EASY;  // Default case
    }

    PRIORITY parsePriority(const json& j) {
      if (!j.contains("priority") || j["priority"].is_null()) {
        return PRIORITY::LOW;  // Default to LOW if not specified
      }
      string pri = j["priority"].get<string>();
      if (pri == "low") return PRIORITY::LOW;
      if (pri == "moderate") return PRIORITY::MODERATE;
      if (pri == "high") return PRIORITY::HIGH;
      return PRIORITY::LOW;  // Default case
    }

    STATUS parseStatus(const json& j) {
      if (!j.contains("status") || j["status"].is_null()) {
        return STATUS::NOT_STARTED;  // Default to NOT_STARTED if not specified
      }
      string stat = j["status"].get<string>();
      if (stat == "not started") return STATUS::NOT_STARTED;
      if (stat == "in progress") return STATUS::IN_PROGRESS;
      if (stat == "completed") return STATUS::COMPLETED;
      return STATUS::NOT_STARTED;  // Default case
    }

    // Methods to convert ENUM values to strings for output
    string toStringD(DIFFICULTY d) const {
      switch (d) {
      case DIFFICULTY::EASY: return "easy";
      case DIFFICULTY::MEDIUM: return "medium";
      case DIFFICULTY::HARD: return "hard";
      default: return "easy";  // Default to "easy" if unknown
      }
    }

    string toStringS(STATUS s) const {
      switch (s) {
      case STATUS::NOT_STARTED: return "not started";
      case STATUS::IN_PROGRESS: return "in progress";
      case STATUS::COMPLETED: return "completed";
      default: return "not started";  // Default to "not started" if unknown
      }
    }

    string toStringP(PRIORITY p) const {
      switch (p) {
      case PRIORITY::LOW: return "low";
      case PRIORITY::MODERATE: return "moderate";
      case PRIORITY::HIGH: return "high";
      default: return "low";  // Default to "low" if unknown
      }
    }
  };

  // Implementation of the operator<< for Chore class
  ostream& operator<<(ostream& os, const Chore& chore) {
    os << chore.PrettyPrintClassAttributes();
    return os;
  }

  // Comparison structures for sorting chores based on specific attributes
  struct CompareEarnings {
    bool operator()(const shared_ptr<Chore>& a, const shared_ptr<Chore>& b) const {
      return a->getEarnings() < b->getEarnings();
    }
  };

  struct CompareDifficulty {
    bool operator()(const shared_ptr<Chore>& a, const shared_ptr<Chore>& b) const {
      return a->getDifficulty() < b->getDifficulty(); // Ensure Difficulty can be compared
    }
  };

  struct CompareID {
    bool operator()(const shared_ptr<Chore>& a, const shared_ptr<Chore>& b) const {
      return a->getId() < b->getId();
    }
  };

  struct CompareName
  {
    bool operator()(const shared_ptr<Chore>& a, const shared_ptr<Chore>& b) const
    {
      return a->getName() < b->getName();
    }
  };

  // Search criteria functions
  bool matchById(const shared_ptr<Chore>& chore, const int& id) {
    return chore->getId() == id;
  }

  bool matchByName(const shared_ptr<Chore>& chore, const string& name) {
    return chore->getName() == name;
  }

  bool matchByEarnings(const shared_ptr<Chore>& chore, const int& earnings) {
    return chore->getEarnings() == earnings;
  }
  //*********************************************************************

  /*Class definition of an easy chore, derived from Chore*/
  class EasyChore : public Chore {
  private:
    string multitasking_tips;  // Tips for multitasking while performing the chore

  public:
    // Constructor that initializes an EasyChore from a JSON object
    EasyChore(const json& j) : Chore(j) {
      multitasking_tips = j["multitasking_tips"];  // Extract multitasking tips from JSON
    }

    /*
    // Override the startChore method to include multitasking tips
    void startChore() override {
      cout << "Starting easy chore: " << endl;
      Chore::startChore();  // Call base class implementation
      cout << "Multitasking Tips: " << multitasking_tips << endl;  // Output multitasking tips
    }

    // Override the completeChore method to add behavior for easy chores
    void completeChore() override {
      cout << "Completing easy chore: " << endl;
      Chore::completeChore();  // Call base class implementation
      cout << "Earnings from chore: " << getEarnings() << endl;  // Output earnings from the chore
    }

    // Override the resetChore method to add behavior specific to easy chores
    void resetChore() override {
      cout << "Resetting easy chore: " << endl;
      Chore::resetChore();  // Call base class implementation
    }
    */

    // Provide a string representation of the class's attributes
    string PrettyPrintClassAttributes() const override {
      return Chore::PrettyPrintClassAttributes() +
        "\nMultitasking Tips: " + multitasking_tips;  // Append multitasking tips to output
    }

    // Equality comparison to check if two EasyChore objects are the same
    bool operator==(const EasyChore& other) const {
      if (Chore::operator==(other)) {  // First compare base class attributes
        return multitasking_tips == other.multitasking_tips;  // Then compare multitasking tips
      }
      return false;
    }

    // Convert the object's data to JSON, adding specific attributes
    json toJSON() const override {
      json j = Chore::toJSON();  // Start with base class JSON
      j["multitasking_tips"] = multitasking_tips;  // Add multitasking tips
      return j;
    }

    string getMultitaskingTips() {
      return multitasking_tips;
    }

    void setMultitaskingTips(const string& newTip) {
      multitasking_tips = newTip;
    }
  };

  //************************************************************************************************

  /*Class definition of a medium chore, derived from Chore*/
  class MediumChore : public Chore {
  private:
    vector<string> variations;  // List of variations of the medium chore

  public:
    // Constructor that initializes a MediumChore from a JSON object
    MediumChore(const json& j) : Chore(j) {
      variations = j["variations"].is_null() ? vector<string>() : j["variations"].get<vector<string>>();  // Parse variations from JSON
    }

    /*
    // Override the startChore method to display available variations
    void startChore() override {
      cout << "Starting medium chore: " << endl;
      Chore::startChore();  // Call base class implementation
      cout << "Variations available: ";
      for (const auto& variation : variations) {
        cout << variation << (variation != variations.back() ? ", " : "");  // Output each variation
      }
      cout << endl;
    }

    // Override the completeChore method to add behavior for medium chores
    void completeChore() override {
      cout << "Completing medium chore: " << endl;
      Chore::completeChore();  // Call base class implementation
      cout << "Earnings from chore: " << getEarnings() << endl;  // Output earnings from the chore
    }

    // Override the resetChore method for medium chores
    void resetChore() override {
      cout << "Resetting medium chore: " << endl;
      Chore::resetChore();  // Call base class implementation
    }
    */

    // Provide a string representation of the class's attributes including variations
    string PrettyPrintClassAttributes() const override {
      return Chore::PrettyPrintClassAttributes() +
        "\nVariations: " + formatVector(variations);  // Append variations to output
    }

    // Equality comparison to check if two MediumChore objects are the same
    bool operator==(const MediumChore& other) const {
      if (Chore::operator==(other)) {  // First compare base class attributes
        return variations == other.variations;  // Then compare variations
      }
      return false;
    }

    // Convert the object's data to JSON, including variations
    json toJSON() const override {
      json j = Chore::toJSON();  // Start with base class JSON
      j["variations"] = variations;  // Add variations
      return j;
    }

    vector<string> getVariations() {
      return variations;
    }

    void setVariations(const vector<string> &newVariation) {
      variations = newVariation;
    }
  };

  //************************************************************************************************

  /*Class definition of a hard chore, derived from Chore*/
  class HardChore : public Chore {
  private:
    // Structure to represent subtasks within a hard chore
    struct Subtask {
      string name;  // Name of the subtask
      string estimated_time;  // Estimated time to complete the subtask
      int earnings;  // Potential earnings from completing the subtask

      // Constructor that initializes a Subtask from a JSON object
      Subtask(const json& subtaskJson) :
        name(subtaskJson["name"].is_null() ? "" : subtaskJson["name"].get<string>()),
        estimated_time(subtaskJson["estimated_time"].is_null() ? "" : subtaskJson["estimated_time"].get<string>()),
        earnings(subtaskJson["earnings"].is_null() ? 0 : subtaskJson["earnings"].get<int>()) {}

      // Equality comparison operator for Subtask
      bool operator==(const Subtask& other) const {
        return name == other.name &&
          estimated_time == other.estimated_time &&
          earnings == other.earnings;
      }
    };

    vector<Subtask> subtasks;  // List of subtasks for the hard chore

  public:
    // Constructor that initializes a HardChore from a JSON object
    HardChore(const json& j) : Chore(j) {
      if (!j["subtasks"].is_null() && j["subtasks"].is_array()) {
        for (const auto& subtaskJson : j["subtasks"]) {
          subtasks.push_back(Subtask(subtaskJson));  // Initialize subtasks from JSON array
        }
      }
    }

    /*
    // Override the startChore method to display subtask details
    void startChore() override {
      cout << "Starting hard chore: " << endl;
      Chore::startChore();  // Call base class implementation
      for (const auto& subtask : subtasks) {
        cout << "  Subtask: " << subtask.name << ", Time: " << subtask.estimated_time << ", Earnings: $" << to_string(subtask.earnings) << endl;
      }
    }

    // Override the completeChore method to add behavior for hard chores
    void completeChore() override {
      cout << "Completing hard chore: " << endl;
      cout << "Earnings from chore: " << getEarnings() << endl;  // Output earnings from the chore
    }

    // Override the resetChore method for hard chores
    void resetChore() override {
      cout << "Resetting hard chore: " << endl;
      Chore::resetChore();  // Call base class implementation
    }
    */

    // Provide a string representation of the class's attributes including subtasks
    string PrettyPrintClassAttributes() const override {
      string result = Chore::PrettyPrintClassAttributes();  // Start with base class attributes
      for (const auto& subtask : subtasks) {
        result += "\nSubtask: " + subtask.name +
          ", Time: " + subtask.estimated_time +
          ", Earnings: $" + to_string(subtask.earnings);  // Append each subtask details
      }
      return result;  // Return the formatted string
    }

    // Equality comparison to check if two HardChore objects are the same
    bool operator==(const HardChore& other) const {
      if (!Chore::operator==(other)) {  // First compare base class attributes
        return false;
      }

      if (this->subtasks.size() != other.subtasks.size()) {  // Ensure both have the same number of subtasks
        return false;
      }

      for (size_t i = 0; i < this->subtasks.size(); ++i) {
        if (!(this->subtasks[i] == other.subtasks[i])) {  // Compare each subtask individually
          return false;
        }
      }

      return true;  // Return true if all checks pass
    }

    // Convert the object's data to JSON, including subtasks
    json toJSON() const override {
      json j = Chore::toJSON();  // Start with base class JSON
      json subtasksJson = json::array();
      for (const auto& subtask : subtasks) {
        json stJson;
        stJson["name"] = subtask.name;
        stJson["estimated_time"] = subtask.estimated_time;
        stJson["earnings"] = subtask.earnings;
        subtasksJson.push_back(stJson);  // Add each subtask to the JSON array
      }
      j["subtasks"] = subtasksJson;  // Add subtasks array to the chore JSON
      return j;
    }

    // Getter function to retrieve the vector of subtasks
    const vector<Subtask>& getSubtasks() const {
      return subtasks;
    }

    // Setter function to set the vector of subtasks
    void setSubtasks(const vector<Subtask>& newSubtasks) {
      subtasks = newSubtasks;
    }
  };

  //*********************************************************************************

  /*Templated Class definition of a Container of shared pointers (Example: vector<shared_ptr<Chore>> chores)*/
  template<typename T>
  class Container {
  private:
    vector<shared_ptr<T>> items;  // A vector to store shared pointers to objects of type T



  public:
    Container() {}  // Correct way, automatically initializes an empty vector of shared_ptr<T>

    // Generic bubble sort using comparator
    template<typename Comparator>
    void sortItems(Comparator comp, bool ascending = true) {
      try {
        bool swapped;  // Flag to keep track of whether any items were swapped during a pass
        do {
          swapped = false;  // Reset swapped flag for each pass
          for (size_t i = 1; i < items.size(); i++) {  // Loop through items
            // Perform comparison and possibly swap items
            if ((ascending && comp(items[i - 1], items[i])) || (!ascending && comp(items[i], items[i - 1]))) {
              swap(items[i - 1], items[i]);  // Swap the items if needed
              swapped = true;  // Set swapped flag if a swap has occurred
            }
          }
        } while (swapped);  // Continue sorting until no swaps are made in a pass
      }
      catch (const exception& e) {
        cerr << "Exception thrown in sortItems: " << e.what() << endl;  // Output any exceptions to standard error
      }
    }

    // Templated search method
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

    // Function to display search results
    void displaySearchResults(const vector<shared_ptr<T>>& results) {
      if (results.empty()) {
        cerr << "Results are empty!";
      }
      else {
        string info;
        for (const auto& item : results) {
          info += item->simplePrint() + "\n";
        }
        cout << info;
      }
    }

    // Move an item from this container to another container based on its ID
    void moveItemToAnotherContainer(int id, Container<T>& destination) {
      bool found = false;  // Flag to check if item was found
      for (auto it = items.begin(); it != items.end(); ++it) {  // Iterate through the items
        if ((*it)->getId() == id) {  // Check if the current item's ID matches the specified ID
          destination.items.push_back(*it);  // Add item to destination container
          cout << "Chore moved successfully: " << (*it)->getName() << endl;
          items.erase(it);  // Remove item from the current container
          found = true;  // Set found flag
          break;  // Exit loop after moving the item
        }
      }

      if (!found) {
        cout << "Chore not found." << endl;  // Inform if the item wasn't found
      }
    }

    // Delete an item from the container based on its ID
    void deleteItem(int id) {
      for (auto it = items.begin(); it != items.end(); ++it) {  // Iterate through items
        if ((*it)->getId() == id) {  // Check if current item's ID matches the specified ID
          items.erase(it);  // Remove item from vector
          break;  // Exit loop after removing item
        }
      }
    }

    // Add an item to the container
    void push_back(const shared_ptr<T>& item) {
      items.push_back(item);  // Append the item to the vector
    }

    // Return the number of items in the container
    size_t size() const {
      return items.size();
    }

    // Provide access to items by index
    shared_ptr<T>& operator[](size_t index) {
      return items[index];  // Return reference to the item at the given index
    }

    // Display all items in the container
    void displayAllItems() const {
      for (const auto& item : items) {  // Loop through all items
        cout << "Chore: " << item->getName() << " (ID: " << item->getId() << ")" << endl;  // Display each item's name and ID
      }
    }

    string returnAllItems() {
      string info;
      for (const auto& item : items) {
        // Convert the integer ID to a string before concatenation
        info += "Chore: " + item->getName() + " (ID: " + std::to_string(item->getId()) + ")\n";
      }
      return info;
    }

    string displayAllItemsAllAttributes()
    {
      string info;
      for (const auto& item : items)
      {
        info += item->PrettyPrintClassAttributes() + "\n\n";
      }
      return info;
    }

    // Check if the container is empty
    bool empty() const {
      return items.empty();
    }

    // Clear all items from the container
    void clear() {
      items.clear();  // Clear the vector
    }

    // Provide access to the internal items vector
    const vector<shared_ptr<T>>& item() const {
      return items;
    }

    // Begin iterator for range-based loops
    auto begin() -> decltype(items.begin()) {
      return items.begin();
    }

    // End iterator for range-based loops
    auto end() -> decltype(items.end()) {
      return items.end();
    }

    // Const begin iterator for range-based loops over const containers
    auto begin() const -> decltype(items.begin()) const {
      return items.begin();
    }

    // Const end iterator for range-based loops over const containers
    auto end() const -> decltype(items.end()) const {
      return items.end();
    }
  };

  //*********************************************************************************

  // Definition of the ChoreDoer class, which manages a list of chores and related information
  class ChoreDoer {
  public:
    // Container to store assigned chores
    Container<Chore> assignedChores;

    // Constructor to initialize a ChoreDoer with a name
    ChoreDoer(const string& name) : name(name) {
      age = 0;  // Default age is set to 0
      choreAmount = 0;  // Initialize the count of assigned chores to 0
      totalEarnings = 0;  // Initialize total earnings from completed chores to 0
      assignedChores = Container<Chore>();  // Initialize the container for managing chores
      id = 0;
    }

    // Method to assign a chore to the doer
    void assignChore(const shared_ptr<Chore>& chore) {
      assignedChores.push_back(chore);  // Add the chore to the container
      choreAmount++;  // Increment the count of assigned chores
    }

    void removeChore(int index) {
      assignedChores.deleteItem(index);
    }

    int getId() const
    {
      return id;
    }
    // Getter method to retrieve the chore doer's name
    string getName() const {
      return name;
    }

    void setName (const string &newName)
    {
      name = newName;
    }

    // Getter method to retrieve the total earnings accumulated from completed chores
    int getTotalEarnings() const {
      return totalEarnings;
    }

    // Getter method to retrieve the number of chores assigned to the doer
    int getChoreAmount() const {
      return choreAmount;
    }

    // Used when all chores are cleared from ChoreDoers assigned chores
    // NOT IMPLEMENTED
    /*
    void resetChoreAmount()
    {
      choreAmount = 0;
    }

    // Used when a chore is removed from ChoreDoers assigned chores
    void decreaseChoreAmount()
    {
      if (choreAmount > 0)
      {
        choreAmount--;
      }
    }
    

    // Method to generate a formatted string displaying chore doer's details
    string printChoreDoer() const {
      string result = "Chore Doer: " + name + "\n";  // Start with the chore doer's name
      result += "Total Earnings: $" + to_string(totalEarnings) + "\n";  // Add total earnings
      result += "Chore Amount: " + to_string(choreAmount) + "\n";  // Add number of chores assigned
      return result;
    }

    // Sort assigned chores by earnings
    void sortAssignedChoresByEarnings(bool ascending = true) {
      if (!assignedChores.empty())
      {
        assignedChores.sortItems(CompareEarnings(), ascending);
      }
    }

    // Sort assigned chores by difficulty
    void sortAssignedChoresByDifficulty(bool ascending = true) {
      if (!assignedChores.empty())
      {
        assignedChores.sortItems(CompareDifficulty(), ascending);
      }
    }

    // Sort assigned chores by ID
    void sortAssignedChoresByID(bool ascending = true) {
      if (!assignedChores.empty())
      {
        assignedChores.sortItems(CompareID(), ascending);
      }
    }

    // Method to start a chore based on its ID
    void startChore(int choreId) {
      for (auto& chore : assignedChores) {
        if (chore->getId() == choreId) {
          chore->startChore();  // Call startChore on the matching chore
          break;
        }
      }
    }

    // Method to complete a chore based on its ID
    void completeChore(int choreId) {
      for (auto& chore : assignedChores) {
        if (chore->getId() == choreId) {
          if (chore->getStatus() == STATUS::IN_PROGRESS || chore->getStatus() == STATUS::NOT_STARTED) {
            chore->completeChore();  // Complete the chore
            totalEarnings += chore->getEarnings();  // Update total earnings
            cout << "Chore " << chore->getName() << " completed. Total earnings now: $" << totalEarnings << endl;
            break;
          }
          cout << "Chore " << chore->getName() << " is already completed. No action taken." << endl;
        }
      }
    }

    // Method to reset a chore based on its ID
    void resetChore(int choreId) {
      for (auto& chore : assignedChores) {
        if (chore->getId() == choreId) {
          if (chore->getStatus() == STATUS::COMPLETED || chore->getStatus() == STATUS::IN_PROGRESS) {
            cout << "Resetting Chore: " << chore->getName() << endl;
            chore->resetChore();
          }
          else {
            cout << "Chore is already in the initial state (Not Started)." << endl;
          }
          return;
        }
      }
      cout << "Chore ID " << choreId << " not found among assigned chores." << endl;
    }
    */

    string returnAssignedChores()
    {
      return assignedChores.returnAllItems();
    }

    // Overloaded insertion operator to output the details of the ChoreDoer
    friend ostream& operator<<(ostream& os, const ChoreDoer& chDoer);

  private:
    string name;  // Name of the chore doer
    int choreAmount;  // Number of chores assigned to the doer
    int age;  // Age of the chore doer, not implemented in full
    int totalEarnings;  // Total earnings from chores completed by the doer
    int id;
  };

  /* NOT IMPLEMENTED
  // Overloaded ostream operator to facilitate easy output of ChoreDoer's state
  ostream& operator<<(ostream& os, const ChoreDoer& chDoer) {
    os << chDoer.printChoreDoer();  // Output formatted chore doer details
    return os;
  }
  */
  //*********************************************************************************
  class ChoreManager {
  private:
    json j; // JSON object to store data
    int choreCount; // Counter for the number of chores
    Container<Chore> Chores; // Container to hold chores
    Container<ChoreDoer> ChoreDoers; // List of chore doers
    //Container<Chore> LeftoverChores; // Container for leftover chores
    Client* client; // Client object, part of the ChoreManager
    string dynamicFile; // Filename for dynamic operations

    // Clear all data from ChoreManager
    void clearAll() {
      try {
        // Clear original Container of chores
        clearChores();
        // Clear leftover Container of chores
        //clearLeftoverChores();
        // Clear assigned Container of chores for each chore doer
        clearAllAssignedChores();
        // Clear chore doers
        clearAllChoreDoers();
      }
      catch (const exception& e) {
        cerr << "Exception caught in clearChores: " << e.what() << endl;
      }
    }

    /*
    // Clear leftover chores from the container
    void clearLeftoverChores() {
      try {
        if (!LeftoverChores.empty()) {
          LeftoverChores.clear();
        }
      }
      catch (const exception& e) {
        cerr << "Exception caught in clearLeftoverChores: " << e.what() << endl;
      }
    }
    */

    // Clear all chores from the container
    void clearChores() {
      try {
        if (!Chores.empty()) {
          Chores.clear();
        }
      }
      catch (const exception& e) {
        cerr << "Exception caught in DeleteChores: " << e.what() << endl;
      }
    }

    // Clear all chore doers from the list
    void clearChoreDoers() {
      try {
        if (!ChoreDoers.empty())
          ChoreDoers.clear();
      }
      catch (const exception& e) {
        cerr << "Exception caught in clearChoreDoers: " << e.what() << endl;
      }
    }

    // Clear all assigned chores for each chore doer
    void clearAllAssignedChores() {
      try {
        if (!ChoreDoers.empty()) {
          // Clear assigned Container of chores for each chore doer
          for (auto& doer : ChoreDoers) {
            if (!doer->assignedChores.empty()) {
              doer->assignedChores.clear();
            }
          }
        }
      }
      catch (const exception& e)
      {
        cerr << "Exception caught in clearAllAssignedChores: " << e.what() << endl;
      }
    }

    // Clear all chore doers from the list
    void clearAllChoreDoers() {
      // Clear chore doers
      if (!ChoreDoers.empty()) {
        ChoreDoers.clear();
      }
    }
  public:
    // Constructor for ChoreManager that initializes its internal state with file data
    ChoreManager(string fileName) {
      try {
        dynamicFile = fileName; // Save the filename for later use

        ifstream file(fileName); // Open the specified file
        if (file.is_open()) {
          // Parse the contents of the file into a JSON object if not empty
          if (j.is_null()) {
            j = json::parse(file);
          }
          file.close(); // Always close the file after opening
        }
        else {
          cerr << "Failed to open file: " << fileName << endl; // Error message if file cannot be opened
        }
        client = new Client(j);
        // Initialize client with the user profile from the JSON data
        if (j.contains("user_profile")) {
          if (client->getUserName() == "DefaultUser") {
            cout << "Please enter a User Name\n";
            string usrName;
            getline(cin, usrName);
            client->setUsername(usrName);
          }
        }
        choreCount = 0; // Initialize the chore count
        loadChores(); // Load chores from the JSON object
      }
      catch (const json::exception& e) {
        cerr << "JSON Error: " << e.what() << endl; // Handle JSON parsing errors
        throw; // Rethrow the exception for external handling
      }
      catch (const ifstream::failure& e) {
        cerr << "File Error: " << e.what() << endl; // Handle file opening errors
        throw;
      }
    }

    // Destructor to clear resources
    ~ChoreManager() {
      clearAll();
    }

    // Method to load chores from the JSON object into the container
    void loadChores() {
      try {
        if (j.contains("chores") && j["chores"].is_array()) {
          for (auto& choreJson : j["chores"]) {
            string difficulty = choreJson.value("difficulty", "unknown"); // Get the difficulty level from the JSON
            shared_ptr<Chore> chore;
            addChore(choreJson);
            /*
            // Create a specific type of Chore based on the difficulty level
            if (difficulty == "easy") {
              chore = make_shared<EasyChore>(choreJson);
            }
            else if (difficulty == "medium") {
              chore = make_shared<MediumChore>(choreJson);
            }
            else if (difficulty == "hard") {
              chore = make_shared<HardChore>(choreJson);
            }
            else {
              cerr << "Unknown difficulty level: " << difficulty << endl;
              continue;
            }

            // Uses the overloaded push_back method to add the chore to the container
            Chores.push_back(chore); // Add chore to the container
            */
            choreCount++; // Increment the count of chores
          }
          saveData();
        }
      }
      catch (const json::parse_error& e) {
        cerr << "JSON parse error: " << e.what() << endl;
      }
      catch (const json::out_of_range& e) {
        cerr << "JSON out of range error: " << e.what() << endl;
      }
      catch (const json::type_error& e) {
        cerr << "JSON type error: " << e.what() << endl;
      }
      catch (const exception& e) {
        cerr << "Standard exception: " << e.what() << endl;
      }
    }

    // Method to modify either user profile data or chore data interactively
    // NOT IMPLEMENTED YET
    /*
    void modifyData() {
      cout << "1: Modify User Profile\n2: Modify Chores\nChoose option: \n";
      int choice;
      cin >> choice;
      cin.ignore(); // clear buffer after reading number

      switch (choice) {
      case 1:
        client->modifyProfile();
        break;
      case 2:
        cout << "Enter chore index to modify: \n";
        int index;
        cin >> index;
        cin.ignore();
        if (index >= 0 && index < Chores.size()) {
          Chores[index]->modifyChore();
        }
        else {
          cout << "Invalid index!" << endl;
        }
        break;
      default:
        cout << "Invalid choice!" << endl;
      }
      saveData(); // Save changes to file
    }
    */
    // Method to output all data including chore assignments and user profile to a file
    void outputChoreAssignmentsToFile(const string& outputPath) {
      // Check if the output path is empty
      if (outputPath.empty()) {
        cerr << "Output path is empty!" << endl;
        return;
      }

      // Check if there are any chore doers to process
      if (ChoreDoers.empty()) {
        cerr << "No Chore Doers available to output!" << endl;
        return;
      }

      // Check if there are any assigned chores to process
      bool hasAssignedChores = false;
      for (const auto& doer : ChoreDoers) {
        if (!doer->assignedChores.empty()) {
          hasAssignedChores = true;
          break;
        }
      }

      if (!hasAssignedChores) {
        cerr << "No chores assigned to any Chore Doer!" << endl;
        return;
      }

      json output;

      // Include client's user profile data
      output["user_profile"] = client->toJSON();

      // Include chore assignments for each chore doer
      output["chore_doers"] = json::array();
      for (const auto& doer : ChoreDoers) {
        // Prepare the JSON object for each chore doer with their name and chore count at the top
        json doerJson = json::object({
          {"chore_count", doer->getChoreAmount() },
          {"name", doer->getName()},
           // Include the count of assigned chores
          {"chores", json::array()}  // Initialize chores array
          });

        for (const auto& chore : doer->assignedChores) {
          doerJson["chores"].push_back(chore->toJSON());
        }
        output["chore_doers"].push_back(doerJson);
      }

      // Write to file with pretty formatting
      ofstream outFile(outputPath);
      if (!outFile.is_open()) {
        throw runtime_error("Could not open file to write chore assignments.\n");
      }
      outFile << setw(4) << output; // Pretty print with indent of 4 spaces
      outFile.close();
    }

    // Output all data to the console or to file
    json toJSON() const {
      json output;
      for (const auto& chore : Chores) {
        output["chores"].push_back(chore->toJSON());
      }
      if (client) {
        output["user_profile"] = client->toJSON();
      }
      return output;
    }

    // saveData method to overwrite the original data.json file for chores and user data
    void saveData() {
      // Reset our json object because json is getting overwritten
      j.clear();

      // All data changed through class functions modify or set(attribute) or add/create chore  will be saved here
      // Add new data to json object with updated chore list
      j = toJSON();

      // Overwrites original file
      ofstream file(dynamicFile);
      if (file) {
        file << setw(4) << j << endl;
      }
      else {
        cerr << "Error saving file " << dynamicFile << "\n";
      }
      file.close();
    }

    string displayChoreDoerList() {
      if (ChoreDoers.empty()) {
        return "No chore doers available.\n";
      }

      string info;
      for (const auto& doer : ChoreDoers) {
        info += doer->getName() + "\n";
      }
      return info;
    }

    string displayChoreList() {
      if (Chores.empty()) {
        return "No chores available.\n";
      }

      string info;
      for (const auto& chore : Chores) {
        info += chore->simplePrint() + "\n"; // Only returns few attributes instead of all attributes
      }
      return info;
    }


    // Returns assigned chores for a chosen chore doer
    std::string displayAssignedChores(const std::string& doerName) {
      auto doer = std::find_if(ChoreDoers.begin(), ChoreDoers.end(), [&doerName](shared_ptr<ChoreDoer>& d) {
        return d->getName() == doerName;
        });

      if (doer != ChoreDoers.end()) {
        std::string info = "Chore Doer: " + (*doer)->getName() + " has chores:\n";
        info += (*doer)->returnAssignedChores();  // Use Container's method
        return info;
      }

      return "Chore Doer not found.\n";
    }

    string displayAllChoresAllAttributes()
    {
      if (Chores.empty())
      {
        return "No chores available.\n";
      }
      return Chores.displayAllItemsAllAttributes();
    }

    // Returns assigned chores for all chore doers
    string displayAllChoreAssignments() {
      if (ChoreDoers.empty()) {
        return "No chore doers available.\n";
      }

      std::string info;
      bool anyChoresAssigned = false;  // To check if any chores have been assigned at all.

      for (auto& doer : ChoreDoers) {
        auto choresInfo = doer->returnAssignedChores();
        if (!choresInfo.empty()) {
          anyChoresAssigned = true;
          info += "Chore Doer: " + doer->getName() + " has chores:\n";
          info += choresInfo;
          info += "\n";  // Add a newline for separation between doers
        }
        else {
          info += "Chore Doer: " + doer->getName() + " has no assigned chores.\n\n";
        }
      }

      if (!anyChoresAssigned) {
        return "No chores have been assigned to any chore doer.\n";
      }

      return info;  // Return the complete string with all assigned chores details
    }

    // Method to delete a chore from the available chore list by ID
    void deleteChoreFromAvailable(int choreId) {
      if (Chores.empty()) {
        cout << "No chores available to delete." << endl;
        return;
      }

      if (choreId < 0) {
        cout << "Invalid chore ID provided. ID must be non-negative." << endl;
        return;
      }

      size_t sizeBefore = Chores.size(); // Store the size before deletion

      // Delegate the deletion to the Container class method
      Chores.deleteItem(choreId);
      choreCount--;
      if (Chores.size() < sizeBefore) {
        cout << "Chore with ID " << choreId << " has been successfully deleted." << endl;
      }
      else {
        cout << "No chore with ID " << choreId << " found in the available chores." << endl;
      }
    }

    /*
    // Method to delete a chore from leftover chores by ID
    void deleteLeftoverChore(int choreId) {
      LeftoverChores.deleteItem(choreId);
    }
    */

    // NOT IMPLEMENTED IN FINAL VERSION

    // Method to delete a chore from any chore doer's assigned list by ID
    /*
    void deleteChoreFromAnyDoer(int choreId) {
      if (ChoreDoers.empty())
      {
        cout << "No chore doers available to delete chores from." << endl;
        return;
      }
      for (auto& doer : ChoreDoers) {
        doer->assignedChores.deleteItem(choreId);
      }
    }
    */

    // Method to add a new chore doer to the system
    void addChoreDoer(const string& name) {
      ChoreDoers.push_back(make_shared<ChoreDoer>(name));
    }

    // Method to delete a chore doer from the system by name
    void deleteChoreDoer(const string& name) {
      try {
        int index = 0;  // Start index from 0
        bool found = false;  // Flag to check if chore doer is found

        for (const auto& doer : ChoreDoers) {  // Use range-based for loop to access elements
          if (doer->getName() == name) {
            ChoreDoers.deleteItem(index);  // Remove the chore doer at the found index
            cout << "Chore doer named " << name << " has been deleted.\n";
            found = true;
            break;  // Exit loop after removing the chore doer
          }
          index++;  // Increment index for each iteration
        }

        if (!found) {  // Check if the chore doer was found and removed
          cerr << "ChoreDoer named '" << name << "' not found.\n" << endl;
        }
      }
      catch (const exception& e) {
        cerr << "Exception thrown in removeChoreDoer: \n" << e.what() << endl;
      }
    }

    // Method to add a new chore to the system from a JSON object
    void addChore(const json& choreJson) {
      shared_ptr<Chore> chore;

      if (choreJson.is_null()) {
        cerr << "Invalid chore JSON." << endl;
        return;
      }
      if (choreJson.contains("difficulty") && choreJson["difficulty"].is_string()) {
        string difficulty = choreJson["difficulty"].get<string>();

        if (difficulty == "easy") {
          chore = make_shared<EasyChore>(choreJson);
        }
        else if (difficulty == "medium") {
          chore = make_shared<MediumChore>(choreJson);
        }
        else if (difficulty == "hard") {
          chore = make_shared<HardChore>(choreJson);
        }
      }
      Chores.push_back(chore);
    }

    // NOT IMPLEMENTED IN FINAL VERSION
    /*
    // Method to move a chore to leftover chores list by ID
    void moveChoreToLeftover(int choreId) {
      for (auto& doer : ChoreDoers) {
        doer.assignedChores.moveItemToAnotherContainer(choreId, LeftoverChores);
      }
    }

    // Method to move a chore from leftover chores to a specific chore doer
    void moveChoreFromLeftoverToDoer(const string& toDoer, int choreId) {
      for (auto& doer : ChoreDoers) {
        if (doer.getName() == toDoer) {
          LeftoverChores.moveItemToAnotherContainer(choreId, doer.assignedChores);
          break;
        }
        cout << "Chore Doer not found." << endl;
      }
    }
    */

    // Method to move a chore between two chore doers
    // NOT IMPLEMENTED IN FINAL VERSION
    /*
    void moveChoreBetweenDoers(const string& fromDoer, const string& toDoer, int choreId) {
      bool found = false;
      for (auto& doer : ChoreDoers) {
        if (doer->getName() == fromDoer) {
          for (auto& targetDoer : ChoreDoers) {
            if (targetDoer->getName() == toDoer) {
              doer->assignedChores.moveItemToAnotherContainer(choreId, targetDoer->assignedChores);
              found = true;
              break;
            }
          }
        }
      }
      if (!found) {
        cout << "Chore or Chore Doer not found.\n" << endl;
      }
    }
    */

    //NOT IMPLEMENTED IN FINAL VERSION
    /*
    // Method to manually assign a chore to a ChoreDoer
    void assignChoreDoer(int choreId, string& doerName) {
      auto chore = std::find_if(Chores.begin(), Chores.end(), [choreId](const shared_ptr<Chore>& c) {
        return c->getId() == choreId;
        });
      if (chore != Chores.end()) {
        auto doer = std::find_if(ChoreDoers.begin(), ChoreDoers.end(), [&doerName](const shared_ptr<ChoreDoer>& d) {
          return d->getName() == doerName;
          });

        if (doer != ChoreDoers.end()) {
          doer->assignChore(*chore);
        }
        else {
          std::cerr << "ChoreDoer " << doerName << " not found." << std::endl;
        }
      }
      else {
        std::cerr << "Chore ID " << choreId << " not found." << std::endl;
      }
    }
    */
    
    // Method to assign chores randomly to chore doers
    void assignChoresRandomly() {
      try {
        // Check if there are chores to assign
        if (Chores.empty()) {
          std::cout << "No chores to assign.\n" << std::endl;
          return;
        }

        // Check if there are chore doers available
        if (ChoreDoers.empty()) {
          std::cout << "No chore doers available.\n" << std::endl;
          return;
        }

        // Create a random engine; you could also use default_random_engine
        std::random_device rd;
        std::mt19937 g(rd());

        // Shuffle the chores using the random engine
        shuffle(Chores.begin(), Chores.end(), g);

        size_t choreIndex = 0;
        // Loop over each chore and try to assign it to a chore doer
        while (choreIndex < Chores.size()) {
          for (auto& doer : ChoreDoers) {
            if (choreIndex < Chores.size()) {
              doer->assignChore(Chores[choreIndex++]);
            }
            else {
              break; // Break if there are no more chores to assign
            }
          }
        }
      }
      catch (const std::exception& e) {
        std::cout << "Exception thrown in assignChoresRandomly: " << e.what() << std::endl;
      }
    }

    // Method to create a minimal chore structure with initialized attributes
    // NOT IMPLEMENTED IN FINAL VERSION
    /* 
    void createMinimalChore() {
      std::string name, difficulty, priority, multitasking_tips, input;
      json choreJson;

      // Automatically set chore ID
      static int choreId = 1;  // Static counter to automatically increment chore IDs
      choreJson["id"] = choreId++;

      std::cout << "Enter chore name: ";
      std::getline(std::cin, name);
      choreJson["name"] = name;

      // Automatically set other attributes
      choreJson["description"] = "Standard description";
      choreJson["status"] = "not started";
      choreJson["frequency"] = "weekly";
      choreJson["estimated_time"] = "1 hour";
      choreJson["earnings"] = 10;  // Default earning
      choreJson["days"] = std::vector<std::string>{ "Monday", "Wednesday" };
      choreJson["location"] = "Home";
      choreJson["tools_required"] = std::vector<std::string>{};
      choreJson["materials_needed"] = std::vector<std::string>{};
      choreJson["notes"] = "No additional notes";
      choreJson["tags"] = std::vector<std::string>{ "general" };

      // Set Difficulty
      std::cout << "Enter difficulty (easy, medium, hard): ";
      std::getline(std::cin, difficulty);
      while (difficulty != "easy" && difficulty != "medium" && difficulty != "hard") {
        std::cout << "Invalid difficulty. Please enter 'easy', 'medium', or 'hard': \n";
        std::getline(std::cin, difficulty);
      }
      choreJson["difficulty"] = difficulty;

      // Set Priority
      std::cout << "Enter priority (low, moderate, high): \n";
      std::getline(std::cin, priority);
      while (priority != "low" && priority != "moderate" && priority != "high") {
        std::cout << "Invalid priority. Please enter 'low', 'moderate', or 'high': \n";
        std::getline(std::cin, priority);
      }
      choreJson["priority"] = priority;

      // Conditional inputs based on difficulty
      if (difficulty == "easy") {
        std::cout << "Enter multitasking tips for easy chores: \n";
        std::getline(std::cin, multitasking_tips);
        choreJson["multitasking_tips"] = multitasking_tips;
      }
      else if (difficulty == "medium") {
        std::cout << "Enter variations for medium chores (comma separated): \n";
        std::getline(std::cin, input);
        choreJson["variations"] = input;  // Assuming simple string, adjust if JSON array needed
      }
      else if (difficulty == "hard") {
        std::vector<json> subtasks;
        char choice = 'y';
        while (choice == 'y') {
          json subtask;
          std::string subtaskName, subtaskTime;
          int subtaskEarnings;

          std::cout << "Enter a name for one subtask: \n";
          std::getline(std::cin, subtaskName);
          subtask["name"] = subtaskName;

          std::cout << "Enter estimated time for the subtask: \n";
          std::getline(std::cin, subtaskTime);
          subtask["estimated_time"] = subtaskTime;

          std::cout << "Enter earnings for the subtask (integer value): \n";
          std::cin >> subtaskEarnings;
          subtask["earnings"] = subtaskEarnings;

          std::cin.ignore();  // Clear newline character after single char input
          subtasks.push_back(subtask);

          std::cout << "Add another subtask? (y/n): \n";
          std::cin >> choice;
          std::cin.ignore();  // Clear newline character after single char input
        }
        choreJson["subtasks"] = subtasks;
      }

      // Add the chore to the chore list
      addChore(choreJson);
    }
    */
    // Method to create a full-fledged chore with all details
    // NOT IMPLEMENTED IN FINAL VERSION
    /*
    void createFullChore() {
      // Basic chore details
      string name, description, frequency, estimated_time, difficulty, days, location,
        tools_required, materials_needed, priority, notes, status, tags, multitasking_tips,
        earningsInput, input;
      double earnings;

      // Variables for handling unique attributes (Easy, Medium, Hard)
      json choreJson;
      char choice;
      //Id
      choreJson["id"] = choreCount++;

      //Name
      cout << "Enter chore name: \n";
      getline(cin, name);
      choreJson["name"] = name.empty() ? "" : name;

      //Description
      cout << "Enter chore description: \n";
      getline(cin, description);
      choreJson["description"] = description.empty() ? "" : description;

      //Frequency
      cout << "Enter chore frequency (e.g., daily, weekly): \n";
      getline(cin, frequency);
      choreJson["frequency"] = frequency.empty() ? "" : frequency;

      //Estimated_time
      cout << "Enter estimated time for chore completion: \n";
      getline(cin, estimated_time);
      choreJson["estimated_time"] = estimated_time.empty() ? "" : estimated_time;

      // Handle earnings input with robust validation
      // If not entered correctly -> re-enter
      while (true) {
        cout << "Enter earnings for chore (integer value): \n";
        getline(cin, earningsInput);
        try {
          earnings = stoi(earningsInput);
          choreJson["earnings"] = earnings;
          break;  // Break out of the loop if stoi succeeds
        }
        catch (const exception& e) {  // Catching all exceptions
          cout << "Invalid input for earnings. Please enter a valid integer.\n" << e.what() << endl;
        }
      }

      //Days
      cout << "Enter days when the chore should be performed (comma separated): \n";
      getline(cin, days);
      choreJson["days"] = days.empty() ? json::array() : json::parse("[" + days + "]");

      //Location
      cout << "Enter chore location: \n";
      getline(cin, location);
      choreJson["location"] = location.empty() ? json::array() : json::parse("[" + location + "]");

      //Tools_required
      cout << "Enter tools required (comma separated): \n";
      getline(cin, tools_required);
      choreJson["tools_required"] = tools_required.empty() ? json::array() : json::parse("[" + tools_required + "]");

      //Materials_needed
      cout << "Enter materials needed (comma separated): \n";
      getline(cin, materials_needed);
      choreJson["materials_needed"] = materials_needed.empty() ? json::array() : json::parse("[" + materials_needed + "]");

      //Notes
      cout << "Enter any additional notes: \n";
      getline(cin, notes);
      choreJson["notes"] = notes.empty() ? "" : notes;

      //Tags
      cout << "Enter tags for the chore (comma separated): \n";
      getline(cin, tags);
      choreJson["tags"] = tags.empty() ? json::array() : json::parse("[" + tags + "]");

      //Difficulty
      //If not entered correctly -> re-enter
      cout << "Enter difficulty (easy, medium, hard): ";
      getline(cin, difficulty);
      while (difficulty != "easy" && difficulty != "medium" && difficulty != "hard" && !difficulty.empty()) {
        cout << "Invalid difficulty. Please enter 'easy', 'medium', or 'hard': \n";
        getline(cin, difficulty);
      }
      choreJson["difficulty"] = difficulty.empty() ? "" : difficulty;

      //Priority
      //If not entered correctly -> re-enter
      cout << "Enter priority (low, moderate, high): ";
      getline(cin, priority);
      while (priority != "low" && priority != "moderate" && priority != "high" && !priority.empty()) {
        cout << "Invalid priority. Please enter 'low', 'moderate', or 'high': \n";
        getline(cin, priority);
      }
      choreJson["priority"] = priority.empty() ? "" : priority;

      //Status
      //If not entered correctly -> re-enter
      cout << "Enter status (not started, in progress, completed): \n";
      getline(cin, status);
      while (status != "not started" && status != "in progress" && status != "completed" && !status.empty()) {
        cout << "Invalid status. Please enter 'not started', 'in progress', or 'completed': \n";
        getline(cin, status);
      }
      choreJson["status"] = status.empty() ? "" : status;

      // Unique attributes based on difficulty
      // EASY
      if (difficulty == "easy") {
        cout << "Enter multitasking tips for easy chore: \n";
        getline(cin, multitasking_tips);
        choreJson["multitasking_tips"] = multitasking_tips.empty() ? "" : multitasking_tips;
      }
      // MEDIUM
      else if (difficulty == "medium") {

        cout << "Are there any variations? Enter 'y' for yes or 'n' for no: \n";
        cin >> choice;

        if (choice == 'y')
        {
          cout << "Enter variations for medium chore (comma separated): \n";
          cin.ignore();
          getline(cin, input);
          choreJson["variations"] = input.empty() ? json::array() : json::parse("[" + input + "]");
        }
      }
      // HARD
      else if (difficulty == "hard") {
        json subtask;
        string subtaskName;
        string subtaskTime;
        int subtaskEarnings;

        cout << "Are there any subtasks? Enter 'y' for yes or 'n' for no: \n";
        cin >> choice;

        if (choice == 'y')
        {
          cout << "Enter a name for one subtask: \n";
          getline(cin, subtaskName);

          cout << "Enter estimated time for the subtask: \n";
          getline(cin, subtaskTime);

          // Handle earnings input with robust validation
          while (true) {
            cout << "Enter earnings for chore (integer value) or whole dollar amount: \n";

            try {
              cin >> subtaskEarnings;
              choreJson["earnings"] = subtaskEarnings; // If not int -> catch exception -> repeat loop
              break;  // Break out of the loop if stoi succeeds
            }
            catch (const exception& e) {  // Catching all exceptions
              cout << "Invalid input for earnings. Please enter a valid integer.\n" << e.what() << endl;
            }
          }

          // Save the rest of subtask
          subtask["name"] = subtaskName.empty() ? "" : subtaskName;
          subtask["estimated_time"] = subtaskTime.empty() ? "" : subtaskTime;
          subtask["earnings"] = to_string(subtaskEarnings).empty() ? 0 : subtaskEarnings;
          choreJson["subtasks"] = json::array({ subtask });
        }
        // If n is entered, create empty subtask
        else
        {
          cout << "No subtasks added.\n" << endl;
          choreJson["subtasks"] = json::array(); // should be empty
        }
      }

      // Add the chore to the chore list and save file
      addChore(choreJson);
      cout << "Chore added successfully!\n" << endl;
    }
    */

    //Function wrapper for templated sort function
    template<typename Comparator>
    void sortChores(Comparator comp, bool ascending = true) {
      try
      {
        Chores.sortItems(comp, ascending);
      }
      catch (const exception& e)
      {
        cerr << "Exception caught in sortChores: \n" << e.what() << endl;
      }
    }

    void searchByID(int id) {
      auto resultsById = Chores.searchItem<int>(id, matchById);
      if (resultsById.empty()) {
        // Search through each ChoreDoer's assigned chores
        for (auto& doer : ChoreDoers) {
          auto doerResults = doer->assignedChores.searchItem<int>(id, matchById);
          if (!doerResults.empty()) {
            resultsById.insert(resultsById.end(), doerResults.begin(), doerResults.end());
          }
        }
      }
      if (resultsById.empty()) {
        std::cout << "No chore found with the specified ID.\n" << std::endl;
      }
      else {
        Chores.displaySearchResults(resultsById);
      }
    }

    void searchByName(const std::string& name) {
      auto resultsName = Chores.searchItem<std::string>(name, matchByName);
      if (resultsName.empty()) {
        // Search through each ChoreDoer's assigned chores
        for (auto& doer : ChoreDoers) {
          auto doerResults = doer->assignedChores.searchItem<std::string>(name, matchByName);
          if (!doerResults.empty()) {
            resultsName.insert(resultsName.end(), doerResults.begin(), doerResults.end());
          }
        }
      }
      if (resultsName.empty()) {
        std::cout << "No chore found with the specified name.\n" << std::endl;
      }
      else {
        Chores.displaySearchResults(resultsName);
      }
    }

    void searchByEarnings(int earnings) {
      auto resultsEarned = Chores.searchItem<int>(earnings, matchByEarnings);
      if (resultsEarned.empty()) {
        // Search through each ChoreDoer's assigned chores
        for (auto& doer : ChoreDoers) {
          auto doerResults = doer->assignedChores.searchItem<int>(earnings, matchByEarnings);
          if (!doerResults.empty()) {
            resultsEarned.insert(resultsEarned.end(), doerResults.begin(), doerResults.end());
          }
        }
      }
      if (resultsEarned.empty()) {
        std::cout << "No chore found with the specified earnings.\n" << std::endl;
      }
      else {
        Chores.displaySearchResults(resultsEarned);
      }
    }

    /*
    std::string searchChoreDoer(const std::string& name) {
      std::string info;
      if (ChoreDoers.empty())
      {
        std::cout << "No chore doers available.\n" << std::endl;
        return "There are no chore doers.\nAdd a chore doer in the main menu.\n";
      }
      for (auto doer : ChoreDoers) {
        if (doer->getName() == name) {
          info += doer->printChoreDoer() + "\n";
          if (!doer->assignedChores.empty())
            info += displayAssignedChores(name);
        }
        else {
          info = "Could not find specified chore doer\n";
        }
      }
      return info;
    }
    */

    // Sorts chores by earnings and prints a confirmation message.
    void sortChoresByEarnings(bool ascending = true) {
      CompareEarnings comp;  // Instantiate the comparator for earnings
      if (!Chores.empty()) {  // Check if there are chores to sort
        Chores.sortItems(comp, ascending);  // Perform the sort operation
        std::cout << "Chores sorted by earnings.\n";  // Print confirmation
      }
    }

    // Sorts chores by difficulty and prints a confirmation message.
    void sortChoresByDifficulty(bool ascending = true) {
      CompareDifficulty comp;  // Instantiate the comparator for difficulty
      if (!Chores.empty()) {  // Check if there are chores to sort
        Chores.sortItems(comp, ascending);  // Perform the sort operation
        std::cout << "Chores sorted by difficulty.\n";  // Print confirmation
      }
    }

    // Sorts chores by their unique IDs and prints a confirmation message.
    void sortChoresByID(bool ascending = true) {
      CompareID comp;  // Instantiate the comparator for ID
      if (!Chores.empty()) {  // Check if there are chores to sort
        Chores.sortItems(comp, ascending);  // Perform the sort operation
        std::cout << "Chores sorted by ID.\n";  // Print confirmation
      }
    }

    // Sorts chores by names and prints a confirmation message.
    void sortChoresByName(bool ascending = true) {
      CompareName comp;  // Instantiate the comparator for name
      if (!Chores.empty()) {  // Check if there are chores to sort
        Chores.sortItems(comp, ascending);  // Perform the sort operation
        std::cout << "Chores sorted by name.\n";  // Print confirmation
      }
    }
    // NOT IMPLEMENTED
    /*
    // Sorts all assigned chores for each chore doer by earnings and prints a confirmation message.
    void sortAllChoreDoersChoresByEarnings(bool ascending = true) {
      if (ChoreDoers.empty()) {  // Check if there are chore doers
        std::cout << "No chore doers available.\n";  // Notify if no chore doers are available
        return;
      }
      for (auto& doer : ChoreDoers) {  // Loop through each chore doer
        doer->sortAssignedChoresByEarnings(ascending);  // Sort their assigned chores by earnings
      }
      std::cout << "Sorted all chore doers' chores by earnings.\n";  // Print confirmation
    }

    // Sorts all assigned chores for each chore doer by difficulty and prints a confirmation message.
    void sortAllChoreDoersChoresByDifficulty(bool ascending = true) {
      if (ChoreDoers.empty()) {  // Check if there are chore doers
        std::cout << "No chore doers available.\n";  // Notify if no chore doers are available
        return;
      }
      for (auto& doer : ChoreDoers) {  // Loop through each chore doer
        doer->sortAssignedChoresByDifficulty(ascending);  // Sort their assigned chores by difficulty
      }
      std::cout << "Sorted all chore doers' chores by difficulty.\n";  // Print confirmation
    }

    // Sorts all assigned chores for each chore doer by ID and prints a confirmation message.
    void sortAllChoreDoersChoresByID(bool ascending = true) {
      if (ChoreDoers.empty()) {  // Check if there are chore doers
        std::cout << "No chore doers available.\n";  // Notify if no chore doers are available
        return;
      }
      for (auto& doer : ChoreDoers) {  // Loop through each chore doer
        doer->sortAssignedChoresByID(ascending);  // Sort their assigned chores by ID
      }
      std::cout << "Sorted all chore doers' chores by ID.\n";  // Print confirmation
    }
    */
    // Compares two chores based on their IDs to check if they are identical.
    void compareChores(int id1, int id2) {
      if (id1 < 0 || id2 < 0) {  // Validate the IDs to ensure they are non-negative
        cout << "Invalid chore ID provided. ID must be non-negative.\n";
        return;
      }
      auto chore1 = find_if(Chores.begin(), Chores.end(), [id1](const shared_ptr<Chore>& chore) {
        return chore->getId() == id1;  // Find the first chore by ID
        });
      auto chore2 = find_if(Chores.begin(), Chores.end(), [id2](const shared_ptr<Chore>& chore) {
        return chore->getId() == id2;  // Find the second chore by ID
        });

      if (chore1 != Chores.end() && chore2 != Chores.end()) {  // Ensure both chores are found
        if (**chore1 == **chore2) {
          cout << "The two chores are identical.\n";
        }
        else {
          cout << "The two chores are not identical.\n";
        }
      }
      else {
        cout << "One or both chore IDs not found.\n";
      }
    }

    // Provides a menu for searching chores by ID, name, or earnings.
    void searchMenu() {
      int choice, id, earnings;
      string name;
      bool searchMenuRunning = true;
      while (searchMenuRunning) {
        cout << "\nSearch Chores Menu\n";
        cout << "1. Search by ID\n";
        cout << "2. Search by Name\n";
        cout << "3. Search by Earnings\n";
        cout << "4. Back to Main Menu\n";
        cout << "Enter your choice: ";
        cin >> choice;
        cin.ignore();  // Clear the newline character after the integer input

        switch (choice) {
        case 1:
          cout << "Enter the ID of the chore to search for: ";
          cin >> id;
          cin.ignore();
          searchByID(id);
          break;
        case 2:
          cout << "Enter the name of the chore to search for: ";
          getline(cin, name);
          searchByName(name);
          break;
        case 3:
          cout << "Enter the earnings of the chores to search for: ";
          cin >> earnings;
          cin.ignore();
          searchByEarnings(earnings);
          break;
        case 4:
          searchMenuRunning = false;
          break;
        default:
          cout << "Invalid choice. Please try again.\n";
          break;
        }
      }
    }

    // Provides a menu for sorting chores by ID, name, earnings, or difficulty.
    void sortMenu() {
      int choice;
      bool sortMenuRunning = true;

      while (sortMenuRunning) {
        cout << "\nSort Chores Menu\n";
        cout << "1. Sort by ID\n";
        cout << "2. Sort by Name\n";
        cout << "3. Sort by Earnings\n";
        cout << "4. Sort by Difficulty\n";
        cout << "5. Back to Main Menu\n";
        cout << "Enter your choice: ";
        cin >> choice;
        cin.ignore();  // Clear the newline character after the integer input

        switch (choice) {
        case 1:
          cout << "Sorting chores by ID.\n";
          sortChoresByID();
          break;
        case 2:
          cout << "Sorting chores by Name.\n";
          sortChoresByName();
          break;
        case 3:
          cout << "Sorting chores by Earnings.\n";
          sortChoresByEarnings();
          break;
        case 4:
          cout << "Sorting chores by Difficulty.\n";
          sortChoresByDifficulty();
          break;
        case 5:
          sortMenuRunning = false;
          break;
        default:
          cout << "Invalid choice. Please try again.\n";
          break;
        }
      }
    }
  };
}
int main() {
  try {
    // Define the file path for the main data file
    string testFile = DATA_FILE_PATH + "data.json";

    // Define the file path for the file where assigned chores are written
    string assignedChoresFile = DATA_FILE_PATH + "assigned_chores.json";

    // Instantiate a ChoreManager object using the data file path
    ChoreApp::ChoreManager manager(testFile);

    // Flag to control the main loop
    bool running = true;
    string input; // Variable to store user input
    int choice;   // Variable to store user's menu choice
    int id;       // Variable to store chore or chore doer ID

    // Main loop to display the menu and process user input
    while (running) {
      // Display the main menu options
      cout << "\nChore Manager System\n";
      cout << "1. Add Chore Doer\n";
      cout << "2. Assign Chores Randomly\n";
      cout << "3. Show All Chores\n";
      cout << "4. Display All Assigned Chores\n";
      cout << "5. Display All Chores with all data\n";
      cout << "6. Print All Chore Doers Assigned Chores to File\n";
      cout << "7. Delete a Chore by ID\n";
      cout << "8. Delete a Chore Doer by Name\n";
      cout << "9. Display All Chore Doers\n";
      cout << "10. Search Chores\n";
      cout << "11. Compare Two Chores\n";
      cout << "12. Sort Chores\n";
      cout << "15. Exit\n";
      cout << "Enter your choice: \n";
      cin >> choice;
      cin.ignore();  // Clear the newline character to prepare for next input

      switch (choice) {
      case 1:
        cout << "Enter chore doer's name: \n";
        getline(cin, input); // Get the chore doer's name from user
        manager.addChoreDoer(input); // Add the chore doer to the system
        cout << "Chore doer " << input << " added successfully.\n";
        break;
      case 2:
        manager.assignChoresRandomly(); // Assign chores to chore doers randomly
        cout << "Chores have been assigned randomly.\n";
        break;
      case 3:
        cout << "Listing all chores:\n";
        cout << manager.displayChoreList(); // Display a list of all chores
        break;
      case 4:
        cout << "Displaying chores assigned to all chore doers:\n";
        cout << manager.displayAllChoreAssignments(); // Display all assigned chores
        break;
      case 5:
        cout << "Displaying all chores with all data:\n";
        cout << manager.displayAllChoresAllAttributes(); // Display detailed info of all chores
        break;
      case 6:
        manager.outputChoreAssignmentsToFile(assignedChoresFile); // Output chore assignments to a file
        cout << "Chore assignments have been printed to " << assignedChoresFile << "\n";
        break;
      case 7:
        cout << "Enter the ID of the chore to delete: ";
        cin >> id;
        cin.ignore();
        manager.deleteChoreFromAvailable(id); // Delete the specified chore by ID
        cout << "Chore with ID " << id << " has been deleted.\n";
        break;
      case 8:
        cout << "Enter the name of the chore doer to delete: ";
        getline(cin, input);
        manager.deleteChoreDoer(input); // Delete the specified chore doer by name
        break;
      case 9:
        cout << "Displaying all chore doers:\n";
        cout << manager.displayChoreDoerList(); // Display all chore doers
        break;
      case 10:
        manager.searchMenu(); // Display the search menu
        break;
      case 11:
        int choreId1, choreId2;
        cout << "Enter the ID of the first chore to compare: ";
        cin >> choreId1;
        cout << "Enter the ID of the second chore to compare: ";
        cin >> choreId2;
        cin.ignore();
        manager.compareChores(choreId1, choreId2); // Compare two chores by ID
        break;
      case 12:
        manager.sortMenu(); // Display the sorting menu
        break;
      case 13:
        running = false; // Set the flag to false to exit the loop
        cout << "Exiting Chore Manager System.\n";
        break;
      default:
        cout << "Invalid choice. Please try again.\n"; // Handle invalid menu options
        break;
      }
    }
  }
  catch (const exception& e) {
    // Catch and display any exceptions that occur during execution
    cout << "Exception caught in main: " << e.what() << endl;
  }
  return 0; // Return 0 to indicate successful completion
}