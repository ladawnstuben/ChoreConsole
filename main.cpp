
// wx (and the Windows COM headers it pulls in on MSW) must be parsed BEFORE
// ChoreModel.h's "using namespace std;" takes effect — otherwise unqualified `byte`
// inside those Windows headers becomes ambiguous with std::byte (from <cstddef>,
// transitively included by ChoreModel.h's <filesystem>).
#pragma warning( push )
#pragma warning( disable: 4996 )
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/scrolwin.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/graphics.h>
#include <wx/clrpicker.h>
#include <wx/dcbuffer.h>
#pragma warning( pop )

#include "ChoreModel.h"

// TestData paths are resolved relative to the running executable's own directory
// (rather than the process's current working directory) so the app works the same
// whether launched via Visual Studio or by double-clicking the .exe. The executable
// lands at <repo>\x64\<Config>\ChoreConsole.exe, so walk up two directory levels to
// reach the repo root, then into TestData.
string GetTestDataDir() {
  wxFileName exeFile(wxStandardPaths::Get().GetExecutablePath());
  wxFileName dir = wxFileName::DirName(exeFile.GetPath()); // .../x64/<Config>
  dir.RemoveLastDir(); // .../x64
  dir.RemoveLastDir(); // repo root
  dir.AppendDir("TestData");
  return dir.GetPath().ToStdString();
}

string GetTestDataFilePath(const string& fileName) {
  wxFileName file(GetTestDataDir(), fileName);
  return file.GetFullPath().ToStdString();
}

string GetHouseholdsDir() {
  return GetTestDataDir() + "\\households";
}

string GetAppStatePath() {
  return GetTestDataFilePath("app_state.json");
}

// wxListCtrl has no GetFirstSelected() of its own; this is the standard idiom for it.
long GetFirstSelectedItem(wxListCtrl* list) {
  return list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}

// Splits a comma-separated field (as typed into the chore editor) into trimmed tokens.
vector<string> SplitCommaList(const string& text) {
  vector<string> result;
  stringstream ss(text);
  string token;
  while (getline(ss, token, ',')) {
    size_t start = token.find_first_not_of(" \t");
    size_t end = token.find_last_not_of(" \t");
    if (start != string::npos) {
      result.push_back(token.substr(start, end - start + 1));
    }
  }
  return result;
}

// Joins a vector back into a comma-separated string for prefilling an editable field.
string JoinCommaList(const vector<string>& items) {
  string result;
  for (const auto& item : items) {
    if (!result.empty()) result += ", ";
    result += item;
  }
  return result;
}

// Date-navigation helpers for the History tab. Must produce/consume the exact same
// zero-padded "YYYY-MM-DD" shape as ChoreModel.h's FormatDateNow(), since day lookups
// round-trip by plain string equality.
string TodayDateString() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_s(&timeinfo, &now);
  stringstream ss;
  ss << put_time(&timeinfo, "%Y-%m-%d");
  return ss.str();
}

string AddDaysToDateString(const string& yyyyMMdd, int deltaDays) {
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

string FormatDateForDisplay(const string& yyyyMMdd) {
  struct tm timeinfo = {};
  sscanf_s(yyyyMMdd.c_str(), "%d-%d-%d", &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday);
  timeinfo.tm_year -= 1900;
  timeinfo.tm_mon -= 1;
  timeinfo.tm_hour = 12;
  mktime(&timeinfo); // normalizes tm_wday from the date fields
  stringstream ss;
  ss << put_time(&timeinfo, "%A, %B %d, %Y");
  return ss.str();
}

namespace ChoreApp
{
  //*********************************************************************************
  // GUI
  //*********************************************************************************

  // Fired by a DoerCardPanel when clicked; carries the doer's name via GetString(),
  // so the existing name-keyed selectedDoerName plumbing in MainFrame needs no change.
  wxDECLARE_EVENT(EVT_DOER_CARD_SELECTED, wxCommandEvent);
  wxDEFINE_EVENT(EVT_DOER_CARD_SELECTED, wxCommandEvent);

  // Shared color palette for the "bubbly" visual language, centralized so every tab,
  // card, and button draws from the same set of accents instead of ad-hoc wxColour
  // literals scattered through each control.
  namespace Palette {
    static const wxColour Background(255, 250, 240);   // warm cream app/tab background
    static const wxColour CardBg(255, 255, 255);        // white card surfaces
    static const wxColour CardBorder(230, 222, 203);    // soft warm border for cards
    static const wxColour RowAlt(251, 246, 234);        // subtle zebra stripe on list rows
    static const wxColour Selection(255, 244, 214);     // selection fill (e.g. doer card)
    static const wxColour SelectionBorder(255, 183, 27); // selection border/accent
    static const wxColour TextPrimary(45, 42, 38);
    static const wxColour TextMuted(110, 105, 98);
    static const wxColour Purple(108, 92, 231);
    static const wxColour Teal(0, 184, 148);
    static const wxColour Coral(255, 107, 107);
    static const wxColour Blue(84, 160, 255);
    static const wxColour Amber(255, 159, 28);
  }

  // A small owner-drawn rounded button used across the main tabs for a friendlier
  // look than a native wxButton. Fires a genuine wxEVT_BUTTON with its own id on
  // click, so every existing Bind(wxEVT_BUTTON, &MainFrame::OnX, this) call site
  // works completely unmodified after swapping the constructor call.
  class RoundedButton : public wxPanel {
  public:
    RoundedButton(wxWindow* parent, wxWindowID id, const wxString& label,
      const wxColour& baseColor = Palette::Purple, const wxSize& size = wxSize(120, 36))
      : wxPanel(parent, id, wxDefaultPosition, size, wxBORDER_NONE),
      baseColor(baseColor), hovered(false), pressed(false)
    {
      SetLabel(label);
      SetBackgroundStyle(wxBG_STYLE_PAINT);
      SetCursor(wxCursor(wxCURSOR_HAND));
      Bind(wxEVT_PAINT, &RoundedButton::OnPaint, this);
      Bind(wxEVT_ENTER_WINDOW, &RoundedButton::OnEnter, this);
      Bind(wxEVT_LEAVE_WINDOW, &RoundedButton::OnLeave, this);
      Bind(wxEVT_LEFT_DOWN, &RoundedButton::OnLeftDown, this);
      Bind(wxEVT_LEFT_UP, &RoundedButton::OnLeftUp, this);
    }

    bool Enable(bool enable = true) override {
      bool result = wxPanel::Enable(enable);
      hovered = false;
      pressed = false;
      Refresh();
      return result;
    }

  private:
    wxColour baseColor;
    bool hovered;
    bool pressed;

    static wxColour Shade(const wxColour& c, int delta) {
      auto clamp = [](int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); };
      return wxColour(clamp(c.Red() + delta), clamp(c.Green() + delta), clamp(c.Blue() + delta));
    }

    void OnPaint(wxPaintEvent&) {
      wxAutoBufferedPaintDC dc(this);
      wxColour parentBg = GetParent() ? GetParent()->GetBackgroundColour() : *wxWHITE;
      dc.SetBackground(wxBrush(parentBg));
      dc.Clear();

      wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
      if (!gc) return;

      wxColour fill = !IsEnabled() ? wxColour(200, 200, 200)
        : pressed ? Shade(baseColor, -30)
        : hovered ? Shade(baseColor, 20)
        : baseColor;

      wxRect rect = GetClientRect();
      gc->SetBrush(wxBrush(fill));
      gc->SetPen(wxPen(Shade(fill, -40), 1));
      gc->DrawRoundedRectangle(1, 1, rect.width - 2, rect.height - 2, 10);

      wxFont font = GetFont();
      font.SetWeight(wxFONTWEIGHT_BOLD);
      gc->SetFont(font, IsEnabled() ? *wxWHITE : wxColour(230, 230, 230));
      wxString label = GetLabel();
      double textW, textH;
      gc->GetTextExtent(label, &textW, &textH);
      gc->DrawText(label, (rect.width - textW) / 2, (rect.height - textH) / 2);

      delete gc;
    }

    void OnEnter(wxMouseEvent&) { if (IsEnabled()) { hovered = true; Refresh(); } }
    void OnLeave(wxMouseEvent&) { hovered = false; pressed = false; Refresh(); }
    void OnLeftDown(wxMouseEvent&) { if (IsEnabled()) { pressed = true; Refresh(); } }

    void OnLeftUp(wxMouseEvent&) {
      if (IsEnabled() && pressed) {
        pressed = false;
        Refresh();
        wxCommandEvent evt(wxEVT_BUTTON, GetId());
        evt.SetEventObject(this);
        ProcessWindowEvent(evt);
      }
    }
  };

  // A colored circle with a bold initial letter, used both small (in doer cards) and
  // large (in the profile panel) — same class, different constructor size.
  class AvatarCircle : public wxWindow {
  public:
    AvatarCircle(wxWindow* parent, wxWindowID id, const wxColour& color, const wxString& initial, const wxSize& size)
      : wxWindow(parent, id, wxDefaultPosition, size), color(color), initial(initial)
    {
      SetBackgroundStyle(wxBG_STYLE_PAINT);
      Bind(wxEVT_PAINT, &AvatarCircle::OnPaint, this);
    }

    void SetAvatarColor(const wxColour& newColor) { color = newColor; Refresh(); }
    void SetInitial(const wxString& newInitial) { initial = newInitial; Refresh(); }

  private:
    wxColour color;
    wxString initial;

    void OnPaint(wxPaintEvent&) {
      wxAutoBufferedPaintDC dc(this);
      wxColour parentBg = GetParent() ? GetParent()->GetBackgroundColour() : *wxWHITE;
      dc.SetBackground(wxBrush(parentBg));
      dc.Clear();

      wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
      if (!gc) return;

      wxRect rect = GetClientRect();
      int d = min(rect.width, rect.height);
      double ox = (rect.width - d) / 2.0;
      double oy = (rect.height - d) / 2.0;

      gc->SetBrush(wxBrush(color));
      gc->SetPen(*wxTRANSPARENT_PEN);
      gc->DrawEllipse(ox, oy, d, d);

      wxFont font = GetFont();
      font.SetWeight(wxFONTWEIGHT_BOLD);
      font.SetPointSize(max(8, d / 2));
      gc->SetFont(font, *wxWHITE);
      double textW, textH;
      gc->GetTextExtent(initial, &textW, &textH);
      gc->DrawText(initial, ox + (d - textW) / 2, oy + (d - textH) / 2);

      delete gc;
    }
  };

  // A clickable card summarizing one chore doer (avatar, name, streak, earnings),
  // used as the master list on the redesigned Chore Doers tab. Fully self-painted
  // (no child controls) so its rounded background and the avatar circle composite
  // cleanly without any child-background-color matching to worry about.
  class DoerCardPanel : public wxPanel {
  public:
    DoerCardPanel(wxWindow* parent, wxWindowID id, const wxString& doerName, const wxColour& avatarColor,
      const wxString& streakText, const wxString& earningsText)
      : wxPanel(parent, id, wxDefaultPosition, wxSize(240, 64), wxBORDER_NONE),
      doerName(doerName), avatarColor(avatarColor), streakText(streakText), earningsText(earningsText), selected(false)
    {
      SetBackgroundStyle(wxBG_STYLE_PAINT);
      SetCursor(wxCursor(wxCURSOR_HAND));
      Bind(wxEVT_PAINT, &DoerCardPanel::OnPaint, this);
      Bind(wxEVT_LEFT_UP, &DoerCardPanel::OnClick, this);
    }

    void SetSelected(bool sel) { selected = sel; Refresh(); }
    bool IsSelected() const { return selected; }
    wxString GetDoerName() const { return doerName; }

    void UpdateInfo(const wxColour& newAvatarColor, const wxString& newStreakText, const wxString& newEarningsText) {
      avatarColor = newAvatarColor;
      streakText = newStreakText;
      earningsText = newEarningsText;
      Refresh();
    }

  private:
    wxString doerName;
    wxColour avatarColor;
    wxString streakText;
    wxString earningsText;
    bool selected;

    void OnClick(wxMouseEvent&) {
      wxCommandEvent evt(EVT_DOER_CARD_SELECTED, GetId());
      evt.SetString(doerName);
      evt.SetEventObject(this);
      ProcessWindowEvent(evt);
    }

    void OnPaint(wxPaintEvent&) {
      wxAutoBufferedPaintDC dc(this);
      wxColour bg = selected ? Palette::Selection : Palette::CardBg;
      dc.SetBackground(wxBrush(bg));
      dc.Clear();

      wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
      if (!gc) return;

      wxRect rect = GetClientRect();
      gc->SetBrush(wxBrush(bg));
      gc->SetPen(selected ? wxPen(Palette::SelectionBorder, 2) : wxPen(Palette::CardBorder, 1));
      gc->DrawRoundedRectangle(2, 2, rect.width - 4, rect.height - 4, 12);

      double d = rect.height - 20;
      double cx = 12, cy = 10;
      gc->SetBrush(wxBrush(avatarColor));
      gc->SetPen(*wxTRANSPARENT_PEN);
      gc->DrawEllipse(cx, cy, d, d);

      wxFont initialFont = GetFont();
      initialFont.SetWeight(wxFONTWEIGHT_BOLD);
      initialFont.SetPointSize((int)(d / 2.2));
      gc->SetFont(initialFont, *wxWHITE);
      wxString initial = doerName.IsEmpty() ? wxString("?") : doerName.Left(1).Upper();
      double textW, textH;
      gc->GetTextExtent(initial, &textW, &textH);
      gc->DrawText(initial, cx + (d - textW) / 2, cy + (d - textH) / 2);

      double textX = cx + d + 12;
      wxFont nameFont = GetFont();
      nameFont.SetWeight(wxFONTWEIGHT_BOLD);
      gc->SetFont(nameFont, *wxBLACK);
      gc->DrawText(doerName, textX, 6);

      wxFont smallFont = GetFont();
      smallFont.SetPointSize(max(7, smallFont.GetPointSize() - 1));
      gc->SetFont(smallFont, wxColour(90, 90, 90));
      gc->DrawText(streakText, textX, 27);
      gc->DrawText(earningsText, textX, 44);

      delete gc;
    }
  };

  // A plain rounded-rect container used to visually group related controls into a
  // "card" (same visual language as DoerCardPanel, minus the click/selection state).
  // Add content to GetInnerSizer() rather than calling SetSizer() directly, so
  // children sit inset from the rounded corners instead of covering them.
  class CardPanel : public wxPanel {
  public:
    CardPanel(wxWindow* parent, int cornerRadius = 14)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE), cornerRadius(cornerRadius)
    {
      SetBackgroundStyle(wxBG_STYLE_PAINT);
      innerSizer = new wxBoxSizer(wxVERTICAL);
      wxPanel::SetSizer(innerSizer);
      Bind(wxEVT_PAINT, &CardPanel::OnPaint, this);
    }

    wxSizer* GetInnerSizer() const { return innerSizer; }

  private:
    int cornerRadius;
    wxSizer* innerSizer;

    void OnPaint(wxPaintEvent&) {
      wxAutoBufferedPaintDC dc(this);
      wxColour parentBg = GetParent() ? GetParent()->GetBackgroundColour() : Palette::Background;
      dc.SetBackground(wxBrush(parentBg));
      dc.Clear();

      wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
      if (!gc) return;

      wxRect rect = GetClientRect();
      gc->SetBrush(wxBrush(Palette::CardBg));
      gc->SetPen(wxPen(Palette::CardBorder, 1));
      gc->DrawRoundedRectangle(1, 1, rect.width - 2, rect.height - 2, cornerRadius);

      delete gc;
    }
  };

  // A bold, slightly muted section label placed above a CardPanel (e.g. "All Chores").
  wxStaticText* MakeSectionTitle(wxWindow* parent, const wxString& text) {
    wxStaticText* title = new wxStaticText(parent, wxID_ANY, text);
    wxFont font = title->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    font.SetPointSize(font.GetPointSize() + 1);
    title->SetFont(font);
    title->SetForegroundColour(Palette::TextPrimary);
    return title;
  }

  // Lets the user search chores by ID, name, or earnings, and view the results inline.
  class SearchDialog : public wxDialog {
  public:
    SearchDialog(wxWindow* parent, ChoreManager& manager)
      : wxDialog(parent, wxID_ANY, "Search Chores", wxDefaultPosition, wxSize(420, 420)), manager(manager)
    {
      wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

      wxArrayString modes;
      modes.Add("By ID");
      modes.Add("By Name");
      modes.Add("By Earnings");
      modeBox = new wxRadioBox(this, wxID_ANY, "Search by", wxDefaultPosition, wxDefaultSize, modes, 1, wxRA_SPECIFY_ROWS);
      mainSizer->Add(modeBox, 0, wxALL | wxEXPAND, 8);

      valueCtrl = new wxTextCtrl(this, wxID_ANY);
      mainSizer->Add(valueCtrl, 0, wxALL | wxEXPAND, 8);

      wxButton* searchBtn = new wxButton(this, wxID_ANY, "Search");
      mainSizer->Add(searchBtn, 0, wxALL | wxALIGN_CENTER, 4);

      resultsList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(380, 180), wxLC_REPORT | wxLC_SINGLE_SEL);
      resultsList->InsertColumn(0, "ID", wxLIST_FORMAT_LEFT, 40);
      resultsList->InsertColumn(1, "Name", wxLIST_FORMAT_LEFT, 160);
      resultsList->InsertColumn(2, "Earnings", wxLIST_FORMAT_LEFT, 70);
      resultsList->InsertColumn(3, "Status", wxLIST_FORMAT_LEFT, 100);
      mainSizer->Add(resultsList, 1, wxALL | wxEXPAND, 8);

      wxButton* closeBtn = new wxButton(this, wxID_CANCEL, "Close");
      mainSizer->Add(closeBtn, 0, wxALL | wxALIGN_CENTER, 4);

      SetSizerAndFit(mainSizer);

      searchBtn->Bind(wxEVT_BUTTON, &SearchDialog::OnSearch, this);
    }

  private:
    ChoreManager& manager;
    wxRadioBox* modeBox;
    wxTextCtrl* valueCtrl;
    wxListCtrl* resultsList;

    void OnSearch(wxCommandEvent&) {
      vector<shared_ptr<Chore>> results;
      wxString value = valueCtrl->GetValue();
      int mode = modeBox->GetSelection();
      long numericValue = 0;
      if ((mode == 0 || mode == 2) && !value.ToLong(&numericValue)) {
        wxMessageBox("Enter a valid whole number.", "Invalid Input", wxOK | wxICON_WARNING, this);
        return;
      }
      if (mode == 0) {
        results = manager.searchByID((int)numericValue);
      }
      else if (mode == 1) {
        results = manager.searchByName(value.ToStdString());
      }
      else {
        results = manager.searchByEarnings((int)numericValue);
      }

      resultsList->DeleteAllItems();
      for (const auto& chore : results) {
        long row = resultsList->InsertItem(resultsList->GetItemCount(), to_string(chore->getId()));
        resultsList->SetItem(row, 1, chore->getName());
        resultsList->SetItem(row, 2, to_string(chore->getEarnings()));
        resultsList->SetItem(row, 3, chore->toStringS(chore->getStatus()));
      }
      if (results.empty()) {
        wxMessageBox("No chore found matching that criteria.", "No Results", wxOK | wxICON_INFORMATION, this);
      }
    }
  };

  // Lets the user compare two chores by ID.
  class CompareChoresDialog : public wxDialog {
  public:
    CompareChoresDialog(wxWindow* parent)
      : wxDialog(parent, wxID_ANY, "Compare Two Chores")
    {
      wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
      wxFlexGridSizer* grid = new wxFlexGridSizer(2, 2, 8, 8);
      grid->Add(new wxStaticText(this, wxID_ANY, "First Chore ID:"));
      id1Ctrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000, 0);
      grid->Add(id1Ctrl);
      grid->Add(new wxStaticText(this, wxID_ANY, "Second Chore ID:"));
      id2Ctrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000, 0);
      grid->Add(id2Ctrl);
      mainSizer->Add(grid, 0, wxALL, 12);
      mainSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 8);
      SetSizerAndFit(mainSizer);
    }

    int GetId1() const { return id1Ctrl->GetValue(); }
    int GetId2() const { return id2Ctrl->GetValue(); }

  private:
    wxSpinCtrl* id1Ctrl;
    wxSpinCtrl* id2Ctrl;
  };

  // Lets the user modify their username, notification preference, and theme.
  class ProfileDialog : public wxDialog {
  public:
    ProfileDialog(wxWindow* parent, Client& client)
      : wxDialog(parent, wxID_ANY, "Modify User Profile"), client(client)
    {
      wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

      wxBoxSizer* nameSizer = new wxBoxSizer(wxHORIZONTAL);
      nameSizer->Add(new wxStaticText(this, wxID_ANY, "Username:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
      usernameCtrl = new wxTextCtrl(this, wxID_ANY, client.getUserName());
      nameSizer->Add(usernameCtrl, 1, wxALL | wxEXPAND, 4);
      mainSizer->Add(nameSizer, 0, wxALL | wxEXPAND, 8);

      notifyCheck = new wxCheckBox(this, wxID_ANY, "Notifications Enabled");
      notifyCheck->SetValue(client.getNotify() == "Enabled");
      mainSizer->Add(notifyCheck, 0, wxALL, 8);

      wxArrayString themes;
      themes.Add("dark");
      themes.Add("light");
      themeBox = new wxRadioBox(this, wxID_ANY, "Theme", wxDefaultPosition, wxDefaultSize, themes, 1, wxRA_SPECIFY_ROWS);
      themeBox->SetSelection(client.getTheme() == "light" ? 1 : 0);
      mainSizer->Add(themeBox, 0, wxALL | wxEXPAND, 8);

      mainSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 8);
      SetSizerAndFit(mainSizer);

      Bind(wxEVT_BUTTON, &ProfileDialog::OnOK, this, wxID_OK);
    }

  private:
    Client& client;
    wxTextCtrl* usernameCtrl;
    wxCheckBox* notifyCheck;
    wxRadioBox* themeBox;

    void OnOK(wxCommandEvent& event) {
      string newName = usernameCtrl->GetValue().ToStdString();
      if (newName != client.getUserName()) {
        client.setUsername(newName);
      }
      bool wantNotify = notifyCheck->GetValue();
      if (wantNotify != (client.getNotify() == "Enabled")) {
        client.toggleNotify();
      }
      string wantTheme = themeBox->GetSelection() == 1 ? "light" : "dark";
      if (wantTheme != client.getTheme()) {
        client.toggleTheme();
      }
      event.Skip();
    }
  };

  // Full-field chore creation/editing dialog. Pass existingChore == nullptr for "New
  // Chore" mode (calls manager.createChore()); pass a live chore for "Edit" mode
  // (applies setters directly to that shared_ptr<Chore>, same as the rest of the app's
  // live-reference editing convention).
  class ChoreEditorDialog : public wxDialog {
  public:
    ChoreEditorDialog(wxWindow* parent, ChoreManager& manager, shared_ptr<Chore> existingChore)
      : wxDialog(parent, wxID_ANY, existingChore ? "Edit Chore" : "New Chore", wxDefaultPosition, wxSize(480, 700)),
      manager(manager), existingChore(existingChore)
    {
      wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
      wxScrolledWindow* scroll = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(440, 560), wxVSCROLL);
      scroll->SetScrollRate(0, 10);
      wxFlexGridSizer* grid = new wxFlexGridSizer(2, 6, 6);
      grid->AddGrowableCol(1, 1);

      auto addLabel = [&](const wxString& text) {
        grid->Add(new wxStaticText(scroll, wxID_ANY, text), 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
        };

      addLabel("Name:");
      nameCtrl = new wxTextCtrl(scroll, wxID_ANY, existingChore ? existingChore->getName() : "");
      grid->Add(nameCtrl, 1, wxEXPAND);

      addLabel("Category:");
      wxArrayString categoryChoices;
      for (const auto& entry : manager.getCategoryRegistry().listSorted()) categoryChoices.Add(entry.name);
      categoryCtrl = new wxComboBox(scroll, wxID_ANY, existingChore ? existingChore->getCategory() : "Uncategorized",
        wxDefaultPosition, wxDefaultSize, categoryChoices);
      categoryCtrl->SetToolTip("Pick an existing category, or type a new name to create one.");
      grid->Add(categoryCtrl, 1, wxEXPAND);

      addLabel("Frequency:");
      frequencyCtrl = new wxTextCtrl(scroll, wxID_ANY, existingChore ? existingChore->getFrequency() : "");
      grid->Add(frequencyCtrl, 1, wxEXPAND);

      addLabel("Estimated Time:");
      estimatedTimeCtrl = new wxTextCtrl(scroll, wxID_ANY, existingChore ? existingChore->getEstimatedTime() : "");
      grid->Add(estimatedTimeCtrl, 1, wxEXPAND);

      addLabel("Earnings ($):");
      earningsCtrl = new wxSpinCtrl(scroll, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000,
        existingChore ? existingChore->getEarnings() : 0);
      grid->Add(earningsCtrl, 1, wxEXPAND);

      addLabel("Priority:");
      wxArrayString priorities; priorities.Add("low"); priorities.Add("moderate"); priorities.Add("high");
      priorityCtrl = new wxChoice(scroll, wxID_ANY, wxDefaultPosition, wxDefaultSize, priorities);
      priorityCtrl->SetStringSelection(existingChore ? existingChore->toStringP(existingChore->getPriority()) : "low");
      grid->Add(priorityCtrl, 1, wxEXPAND);

      addLabel("Status:");
      wxArrayString statuses; statuses.Add("not started"); statuses.Add("in progress"); statuses.Add("completed");
      statusCtrl = new wxChoice(scroll, wxID_ANY, wxDefaultPosition, wxDefaultSize, statuses);
      statusCtrl->SetStringSelection(existingChore ? existingChore->toStringS(existingChore->getStatus()) : "not started");
      grid->Add(statusCtrl, 1, wxEXPAND);

      addLabel("Location:");
      locationCtrl = new wxTextCtrl(scroll, wxID_ANY, existingChore ? existingChore->getLocation() : "");
      grid->Add(locationCtrl, 1, wxEXPAND);

      addLabel("Days:");
      daysCtrl = new wxTextCtrl(scroll, wxID_ANY, existingChore ? JoinCommaList(existingChore->getDays()) : "");
      daysCtrl->SetToolTip("Comma-separated, e.g. Monday, Wednesday");
      grid->Add(daysCtrl, 1, wxEXPAND);

      addLabel("Tools Required:");
      toolsCtrl = new wxTextCtrl(scroll, wxID_ANY, existingChore ? JoinCommaList(existingChore->getToolsRequired()) : "");
      toolsCtrl->SetToolTip("Comma-separated, e.g. Mop, Bucket");
      grid->Add(toolsCtrl, 1, wxEXPAND);

      addLabel("Materials Needed:");
      materialsCtrl = new wxTextCtrl(scroll, wxID_ANY, existingChore ? JoinCommaList(existingChore->getMaterialsNeeded()) : "");
      materialsCtrl->SetToolTip("Comma-separated, e.g. Soap, Rags");
      grid->Add(materialsCtrl, 1, wxEXPAND);

      addLabel("Tags:");
      tagsCtrl = new wxTextCtrl(scroll, wxID_ANY, existingChore ? JoinCommaList(existingChore->getTags()) : "");
      tagsCtrl->SetToolTip("Comma-separated, e.g. daily, kitchen");
      grid->Add(tagsCtrl, 1, wxEXPAND);

      addLabel("Description:");
      descCtrl = new wxTextCtrl(scroll, wxID_ANY, existingChore ? existingChore->getDescription() : "",
        wxDefaultPosition, wxSize(-1, 60), wxTE_MULTILINE);
      grid->Add(descCtrl, 1, wxEXPAND);

      addLabel("Notes:");
      notesCtrl = new wxTextCtrl(scroll, wxID_ANY, existingChore ? existingChore->getNotes() : "",
        wxDefaultPosition, wxSize(-1, 80), wxTE_MULTILINE);
      grid->Add(notesCtrl, 1, wxEXPAND);

      scroll->SetSizer(grid);
      scroll->FitInside();
      mainSizer->Add(scroll, 1, wxALL | wxEXPAND, 8);
      mainSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 8);
      SetSizerAndFit(mainSizer);

      Bind(wxEVT_BUTTON, &ChoreEditorDialog::OnOK, this, wxID_OK);
    }

  private:
    ChoreManager& manager;
    shared_ptr<Chore> existingChore;
    wxTextCtrl* nameCtrl;
    wxComboBox* categoryCtrl;
    wxTextCtrl* frequencyCtrl;
    wxTextCtrl* estimatedTimeCtrl;
    wxSpinCtrl* earningsCtrl;
    wxChoice* priorityCtrl;
    wxChoice* statusCtrl;
    wxTextCtrl* locationCtrl;
    wxTextCtrl* daysCtrl;
    wxTextCtrl* toolsCtrl;
    wxTextCtrl* materialsCtrl;
    wxTextCtrl* tagsCtrl;
    wxTextCtrl* descCtrl;
    wxTextCtrl* notesCtrl;

    void OnOK(wxCommandEvent& event) {
      string categoryName = categoryCtrl->GetValue().ToStdString();
      if (categoryName.empty()) categoryName = "Uncategorized";

      if (!manager.getCategoryRegistry().exists(categoryName)) {
        int confirm = wxMessageBox("Category '" + categoryName + "' doesn't exist yet. Add it?",
          "New Category", wxYES_NO | wxICON_QUESTION, this);
        if (confirm != wxYES) return; // Leave the dialog open so the field can be fixed.
        manager.createCategory(categoryName);
      }

      if (nameCtrl->GetValue().IsEmpty()) {
        wxMessageBox("Name cannot be empty.", "Missing Name", wxOK | wxICON_WARNING, this);
        return;
      }

      if (existingChore) {
        existingChore->setName(nameCtrl->GetValue().ToStdString());
        existingChore->setDescription(descCtrl->GetValue().ToStdString());
        existingChore->setCategory(categoryName);
        existingChore->setFrequency(frequencyCtrl->GetValue().ToStdString());
        existingChore->setEstimatedTime(estimatedTimeCtrl->GetValue().ToStdString());
        existingChore->setEarnings(earningsCtrl->GetValue());
        existingChore->setPriority(Chore::priorityFromString(priorityCtrl->GetStringSelection().ToStdString()));
        existingChore->setStatus(Chore::statusFromString(statusCtrl->GetStringSelection().ToStdString()));
        existingChore->setLocation(locationCtrl->GetValue().ToStdString());
        existingChore->setDays(SplitCommaList(daysCtrl->GetValue().ToStdString()));
        existingChore->setToolsRequired(SplitCommaList(toolsCtrl->GetValue().ToStdString()));
        existingChore->setMaterialsNeeded(SplitCommaList(materialsCtrl->GetValue().ToStdString()));
        existingChore->setTags(SplitCommaList(tagsCtrl->GetValue().ToStdString()));
        existingChore->setNotes(notesCtrl->GetValue().ToStdString());
      }
      else {
        json fields;
        fields["name"] = nameCtrl->GetValue().ToStdString();
        fields["description"] = descCtrl->GetValue().ToStdString();
        fields["category"] = categoryName;
        fields["frequency"] = frequencyCtrl->GetValue().ToStdString();
        fields["estimated_time"] = estimatedTimeCtrl->GetValue().ToStdString();
        fields["earnings"] = earningsCtrl->GetValue();
        fields["priority"] = priorityCtrl->GetStringSelection().ToStdString();
        fields["status"] = statusCtrl->GetStringSelection().ToStdString();
        fields["location"] = locationCtrl->GetValue().ToStdString();
        fields["days"] = SplitCommaList(daysCtrl->GetValue().ToStdString());
        fields["tools_required"] = SplitCommaList(toolsCtrl->GetValue().ToStdString());
        fields["materials_needed"] = SplitCommaList(materialsCtrl->GetValue().ToStdString());
        fields["tags"] = SplitCommaList(tagsCtrl->GetValue().ToStdString());
        fields["notes"] = notesCtrl->GetValue().ToStdString();
        manager.createChore(fields);
      }

      event.Skip();
    }
  };

  // Lets the user pick a chore doer's avatar color and edit their notes/preferences.
  class DoerProfileEditDialog : public wxDialog {
  public:
    DoerProfileEditDialog(wxWindow* parent, ChoreDoer& doer)
      : wxDialog(parent, wxID_ANY, "Edit " + doer.getName() + "'s Profile", wxDefaultPosition, wxSize(380, 320)), doer(doer)
    {
      wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

      wxBoxSizer* colorSizer = new wxBoxSizer(wxHORIZONTAL);
      colorSizer->Add(new wxStaticText(this, wxID_ANY, "Avatar Color:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
      wxColour initialColor(doer.getAvatarColor().empty() ? "#4ECDC4" : doer.getAvatarColor());
      colorPicker = new wxColourPickerCtrl(this, wxID_ANY, initialColor);
      colorSizer->Add(colorPicker, 0, wxALL, 4);
      mainSizer->Add(colorSizer, 0, wxALL, 8);

      mainSizer->Add(new wxStaticText(this, wxID_ANY, "Notes / Preferences:"), 0, wxLEFT | wxTOP, 8);
      notesCtrl = new wxTextCtrl(this, wxID_ANY, doer.getNotes(), wxDefaultPosition, wxSize(-1, 140), wxTE_MULTILINE);
      mainSizer->Add(notesCtrl, 1, wxALL | wxEXPAND, 8);

      mainSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 8);
      SetSizerAndFit(mainSizer);

      Bind(wxEVT_BUTTON, &DoerProfileEditDialog::OnOK, this, wxID_OK);
    }

  private:
    ChoreDoer& doer;
    wxColourPickerCtrl* colorPicker;
    wxTextCtrl* notesCtrl;

    void OnOK(wxCommandEvent& event) {
      doer.setAvatarColor(colorPicker->GetColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString());
      doer.setNotes(notesCtrl->GetValue().ToStdString());
      event.Skip();
    }
  };

  // Add/rename/delete/reorder categories.
  class ManageCategoriesDialog : public wxDialog {
  public:
    ManageCategoriesDialog(wxWindow* parent, ChoreManager& manager)
      : wxDialog(parent, wxID_ANY, "Manage Categories", wxDefaultPosition, wxSize(380, 440)), manager(manager)
    {
      wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
      list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(340, 220), wxLC_REPORT | wxLC_SINGLE_SEL);
      list->InsertColumn(0, "Category", wxLIST_FORMAT_LEFT, 220);
      list->InsertColumn(1, "Order", wxLIST_FORMAT_LEFT, 80);
      mainSizer->Add(list, 1, wxALL | wxEXPAND, 8);

      wxBoxSizer* row1 = new wxBoxSizer(wxHORIZONTAL);
      wxButton* addBtn = new wxButton(this, wxID_ANY, "Add");
      wxButton* renameBtn = new wxButton(this, wxID_ANY, "Rename");
      wxButton* deleteBtn = new wxButton(this, wxID_ANY, "Delete");
      row1->Add(addBtn, 0, wxALL, 4);
      row1->Add(renameBtn, 0, wxALL, 4);
      row1->Add(deleteBtn, 0, wxALL, 4);
      mainSizer->Add(row1, 0, wxALIGN_LEFT);

      wxBoxSizer* row2 = new wxBoxSizer(wxHORIZONTAL);
      wxButton* upBtn = new wxButton(this, wxID_ANY, "Move Up");
      wxButton* downBtn = new wxButton(this, wxID_ANY, "Move Down");
      row2->Add(upBtn, 0, wxALL, 4);
      row2->Add(downBtn, 0, wxALL, 4);
      mainSizer->Add(row2, 0, wxALIGN_LEFT);

      mainSizer->Add(CreateButtonSizer(wxCLOSE), 0, wxALL | wxEXPAND, 8);
      SetSizerAndFit(mainSizer);

      RefreshList();

      addBtn->Bind(wxEVT_BUTTON, &ManageCategoriesDialog::OnAdd, this);
      renameBtn->Bind(wxEVT_BUTTON, &ManageCategoriesDialog::OnRename, this);
      deleteBtn->Bind(wxEVT_BUTTON, &ManageCategoriesDialog::OnDelete, this);
      upBtn->Bind(wxEVT_BUTTON, &ManageCategoriesDialog::OnMoveUp, this);
      downBtn->Bind(wxEVT_BUTTON, &ManageCategoriesDialog::OnMoveDown, this);
      Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); }, wxID_CLOSE);
    }

  private:
    ChoreManager& manager;
    wxListCtrl* list;

    void RefreshList() {
      list->DeleteAllItems();
      for (const auto& entry : manager.getCategoryRegistry().listSorted()) {
        long row = list->InsertItem(list->GetItemCount(), entry.name);
        list->SetItem(row, 1, to_string(entry.sortWeight));
      }
    }

    string GetSelectedName() {
      long sel = GetFirstSelectedItem(list);
      if (sel == -1) return "";
      return list->GetItemText(sel, 0).ToStdString();
    }

    void OnAdd(wxCommandEvent&) {
      wxString name = wxGetTextFromUser("New category name:", "Add Category", "", this);
      if (name.IsEmpty()) return;
      string err = manager.createCategory(name.ToStdString());
      if (!err.empty()) wxMessageBox(err, "Add Failed", wxOK | wxICON_ERROR, this);
      RefreshList();
    }

    void OnRename(wxCommandEvent&) {
      string name = GetSelectedName();
      if (name.empty()) { wxMessageBox("Select a category first.", "No Selection", wxOK | wxICON_WARNING, this); return; }
      if (name == "Uncategorized") { wxMessageBox("'Uncategorized' cannot be renamed.", "Not Allowed", wxOK | wxICON_WARNING, this); return; }
      wxString newName = wxGetTextFromUser("Rename category to:", "Rename Category", name, this);
      if (newName.IsEmpty()) return;
      string err = manager.renameCategory(name, newName.ToStdString());
      if (!err.empty()) wxMessageBox(err, "Rename Failed", wxOK | wxICON_ERROR, this);
      RefreshList();
    }

    void OnDelete(wxCommandEvent&) {
      string name = GetSelectedName();
      if (name.empty()) { wxMessageBox("Select a category first.", "No Selection", wxOK | wxICON_WARNING, this); return; }
      int confirm = wxMessageBox("Delete category '" + name + "'?", "Confirm Delete", wxYES_NO | wxICON_WARNING, this);
      if (confirm != wxYES) return;
      string err = manager.deleteCategory(name);
      if (!err.empty()) wxMessageBox(err, "Delete Failed", wxOK | wxICON_ERROR, this);
      RefreshList();
    }

    void OnMoveUp(wxCommandEvent&) {
      string name = GetSelectedName();
      if (!name.empty()) manager.moveCategoryUp(name);
      RefreshList();
    }

    void OnMoveDown(wxCommandEvent&) {
      string name = GetSelectedName();
      if (!name.empty()) manager.moveCategoryDown(name);
      RefreshList();
    }
  };

  // Shown at startup when there's more than one household and no unambiguous choice,
  // and reused for the "Switch Household..." menu action.
  class HouseholdPickerDialog : public wxDialog {
  public:
    HouseholdPickerDialog(wxWindow* parent, HouseholdRegistry& registry, const string& currentPath)
      : wxDialog(parent, wxID_ANY, "Choose a Household", wxDefaultPosition, wxSize(360, 320))
    {
      wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
      mainSizer->Add(new wxStaticText(this, wxID_ANY, "Select a household to open:"), 0, wxALL, 8);

      list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(320, 200), wxLC_REPORT | wxLC_SINGLE_SEL);
      list->InsertColumn(0, "Household", wxLIST_FORMAT_LEFT, 300);

      const auto& households = registry.listHouseholds();
      long selectRow = 0;
      for (size_t i = 0; i < households.size(); i++) {
        long row = list->InsertItem(list->GetItemCount(), households[i].displayName);
        paths.push_back(households[i].filePath);
        if (households[i].filePath == currentPath) selectRow = row;
      }
      if (!households.empty()) {
        list->SetItemState(selectRow, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
      }
      mainSizer->Add(list, 1, wxALL | wxEXPAND, 8);

      mainSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 8);
      SetSizerAndFit(mainSizer);
    }

    string GetSelectedPath() const {
      long sel = GetFirstSelectedItem(list);
      if (sel == -1 || (size_t)sel >= paths.size()) return "";
      return paths[sel];
    }

  private:
    wxListCtrl* list;
    vector<string> paths;
  };

  // Rename/delete households. Renaming the currently-open household routes through the
  // live ChoreManager (it holds the in-memory copy); renaming any other goes straight
  // to its file. Deleting the currently-open household is blocked.
  class ManageHouseholdsDialog : public wxDialog {
  public:
    ManageHouseholdsDialog(wxWindow* parent, HouseholdRegistry& registry, ChoreManager& activeManager)
      : wxDialog(parent, wxID_ANY, "Manage Households", wxDefaultPosition, wxSize(420, 400)),
      registry(registry), activeManager(activeManager)
    {
      wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
      list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(380, 250), wxLC_REPORT | wxLC_SINGLE_SEL);
      list->InsertColumn(0, "Household", wxLIST_FORMAT_LEFT, 360);
      mainSizer->Add(list, 1, wxALL | wxEXPAND, 8);

      wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
      wxButton* renameBtn = new wxButton(this, wxID_ANY, "Rename");
      wxButton* deleteBtn = new wxButton(this, wxID_ANY, "Delete");
      btnSizer->Add(renameBtn, 0, wxALL, 4);
      btnSizer->Add(deleteBtn, 0, wxALL, 4);
      mainSizer->Add(btnSizer, 0, wxALIGN_LEFT);

      mainSizer->Add(CreateButtonSizer(wxCLOSE), 0, wxALL | wxEXPAND, 8);
      SetSizerAndFit(mainSizer);

      RefreshList();

      renameBtn->Bind(wxEVT_BUTTON, &ManageHouseholdsDialog::OnRename, this);
      deleteBtn->Bind(wxEVT_BUTTON, &ManageHouseholdsDialog::OnDelete, this);
      Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); }, wxID_CLOSE);
    }

  private:
    HouseholdRegistry& registry;
    ChoreManager& activeManager;
    wxListCtrl* list;

    void RefreshList() {
      list->DeleteAllItems();
      for (const auto& h : registry.listHouseholds()) {
        wxString label = h.displayName;
        if (h.filePath == registry.getLastOpenHouseholdPath()) label += " (current)";
        list->InsertItem(list->GetItemCount(), label);
      }
    }

    void OnRename(wxCommandEvent&) {
      long sel = GetFirstSelectedItem(list);
      const auto& households = registry.listHouseholds();
      if (sel == -1 || (size_t)sel >= households.size()) {
        wxMessageBox("Select a household first.", "No Selection", wxOK | wxICON_WARNING, this);
        return;
      }
      string path = households[sel].filePath;
      wxString newName = wxGetTextFromUser("New household name:", "Rename Household", households[sel].displayName, this);
      if (newName.IsEmpty()) return;

      if (path == registry.getLastOpenHouseholdPath()) {
        activeManager.setHouseholdName(newName.ToStdString());
        activeManager.saveData();
      }
      string err = registry.renameHousehold(path, newName.ToStdString());
      if (!err.empty()) {
        wxMessageBox(err, "Rename Failed", wxOK | wxICON_ERROR, this);
      }
      RefreshList();
    }

    void OnDelete(wxCommandEvent&) {
      long sel = GetFirstSelectedItem(list);
      const auto& households = registry.listHouseholds();
      if (sel == -1 || (size_t)sel >= households.size()) {
        wxMessageBox("Select a household first.", "No Selection", wxOK | wxICON_WARNING, this);
        return;
      }
      string path = households[sel].filePath;
      string name = households[sel].displayName;

      if (path == registry.getLastOpenHouseholdPath()) {
        wxMessageBox("Cannot delete the currently open household - switch to another one first.",
          "Delete Blocked", wxOK | wxICON_WARNING, this);
        return;
      }
      int confirm = wxMessageBox("Delete household '" + name + "'? This cannot be undone.",
        "Confirm Delete", wxYES_NO | wxICON_WARNING, this);
      if (confirm != wxYES) return;
      string err = registry.deleteHousehold(path);
      if (!err.empty()) {
        wxMessageBox(err, "Delete Failed", wxOK | wxICON_ERROR, this);
      }
      RefreshList();
    }
  };

  // Menu/control IDs. The Sort/SortAll/Doer-sort groups are each a contiguous range
  // so a single Bind() can dispatch the whole group via event.GetId().
  enum {
    ID_OUTPUT_FILE = wxID_HIGHEST + 1,
    ID_SAVE,
    ID_ASSIGN_RANDOM,
    ID_NEW_CHORE,
    ID_DELETE_CHORE,
    ID_SEARCH,
    ID_COMPARE,
    ID_SORT_ID,
    ID_SORT_NAME,
    ID_SORT_EARNINGS,
    ID_SORT_CATEGORY,
    ID_SORT_DESC,
    ID_VIEW_DETAILS,
    ID_MANAGE_CATEGORIES,
    ID_ADD_DOER,
    ID_DELETE_DOER,
    ID_VIEW_ASSIGNMENTS,
    ID_SORTALL_ID,
    ID_SORTALL_NAME,
    ID_SORTALL_EARNINGS,
    ID_SORTALL_CATEGORY,
    ID_MODIFY_PROFILE,
    ID_DOER_SORT_ID,
    ID_DOER_SORT_NAME,
    ID_DOER_SORT_EARNINGS,
    ID_DOER_SORT_CATEGORY,
    ID_SWITCH_HOUSEHOLD,
    ID_NEW_HOUSEHOLD,
    ID_MANAGE_HOUSEHOLDS,
    ID_EDIT_DOER_PROFILE,
    ID_PREV_DAY,
    ID_NEXT_DAY,
    ID_TODAY_DAY,
  };

  class MainFrame : public wxFrame {
  public:
    MainFrame(unique_ptr<ChoreManager> mgr, HouseholdRegistry& hReg)
      : wxFrame(nullptr, wxID_ANY, "Chore Manager", wxDefaultPosition, wxSize(950, 680)),
      manager(std::move(mgr)), householdRegistry(hReg)
    {
      currentHistoryDate = TodayDateString();

      // A slightly larger, bold-friendly system font, set before building any child
      // controls so they inherit it at creation time (wx children pick up the parent's
      // font when they're constructed, not dynamically afterward).
      wxFont friendlyFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Segoe UI");
      SetFont(friendlyFont);

      BuildMenuBar();

      wxPanel* panel = new wxPanel(this);
      panel->SetBackgroundColour(Palette::Background);
      wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

      BuildHeaderBar(panel, rootSizer);

      notebook = new wxNotebook(panel, wxID_ANY);

      BuildChoresTab();
      BuildChoreDoersTab();
      BuildHistoryTab();

      rootSizer->Add(notebook, 1, wxEXPAND | wxALL, 4);
      panel->SetSizer(rootSizer);

      CreateStatusBar();
      UpdateTitle();

      RefreshChoresList();
      RefreshChoreDoersList();
      RefreshHistoryList();

      // If this is a brand-new profile, prompt for a username right away via the same
      // dialog used for later edits, instead of the old console's blocking cin prompt.
      if (manager->getClient().getUserName() == "DefaultUser") {
        ProfileDialog dlg(this, manager->getClient());
        if (dlg.ShowModal() == wxID_OK) {
          manager->saveData();
        }
      }
    }

  private:
    unique_ptr<ChoreManager> manager;
    HouseholdRegistry& householdRegistry;
    wxStaticText* householdLabel;
    wxNotebook* notebook;
    wxListCtrl* choresList;
    wxTextCtrl* choreDetailText;

    // Chore Doers tab: a scrollable column of doer "cards" (master) plus a profile
    // panel (detail) for whichever one is selected.
    wxScrolledWindow* doerCardsScroll;
    wxBoxSizer* doerCardsSizer;
    vector<DoerCardPanel*> doerCards;
    wxPanel* doerProfilePanel;
    AvatarCircle* profileAvatar;
    wxStaticText* profileNameText;
    wxStaticText* profileStreakText;
    wxStaticText* profileEarningsText;
    wxTextCtrl* profileNotesText;
    wxListCtrl* doerChoresList;
    RoundedButton* startBtn;
    RoundedButton* completeBtn;
    RoundedButton* resetBtn;
    wxString selectedDoerName;

    // History tab
    wxListCtrl* historyList;
    wxStaticText* historyDateLabel;
    string currentHistoryDate;

    void UpdateTitle() {
      SetTitle("Chore Manager - " + manager->getHouseholdName());
      if (householdLabel) householdLabel->SetLabel(manager->getHouseholdName());
    }

    void BuildMenuBar() {
      wxMenuBar* menuBar = new wxMenuBar();

      wxMenu* fileMenu = new wxMenu();
      fileMenu->Append(ID_OUTPUT_FILE, "Output Chore Assignments to File...");
      fileMenu->Append(ID_SAVE, "Save");
      fileMenu->AppendSeparator();
      fileMenu->Append(wxID_EXIT, "Exit");
      menuBar->Append(fileMenu, "&File");

      wxMenu* choresMenu = new wxMenu();
      choresMenu->Append(ID_NEW_CHORE, "New Chore...");
      choresMenu->Append(ID_ASSIGN_RANDOM, "Assign Chores Randomly");
      choresMenu->Append(ID_DELETE_CHORE, "Delete Selected Chore");
      choresMenu->Append(ID_SEARCH, "Search...");
      choresMenu->Append(ID_COMPARE, "Compare Two Chores...");
      wxMenu* sortMenu = new wxMenu();
      sortMenu->Append(ID_SORT_ID, "By ID");
      sortMenu->Append(ID_SORT_NAME, "By Name");
      sortMenu->Append(ID_SORT_EARNINGS, "By Earnings");
      sortMenu->Append(ID_SORT_CATEGORY, "By Category");
      sortMenu->AppendSeparator();
      sortMenu->AppendCheckItem(ID_SORT_DESC, "Descending");
      choresMenu->AppendSubMenu(sortMenu, "Sort");
      choresMenu->Append(ID_VIEW_DETAILS, "View Full Details...");
      choresMenu->Append(ID_MANAGE_CATEGORIES, "Manage Categories...");
      menuBar->Append(choresMenu, "&Chores");

      wxMenu* doersMenu = new wxMenu();
      doersMenu->Append(ID_ADD_DOER, "Add Chore Doer...");
      doersMenu->Append(ID_DELETE_DOER, "Delete Selected Chore Doer");
      doersMenu->Append(ID_EDIT_DOER_PROFILE, "Edit Selected Doer's Profile...");
      doersMenu->Append(ID_VIEW_ASSIGNMENTS, "View All Assignments...");
      wxMenu* sortAllMenu = new wxMenu();
      sortAllMenu->Append(ID_SORTALL_ID, "By ID");
      sortAllMenu->Append(ID_SORTALL_NAME, "By Name");
      sortAllMenu->Append(ID_SORTALL_EARNINGS, "By Earnings");
      sortAllMenu->Append(ID_SORTALL_CATEGORY, "By Category");
      doersMenu->AppendSubMenu(sortAllMenu, "Sort All Doers' Chores");
      menuBar->Append(doersMenu, "Chore &Doers");

      wxMenu* householdMenu = new wxMenu();
      householdMenu->Append(ID_SWITCH_HOUSEHOLD, "Switch Household...");
      householdMenu->Append(ID_NEW_HOUSEHOLD, "New Household...");
      householdMenu->Append(ID_MANAGE_HOUSEHOLDS, "Manage Households...");
      menuBar->Append(householdMenu, "&Household");

      wxMenu* profileMenu = new wxMenu();
      profileMenu->Append(ID_MODIFY_PROFILE, "Modify Profile...");
      menuBar->Append(profileMenu, "&Profile");

      SetMenuBar(menuBar);

      Bind(wxEVT_MENU, &MainFrame::OnOutputToFile, this, ID_OUTPUT_FILE);
      Bind(wxEVT_MENU, &MainFrame::OnSave, this, ID_SAVE);
      Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);
      Bind(wxEVT_MENU, &MainFrame::OnNewChore, this, ID_NEW_CHORE);
      Bind(wxEVT_MENU, &MainFrame::OnAssignRandom, this, ID_ASSIGN_RANDOM);
      Bind(wxEVT_MENU, &MainFrame::OnDeleteChore, this, ID_DELETE_CHORE);
      Bind(wxEVT_MENU, &MainFrame::OnSearch, this, ID_SEARCH);
      Bind(wxEVT_MENU, &MainFrame::OnCompare, this, ID_COMPARE);
      Bind(wxEVT_MENU, &MainFrame::OnSortChores, this, ID_SORT_ID, ID_SORT_CATEGORY);
      Bind(wxEVT_MENU, &MainFrame::OnViewDetails, this, ID_VIEW_DETAILS);
      Bind(wxEVT_MENU, &MainFrame::OnManageCategories, this, ID_MANAGE_CATEGORIES);
      Bind(wxEVT_MENU, &MainFrame::OnAddChoreDoer, this, ID_ADD_DOER);
      Bind(wxEVT_MENU, &MainFrame::OnDeleteChoreDoer, this, ID_DELETE_DOER);
      Bind(wxEVT_MENU, &MainFrame::OnEditDoerProfile, this, ID_EDIT_DOER_PROFILE);
      Bind(wxEVT_MENU, &MainFrame::OnViewAssignments, this, ID_VIEW_ASSIGNMENTS);
      Bind(wxEVT_MENU, &MainFrame::OnSortAllDoers, this, ID_SORTALL_ID, ID_SORTALL_CATEGORY);
      Bind(wxEVT_MENU, &MainFrame::OnSwitchHousehold, this, ID_SWITCH_HOUSEHOLD);
      Bind(wxEVT_MENU, &MainFrame::OnNewHousehold, this, ID_NEW_HOUSEHOLD);
      Bind(wxEVT_MENU, &MainFrame::OnManageHouseholds, this, ID_MANAGE_HOUSEHOLDS);
      Bind(wxEVT_MENU, &MainFrame::OnModifyProfile, this, ID_MODIFY_PROFILE);
    }

    // A slim custom app-bar replacing the dated native wxToolBar, matching the rest of
    // the bubbly redesign. Shows the app/household identity on the left and the two
    // actions that are meaningful from any tab (Save, Search) as RoundedButtons on the
    // right; tab-specific actions (New/Delete/Modify Chore, etc.) stay on their tabs.
    void BuildHeaderBar(wxPanel* panel, wxBoxSizer* rootSizer) {
      wxPanel* header = new wxPanel(panel);
      header->SetBackgroundColour(Palette::CardBg);
      wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);

      wxBoxSizer* titleSizer = new wxBoxSizer(wxVERTICAL);
      wxStaticText* appTitle = new wxStaticText(header, wxID_ANY, "Chore Manager");
      wxFont titleFont = appTitle->GetFont();
      titleFont.SetPointSize(titleFont.GetPointSize() + 4);
      titleFont.SetWeight(wxFONTWEIGHT_BOLD);
      appTitle->SetFont(titleFont);
      appTitle->SetForegroundColour(Palette::Purple);
      titleSizer->Add(appTitle);
      householdLabel = new wxStaticText(header, wxID_ANY, "");
      householdLabel->SetForegroundColour(Palette::TextMuted);
      titleSizer->Add(householdLabel);
      headerSizer->Add(titleSizer, 1, wxALIGN_CENTER_VERTICAL | wxALL, 14);

      RoundedButton* searchBtn = new RoundedButton(header, wxID_ANY, "Search", Palette::Blue, wxSize(90, 34));
      RoundedButton* saveBtn = new RoundedButton(header, wxID_ANY, "Save", Palette::Teal, wxSize(90, 34));
      headerSizer->Add(searchBtn, 0, wxALIGN_CENTER_VERTICAL | wxALL, 10);
      headerSizer->Add(saveBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxTOP | wxBOTTOM, 10);

      header->SetSizer(headerSizer);
      rootSizer->Add(header, 0, wxEXPAND | wxBOTTOM, 6);

      searchBtn->Bind(wxEVT_BUTTON, &MainFrame::OnSearch, this);
      saveBtn->Bind(wxEVT_BUTTON, &MainFrame::OnSave, this);
    }

    void BuildChoresTab() {
      wxPanel* choresPanel = new wxPanel(notebook);
      choresPanel->SetBackgroundColour(Palette::Background);
      wxBoxSizer* choresSizer = new wxBoxSizer(wxVERTICAL);

      choresSizer->Add(MakeSectionTitle(choresPanel, "All Chores"), 0, wxLEFT | wxTOP, 12);
      CardPanel* listCard = new CardPanel(choresPanel);
      choresList = new wxListCtrl(listCard, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
      choresList->InsertColumn(0, "ID", wxLIST_FORMAT_LEFT, 40);
      choresList->InsertColumn(1, "Name", wxLIST_FORMAT_LEFT, 160);
      choresList->InsertColumn(2, "Category", wxLIST_FORMAT_LEFT, 100);
      choresList->InsertColumn(3, "Earnings", wxLIST_FORMAT_LEFT, 70);
      choresList->InsertColumn(4, "Status", wxLIST_FORMAT_LEFT, 100);
      choresList->InsertColumn(5, "Priority", wxLIST_FORMAT_LEFT, 80);
      choresList->InsertColumn(6, "Frequency", wxLIST_FORMAT_LEFT, 100);
      listCard->GetInnerSizer()->Add(choresList, 1, wxALL | wxEXPAND, 12);
      choresSizer->Add(listCard, 2, wxALL | wxEXPAND, 8);

      choresSizer->Add(MakeSectionTitle(choresPanel, "Details"), 0, wxLEFT, 12);
      CardPanel* detailCard = new CardPanel(choresPanel);
      choreDetailText = new wxTextCtrl(detailCard, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 120), wxTE_MULTILINE | wxTE_READONLY | wxBORDER_NONE);
      detailCard->GetInnerSizer()->Add(choreDetailText, 1, wxALL | wxEXPAND, 12);
      choresSizer->Add(detailCard, 1, wxALL | wxEXPAND, 8);

      wxBoxSizer* choreBtnSizer = new wxBoxSizer(wxHORIZONTAL);
      RoundedButton* newChoreBtn = new RoundedButton(choresPanel, wxID_ANY, "New Chore", Palette::Teal);
      RoundedButton* deleteChoreBtn = new RoundedButton(choresPanel, wxID_ANY, "Delete Selected", Palette::Coral);
      RoundedButton* modifyChoreBtn = new RoundedButton(choresPanel, wxID_ANY, "Modify Selected", Palette::Purple);
      choreBtnSizer->Add(newChoreBtn, 0, wxALL, 4);
      choreBtnSizer->Add(deleteChoreBtn, 0, wxALL, 4);
      choreBtnSizer->Add(modifyChoreBtn, 0, wxALL, 4);
      choresSizer->Add(choreBtnSizer, 0, wxALIGN_LEFT | wxLEFT | wxBOTTOM, 8);

      choresPanel->SetSizer(choresSizer);
      notebook->AddPage(choresPanel, "Chores");

      choresList->Bind(wxEVT_LIST_ITEM_SELECTED, &MainFrame::OnChoreSelected, this);
      newChoreBtn->Bind(wxEVT_BUTTON, &MainFrame::OnNewChore, this);
      deleteChoreBtn->Bind(wxEVT_BUTTON, &MainFrame::OnDeleteChore, this);
      modifyChoreBtn->Bind(wxEVT_BUTTON, &MainFrame::OnModifyChore, this);
    }

    void BuildChoreDoersTab() {
      wxPanel* doersPanel = new wxPanel(notebook);
      doersPanel->SetBackgroundColour(Palette::Background);
      wxBoxSizer* rootSizer = new wxBoxSizer(wxHORIZONTAL);

      // Left: scrollable column of doer cards + Add/Delete/Edit-Profile actions.
      wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
      leftSizer->Add(MakeSectionTitle(doersPanel, "Chore Doers"), 0, wxLEFT | wxTOP, 8);
      CardPanel* doerListCard = new CardPanel(doersPanel);
      doerCardsScroll = new wxScrolledWindow(doerListCard, wxID_ANY, wxDefaultPosition, wxSize(260, -1), wxVSCROLL | wxBORDER_NONE);
      doerCardsScroll->SetBackgroundColour(Palette::CardBg);
      doerCardsScroll->SetScrollRate(0, 10);
      doerCardsSizer = new wxBoxSizer(wxVERTICAL);
      doerCardsScroll->SetSizer(doerCardsSizer);
      doerListCard->GetInnerSizer()->Add(doerCardsScroll, 1, wxALL | wxEXPAND, 10);
      leftSizer->Add(doerListCard, 1, wxALL | wxEXPAND, 4);

      wxBoxSizer* doerBtnSizer = new wxBoxSizer(wxHORIZONTAL);
      RoundedButton* addDoerBtn = new RoundedButton(doersPanel, wxID_ANY, "Add", Palette::Teal, wxSize(70, 32));
      RoundedButton* deleteDoerBtn = new RoundedButton(doersPanel, wxID_ANY, "Delete", Palette::Coral, wxSize(70, 32));
      RoundedButton* editProfileBtn = new RoundedButton(doersPanel, wxID_ANY, "Edit", Palette::Purple, wxSize(70, 32));
      doerBtnSizer->Add(addDoerBtn, 0, wxALL, 4);
      doerBtnSizer->Add(deleteDoerBtn, 0, wxALL, 4);
      doerBtnSizer->Add(editProfileBtn, 0, wxALL, 4);
      leftSizer->Add(doerBtnSizer, 0, wxALIGN_LEFT | wxLEFT, 4);
      rootSizer->Add(leftSizer, 0, wxEXPAND);

      // Right: profile panel for the selected doer + their assigned chores, in its own card.
      CardPanel* profileCard = new CardPanel(doersPanel);
      doerProfilePanel = new wxPanel(profileCard);
      doerProfilePanel->SetBackgroundColour(Palette::CardBg);
      wxBoxSizer* profileSizer = new wxBoxSizer(wxVERTICAL);

      wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);
      profileAvatar = new AvatarCircle(doerProfilePanel, wxID_ANY, wxColour(200, 200, 200), "?", wxSize(72, 72));
      headerSizer->Add(profileAvatar, 0, wxALL | wxALIGN_CENTER_VERTICAL, 8);

      wxBoxSizer* headerTextSizer = new wxBoxSizer(wxVERTICAL);
      profileNameText = new wxStaticText(doerProfilePanel, wxID_ANY, "Select a chore doer");
      wxFont nameFont = profileNameText->GetFont();
      nameFont.SetPointSize(nameFont.GetPointSize() + 4);
      nameFont.SetWeight(wxFONTWEIGHT_BOLD);
      profileNameText->SetFont(nameFont);
      profileNameText->SetForegroundColour(Palette::TextPrimary);
      headerTextSizer->Add(profileNameText);
      profileStreakText = new wxStaticText(doerProfilePanel, wxID_ANY, "");
      profileStreakText->SetForegroundColour(Palette::TextMuted);
      headerTextSizer->Add(profileStreakText);
      profileEarningsText = new wxStaticText(doerProfilePanel, wxID_ANY, "");
      profileEarningsText->SetForegroundColour(Palette::TextMuted);
      headerTextSizer->Add(profileEarningsText);
      headerSizer->Add(headerTextSizer, 1, wxALIGN_CENTER_VERTICAL | wxALL, 4);
      profileSizer->Add(headerSizer, 0, wxEXPAND);

      wxStaticText* notesLabel = new wxStaticText(doerProfilePanel, wxID_ANY, "Notes:");
      notesLabel->SetForegroundColour(Palette::TextMuted);
      profileSizer->Add(notesLabel, 0, wxLEFT | wxTOP, 8);
      profileNotesText = new wxTextCtrl(doerProfilePanel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 60), wxTE_MULTILINE | wxTE_READONLY);
      profileSizer->Add(profileNotesText, 0, wxALL | wxEXPAND, 8);

      doerChoresList = new wxListCtrl(doerProfilePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
      doerChoresList->InsertColumn(0, "ID", wxLIST_FORMAT_LEFT, 40);
      doerChoresList->InsertColumn(1, "Name", wxLIST_FORMAT_LEFT, 200);
      doerChoresList->InsertColumn(2, "Earnings", wxLIST_FORMAT_LEFT, 70);
      doerChoresList->InsertColumn(3, "Status", wxLIST_FORMAT_LEFT, 100);
      profileSizer->Add(doerChoresList, 1, wxALL | wxEXPAND, 8);

      wxBoxSizer* actionBtnSizer = new wxBoxSizer(wxHORIZONTAL);
      startBtn = new RoundedButton(doerProfilePanel, wxID_ANY, "Start", Palette::Blue, wxSize(90, 34));
      completeBtn = new RoundedButton(doerProfilePanel, wxID_ANY, "Complete", Palette::Teal, wxSize(90, 34));
      resetBtn = new RoundedButton(doerProfilePanel, wxID_ANY, "Reset", Palette::Amber, wxSize(90, 34));
      startBtn->Disable();
      completeBtn->Disable();
      resetBtn->Disable();
      actionBtnSizer->Add(startBtn, 0, wxALL, 4);
      actionBtnSizer->Add(completeBtn, 0, wxALL, 4);
      actionBtnSizer->Add(resetBtn, 0, wxALL, 4);
      profileSizer->Add(actionBtnSizer, 0, wxALIGN_LEFT | wxLEFT, 4);

      doerProfilePanel->SetSizer(profileSizer);
      profileCard->GetInnerSizer()->Add(doerProfilePanel, 1, wxALL | wxEXPAND, 10);
      rootSizer->Add(profileCard, 1, wxEXPAND | wxALL, 4);

      doersPanel->SetSizer(rootSizer);
      notebook->AddPage(doersPanel, "Chore Doers");

      doerCardsScroll->Bind(wxEVT_CONTEXT_MENU, &MainFrame::OnDoerContextMenu, this);
      Bind(EVT_DOER_CARD_SELECTED, &MainFrame::OnDoerCardSelected, this);
      doerChoresList->Bind(wxEVT_LIST_ITEM_SELECTED, &MainFrame::OnDoerChoreSelectionChanged, this);
      doerChoresList->Bind(wxEVT_LIST_ITEM_DESELECTED, &MainFrame::OnDoerChoreSelectionChanged, this);
      addDoerBtn->Bind(wxEVT_BUTTON, &MainFrame::OnAddChoreDoer, this);
      deleteDoerBtn->Bind(wxEVT_BUTTON, &MainFrame::OnDeleteChoreDoer, this);
      editProfileBtn->Bind(wxEVT_BUTTON, &MainFrame::OnEditDoerProfile, this);
      startBtn->Bind(wxEVT_BUTTON, &MainFrame::OnStartChore, this);
      completeBtn->Bind(wxEVT_BUTTON, &MainFrame::OnCompleteChore, this);
      resetBtn->Bind(wxEVT_BUTTON, &MainFrame::OnResetChore, this);
    }

    void BuildHistoryTab() {
      wxPanel* historyPanel = new wxPanel(notebook);
      historyPanel->SetBackgroundColour(Palette::Background);
      wxBoxSizer* historySizer = new wxBoxSizer(wxVERTICAL);

      wxBoxSizer* navSizer = new wxBoxSizer(wxHORIZONTAL);
      RoundedButton* prevBtn = new RoundedButton(historyPanel, ID_PREV_DAY, "< Prev Day", Palette::Blue, wxSize(110, 32));
      RoundedButton* todayBtn = new RoundedButton(historyPanel, ID_TODAY_DAY, "Today", Palette::Teal, wxSize(80, 32));
      RoundedButton* nextBtn = new RoundedButton(historyPanel, ID_NEXT_DAY, "Next Day >", Palette::Blue, wxSize(110, 32));
      navSizer->Add(prevBtn, 0, wxALL, 4);
      navSizer->Add(todayBtn, 0, wxALL, 4);
      navSizer->Add(nextBtn, 0, wxALL, 4);
      historyDateLabel = new wxStaticText(historyPanel, wxID_ANY, "");
      wxFont dateFont = historyDateLabel->GetFont();
      dateFont.SetPointSize(dateFont.GetPointSize() + 3);
      dateFont.SetWeight(wxFONTWEIGHT_BOLD);
      historyDateLabel->SetFont(dateFont);
      historyDateLabel->SetForegroundColour(Palette::TextPrimary);
      navSizer->Add(historyDateLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 8);
      historySizer->Add(navSizer, 0, wxALIGN_LEFT | wxLEFT | wxTOP, 8);

      historySizer->Add(MakeSectionTitle(historyPanel, "Activity Log"), 0, wxLEFT, 12);
      CardPanel* historyCard = new CardPanel(historyPanel);
      historyList = new wxListCtrl(historyCard, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
      historyList->InsertColumn(0, "Time", wxLIST_FORMAT_LEFT, 90);
      historyList->InsertColumn(1, "Doer", wxLIST_FORMAT_LEFT, 140);
      historyList->InsertColumn(2, "Action", wxLIST_FORMAT_LEFT, 100);
      historyList->InsertColumn(3, "Chore", wxLIST_FORMAT_LEFT, 200);
      historyList->InsertColumn(4, "Earnings", wxLIST_FORMAT_LEFT, 80);
      historyCard->GetInnerSizer()->Add(historyList, 1, wxALL | wxEXPAND, 12);
      historySizer->Add(historyCard, 1, wxALL | wxEXPAND, 8);

      historyPanel->SetSizer(historySizer);
      notebook->AddPage(historyPanel, "History");

      prevBtn->Bind(wxEVT_BUTTON, &MainFrame::OnPrevDay, this);
      todayBtn->Bind(wxEVT_BUTTON, &MainFrame::OnToday, this);
      nextBtn->Bind(wxEVT_BUTTON, &MainFrame::OnNextDay, this);
    }

    void RefreshHistoryList() {
      historyDateLabel->SetLabel(FormatDateForDisplay(currentHistoryDate));
      historyList->DeleteAllItems();
      auto events = manager->getEventsForDate(currentHistoryDate);
      for (const auto& event : events) {
        // timestamp is "YYYY-MM-DD HH:MM:SS" — show just the time portion.
        wxString time = event.timestamp.size() >= 19 ? wxString(event.timestamp.substr(11, 8)) : wxString(event.timestamp);
        long row = historyList->InsertItem(historyList->GetItemCount(), time);
        historyList->SetItem(row, 1, event.doerName);
        historyList->SetItem(row, 2, event.action);
        historyList->SetItem(row, 3, event.choreName);
        historyList->SetItem(row, 4, "$" + to_string(event.earnings));
        historyList->SetItemBackgroundColour(row, row % 2 == 0 ? Palette::CardBg : Palette::RowAlt);
      }
    }

    void OnPrevDay(wxCommandEvent&) {
      currentHistoryDate = AddDaysToDateString(currentHistoryDate, -1);
      RefreshHistoryList();
    }

    void OnNextDay(wxCommandEvent&) {
      currentHistoryDate = AddDaysToDateString(currentHistoryDate, 1);
      RefreshHistoryList();
    }

    void OnToday(wxCommandEvent&) {
      currentHistoryDate = TodayDateString();
      RefreshHistoryList();
    }

    void RefreshChoresList() {
      long selectedId = -1;
      long sel = GetFirstSelectedItem(choresList);
      if (sel != -1) selectedId = (long)choresList->GetItemData(sel);

      choresList->DeleteAllItems();
      for (const auto& chore : manager->getChores().item()) {
        long row = choresList->InsertItem(choresList->GetItemCount(), to_string(chore->getId()));
        choresList->SetItem(row, 1, chore->getName());
        choresList->SetItem(row, 2, chore->getCategory());
        choresList->SetItem(row, 3, to_string(chore->getEarnings()));
        choresList->SetItem(row, 4, chore->toStringS(chore->getStatus()));
        choresList->SetItem(row, 5, chore->toStringP(chore->getPriority()));
        choresList->SetItem(row, 6, chore->getFrequency());
        choresList->SetItemData(row, chore->getId());
        choresList->SetItemBackgroundColour(row, row % 2 == 0 ? Palette::CardBg : Palette::RowAlt);
        if (chore->getId() == selectedId) {
          choresList->SetItemState(row, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        }
      }
    }

    // Rebuilds the doer cards from scratch (mirrors the DeleteAllItems()-then-repopulate
    // idiom used for the native lists elsewhere in this file). The selected card's
    // highlight is explicitly re-applied by name after rebuild — otherwise it would
    // visibly flicker away after almost every action, since nearly every button click
    // triggers a save-and-refresh.
    void RefreshChoreDoersList() {
      doerCardsSizer->Clear(true); // true = also destroy the child windows
      doerCards.clear();

      for (const auto& doer : manager->getChoreDoers().item()) {
        wxColour avatarColor(doer->getAvatarColor().empty() ? "#4ECDC4" : doer->getAvatarColor());
        wxString streakText = wxString::Format("%d-day streak", manager->getDoerStreak(doer->getId()));
        wxString earningsText = wxString::Format("$%d earned", doer->getTotalEarnings());
        DoerCardPanel* card = new DoerCardPanel(doerCardsScroll, wxID_ANY, doer->getName(), avatarColor, streakText, earningsText);
        card->SetSelected(doer->getName() == selectedDoerName.ToStdString());
        doerCardsSizer->Add(card, 0, wxALL | wxEXPAND, 4);
        doerCards.push_back(card);
      }
      doerCardsScroll->FitInside();
      doerCardsScroll->Layout();

      if (!selectedDoerName.IsEmpty() && manager->findChoreDoerByName(selectedDoerName.ToStdString())) {
        RefreshDoerProfilePanel(selectedDoerName);
      }
      else {
        selectedDoerName.Clear();
        RefreshDoerProfilePanel(selectedDoerName);
      }
    }

    // Updates the profile panel (avatar/name/streak/earnings/notes) and the assigned-
    // chores sub-list for whichever doer is currently selected; clears it when none is.
    void RefreshDoerProfilePanel(const wxString& doerName) {
      auto doer = doerName.IsEmpty() ? nullptr : manager->findChoreDoerByName(doerName.ToStdString());
      if (!doer) {
        profileAvatar->SetAvatarColor(wxColour(200, 200, 200));
        profileAvatar->SetInitial("?");
        profileNameText->SetLabel("Select a chore doer");
        profileStreakText->SetLabel("");
        profileEarningsText->SetLabel("");
        profileNotesText->SetValue("");
        doerChoresList->DeleteAllItems();
        UpdateDoerActionButtons();
        return;
      }

      wxColour avatarColor(doer->getAvatarColor().empty() ? "#4ECDC4" : doer->getAvatarColor());
      profileAvatar->SetAvatarColor(avatarColor);
      profileAvatar->SetInitial(doer->getName().empty() ? wxString("?") : wxString(doer->getName()).Left(1).Upper());
      profileNameText->SetLabel(doer->getName());
      profileStreakText->SetLabel(wxString::Format("%d-day streak", manager->getDoerStreak(doer->getId())));
      profileEarningsText->SetLabel(wxString::Format("$%d earned • %d chores assigned", doer->getTotalEarnings(), doer->getChoreAmount()));
      profileNotesText->SetValue(doer->getNotes());
      doerProfilePanel->Layout();

      RefreshDoerSubList(doerName);
    }

    void RefreshDoerSubList(const wxString& doerName) {
      doerChoresList->DeleteAllItems();
      auto doer = manager->findChoreDoerByName(doerName.ToStdString());
      if (!doer) return;
      for (const auto& chore : doer->assignedChores.item()) {
        long row = doerChoresList->InsertItem(doerChoresList->GetItemCount(), to_string(chore->getId()));
        doerChoresList->SetItem(row, 1, chore->getName());
        doerChoresList->SetItem(row, 2, to_string(chore->getEarnings()));
        doerChoresList->SetItem(row, 3, chore->toStringS(chore->getStatus()));
        doerChoresList->SetItemBackgroundColour(row, row % 2 == 0 ? Palette::CardBg : Palette::RowAlt);
      }
      UpdateDoerActionButtons();
    }

    void UpdateDoerActionButtons() {
      bool enable = !selectedDoerName.IsEmpty() && GetFirstSelectedItem(doerChoresList) != -1;
      startBtn->Enable(enable);
      completeBtn->Enable(enable);
      resetBtn->Enable(enable);
    }

    void OnChoreSelected(wxListEvent& event) {
      int choreId = (int)choresList->GetItemData(event.GetIndex());
      auto chore = manager->findChoreById(choreId);
      if (chore) {
        choreDetailText->SetValue(chore->PrettyPrintClassAttributes());
      }
    }

    void OnNewChore(wxCommandEvent&) {
      ChoreEditorDialog dlg(this, *manager, nullptr);
      if (dlg.ShowModal() == wxID_OK) {
        manager->saveData();
        RefreshChoresList();
        SetStatusText("Chore added.");
      }
    }

    void OnDeleteChore(wxCommandEvent&) {
      long sel = GetFirstSelectedItem(choresList);
      if (sel == -1) {
        wxMessageBox("Select a chore first.", "No Selection", wxOK | wxICON_WARNING, this);
        return;
      }
      int choreId = (int)choresList->GetItemData(sel);
      if (manager->deleteChoreFromAvailable(choreId)) {
        manager->saveData();
        RefreshChoresList();
        SetStatusText("Chore deleted.");
      }
      else {
        wxMessageBox("Chore not found.", "Delete Failed", wxOK | wxICON_ERROR, this);
      }
    }

    void OnModifyChore(wxCommandEvent&) {
      long sel = GetFirstSelectedItem(choresList);
      if (sel == -1) {
        wxMessageBox("Select a chore first.", "No Selection", wxOK | wxICON_WARNING, this);
        return;
      }
      int choreId = (int)choresList->GetItemData(sel);
      auto chore = manager->findChoreById(choreId);
      if (!chore) return;
      ChoreEditorDialog dlg(this, *manager, chore);
      if (dlg.ShowModal() == wxID_OK) {
        manager->saveData();
        RefreshChoresList();
        SetStatusText("Chore updated.");
      }
    }

    void OnManageCategories(wxCommandEvent&) {
      ManageCategoriesDialog dlg(this, *manager);
      dlg.ShowModal();
      manager->saveData();
      RefreshChoresList();
    }

    void OnDoerCardSelected(wxCommandEvent& event) {
      selectedDoerName = event.GetString();
      for (auto* card : doerCards) {
        card->SetSelected(card->GetDoerName() == selectedDoerName);
      }
      RefreshDoerProfilePanel(selectedDoerName);
    }

    void OnDoerChoreSelectionChanged(wxListEvent&) {
      UpdateDoerActionButtons();
    }

    void OnAddChoreDoer(wxCommandEvent&) {
      wxString name = wxGetTextFromUser("Enter chore doer's name:", "Add Chore Doer", "", this);
      if (name.IsEmpty()) return;
      manager->addChoreDoer(name.ToStdString());
      manager->saveData();
      RefreshChoreDoersList();
      SetStatusText("Chore doer " + name + " added.");
    }

    void OnEditDoerProfile(wxCommandEvent&) {
      if (selectedDoerName.IsEmpty()) {
        wxMessageBox("Select a chore doer first.", "No Selection", wxOK | wxICON_WARNING, this);
        return;
      }
      auto doer = manager->findChoreDoerByName(selectedDoerName.ToStdString());
      if (!doer) return;
      DoerProfileEditDialog dlg(this, *doer);
      if (dlg.ShowModal() == wxID_OK) {
        manager->saveData();
        RefreshChoreDoersList();
        SetStatusText("Profile updated.");
      }
    }

    void OnDeleteChoreDoer(wxCommandEvent&) {
      if (selectedDoerName.IsEmpty()) {
        wxMessageBox("Select a chore doer first.", "No Selection", wxOK | wxICON_WARNING, this);
        return;
      }
      wxString doerName = selectedDoerName;
      auto doer = manager->findChoreDoerByName(doerName.ToStdString());
      if (doer && doer->getChoreAmount() > 0) {
        int confirm = wxMessageBox("'" + doerName + "' has " + to_string(doer->getChoreAmount()) + " assigned chore(s). Delete anyway?",
          "Confirm Delete", wxYES_NO | wxICON_WARNING, this);
        if (confirm != wxYES) return;
      }
      if (manager->deleteChoreDoer(doerName.ToStdString())) {
        if (doerName == selectedDoerName) selectedDoerName.Clear();
        manager->saveData();
        RefreshChoreDoersList();
        SetStatusText("Chore doer " + doerName + " deleted.");
      }
    }

    void RunDoerChoreAction(int action) {
      long sel = GetFirstSelectedItem(doerChoresList);
      if (sel == -1 || selectedDoerName.IsEmpty()) return;
      int choreId = wxAtoi(doerChoresList->GetItemText(sel, 0));
      string doerName = selectedDoerName.ToStdString();
      string result;
      switch (action) {
      case 0: result = manager->startDoerChore(doerName, choreId); break;
      case 1: result = manager->completeDoerChore(doerName, choreId); break;
      case 2: result = manager->resetDoerChore(doerName, choreId); break;
      }
      manager->saveData();
      RefreshDoerProfilePanel(selectedDoerName); // streak may have changed on completion
      RefreshChoresList();
      RefreshHistoryList();
      wxMessageBox(result, "Chore Status Updated", wxOK | wxICON_INFORMATION, this);
    }

    void OnStartChore(wxCommandEvent&) { RunDoerChoreAction(0); }
    void OnCompleteChore(wxCommandEvent&) { RunDoerChoreAction(1); }
    void OnResetChore(wxCommandEvent&) { RunDoerChoreAction(2); }

    void OnDoerContextMenu(wxContextMenuEvent&) {
      if (selectedDoerName.IsEmpty()) return;
      wxMenu menu;
      menu.Append(ID_DOER_SORT_ID, "Sort by ID");
      menu.Append(ID_DOER_SORT_NAME, "Sort by Name");
      menu.Append(ID_DOER_SORT_EARNINGS, "Sort by Earnings");
      menu.Append(ID_DOER_SORT_CATEGORY, "Sort by Category");
      menu.Bind(wxEVT_MENU, &MainFrame::OnDoerSort, this, ID_DOER_SORT_ID, ID_DOER_SORT_CATEGORY);
      PopupMenu(&menu);
    }

    void OnDoerSort(wxCommandEvent& event) {
      auto doer = manager->findChoreDoerByName(selectedDoerName.ToStdString());
      if (!doer) return;
      switch (event.GetId()) {
      case ID_DOER_SORT_ID: doer->sortAssignedChoresByID(); break;
      case ID_DOER_SORT_NAME: doer->sortAssignedChoresByName(); break;
      case ID_DOER_SORT_EARNINGS: doer->sortAssignedChoresByEarnings(); break;
      case ID_DOER_SORT_CATEGORY: doer->sortAssignedChoresByCategory(manager->getCategoryRegistry()); break;
      }
      manager->saveData();
      RefreshDoerSubList(selectedDoerName);
    }

    void OnOutputToFile(wxCommandEvent&) {
      wxFileDialog dlg(this, "Output Chore Assignments", GetTestDataDir(), "assigned_chores.json",
        "JSON files (*.json)|*.json", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
      if (dlg.ShowModal() != wxID_OK) return;
      try {
        string result = manager->outputChoreAssignmentsToFile(dlg.GetPath().ToStdString());
        if (result.empty()) {
          SetStatusText("Chore assignments written to " + dlg.GetPath());
        }
        else {
          wxMessageBox(result, "Nothing Written", wxOK | wxICON_WARNING, this);
        }
      }
      catch (const exception& e) {
        wxMessageBox(e.what(), "Error Writing File", wxOK | wxICON_ERROR, this);
      }
    }

    void OnSave(wxCommandEvent&) {
      manager->saveData();
      SetStatusText("Saved.");
    }

    void OnExit(wxCommandEvent&) {
      Close(true);
    }

    void OnAssignRandom(wxCommandEvent&) {
      string result = manager->assignChoresRandomly();
      manager->saveData();
      RefreshChoreDoersList();
      RefreshHistoryList();
      if (result.empty()) {
        SetStatusText("Chores assigned randomly.");
      }
      else {
        wxMessageBox(result, "Assign Chores", wxOK | wxICON_WARNING, this);
      }
    }

    void OnSearch(wxCommandEvent&) {
      SearchDialog dlg(this, *manager);
      dlg.ShowModal();
    }

    void OnCompare(wxCommandEvent&) {
      CompareChoresDialog dlg(this);
      if (dlg.ShowModal() == wxID_OK) {
        string result = manager->compareChores(dlg.GetId1(), dlg.GetId2());
        wxMessageBox(result, "Compare Chores", wxOK | wxICON_INFORMATION, this);
      }
    }

    void OnSortChores(wxCommandEvent& event) {
      bool descending = GetMenuBar()->IsChecked(ID_SORT_DESC);
      switch (event.GetId()) {
      case ID_SORT_ID: manager->sortChoresByID(!descending); break;
      case ID_SORT_NAME: manager->sortChoresByName(!descending); break;
      case ID_SORT_EARNINGS: manager->sortChoresByEarnings(!descending); break;
      case ID_SORT_CATEGORY: manager->sortChoresByCategory(!descending); break;
      }
      manager->saveData();
      RefreshChoresList();
    }

    void OnViewDetails(wxCommandEvent&) {
      ShowReadOnlyTextDialog("All Chores - Full Details", manager->displayAllChoresAllAttributes());
    }

    void OnViewAssignments(wxCommandEvent&) {
      ShowReadOnlyTextDialog("All Chore Doer Assignments", manager->displayAllChoreAssignments());
    }

    void ShowReadOnlyTextDialog(const wxString& title, const string& content) {
      wxDialog dlg(this, wxID_ANY, title, wxDefaultPosition, wxSize(600, 500));
      wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
      wxTextCtrl* text = new wxTextCtrl(&dlg, wxID_ANY, content, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
      sizer->Add(text, 1, wxALL | wxEXPAND, 8);
      wxButton* closeBtn = new wxButton(&dlg, wxID_OK, "Close");
      sizer->Add(closeBtn, 0, wxALL | wxALIGN_CENTER, 8);
      dlg.SetSizer(sizer);
      dlg.ShowModal();
    }

    void OnSortAllDoers(wxCommandEvent& event) {
      switch (event.GetId()) {
      case ID_SORTALL_ID: manager->sortAllChoreDoersChoresByID(); break;
      case ID_SORTALL_NAME: manager->sortAllChoreDoersChoresByName(); break;
      case ID_SORTALL_EARNINGS: manager->sortAllChoreDoersChoresByEarnings(); break;
      case ID_SORTALL_CATEGORY: manager->sortAllChoreDoersChoresByCategory(); break;
      }
      manager->saveData();
      if (!selectedDoerName.IsEmpty()) RefreshDoerSubList(selectedDoerName);
    }

    void OnModifyProfile(wxCommandEvent&) {
      ProfileDialog dlg(this, manager->getClient());
      if (dlg.ShowModal() == wxID_OK) {
        manager->saveData();
        SetStatusText("Profile updated.");
      }
    }

    void SwitchToHousehold(const string& filePath) {
      if (manager) manager->saveData();
      unique_ptr<ChoreManager> newManager;
      try {
        newManager = make_unique<ChoreManager>(filePath);
      }
      catch (const exception& e) {
        wxMessageBox(wxString("Failed to load household:\n") + e.what(), "Error", wxOK | wxICON_ERROR, this);
        return;
      }
      manager = std::move(newManager);
      householdRegistry.setLastOpenHousehold(filePath);
      selectedDoerName.Clear();
      choreDetailText->Clear();
      currentHistoryDate = TodayDateString();
      UpdateTitle();
      RefreshChoresList();
      RefreshChoreDoersList();
      RefreshHistoryList();
      SetStatusText("Switched household.");
    }

    void OnSwitchHousehold(wxCommandEvent&) {
      HouseholdPickerDialog dlg(this, householdRegistry, householdRegistry.getLastOpenHouseholdPath());
      if (dlg.ShowModal() == wxID_OK) {
        string path = dlg.GetSelectedPath();
        if (!path.empty() && path != householdRegistry.getLastOpenHouseholdPath()) {
          SwitchToHousehold(path);
        }
      }
    }

    void OnNewHousehold(wxCommandEvent&) {
      wxString name = wxGetTextFromUser("Enter new household's name:", "New Household", "", this);
      if (name.IsEmpty()) return;
      string errorMsg;
      string path = householdRegistry.createHousehold(name.ToStdString(), errorMsg);
      if (path.empty()) {
        wxMessageBox(errorMsg, "Could Not Create Household", wxOK | wxICON_ERROR, this);
        return;
      }
      int confirm = wxMessageBox("Household '" + name + "' created. Switch to it now?", "New Household", wxYES_NO | wxICON_QUESTION, this);
      if (confirm == wxYES) {
        SwitchToHousehold(path);
      }
    }

    void OnManageHouseholds(wxCommandEvent&) {
      ManageHouseholdsDialog dlg(this, householdRegistry, *manager);
      dlg.ShowModal();
      UpdateTitle();
    }
  };

  class ChoreManagerApp : public wxApp {
  private:
    unique_ptr<HouseholdRegistry> householdRegistry;

  public:
    bool OnInit() override {
      if (!wxApp::OnInit()) return false;

      try {
        householdRegistry = make_unique<HouseholdRegistry>(
          GetHouseholdsDir(), GetAppStatePath(), GetTestDataFilePath("data.json"));
      }
      catch (const exception& e) {
        wxMessageBox(wxString("Failed to initialize household storage:\n") + e.what(), "Startup Error", wxOK | wxICON_ERROR);
        return false;
      }

      string chosenPath = householdRegistry->getLastOpenHouseholdPath();
      if (householdRegistry->listHouseholds().size() > 1) {
        HouseholdPickerDialog dlg(nullptr, *householdRegistry, chosenPath);
        if (dlg.ShowModal() != wxID_OK) return false;
        chosenPath = dlg.GetSelectedPath();
      }

      if (chosenPath.empty()) {
        wxMessageBox("No household could be opened.", "Startup Error", wxOK | wxICON_ERROR);
        return false;
      }

      unique_ptr<ChoreManager> manager;
      try {
        manager = make_unique<ChoreManager>(chosenPath);
      }
      catch (const exception& e) {
        wxMessageBox(wxString("Failed to load chore data:\n") + e.what(), "Startup Error", wxOK | wxICON_ERROR);
        return false;
      }

      householdRegistry->setLastOpenHousehold(chosenPath);

      MainFrame* frame = new MainFrame(std::move(manager), *householdRegistry);
      frame->Show(true);
      return true;
    }
  };
}

wxIMPLEMENT_APP(ChoreApp::ChoreManagerApp);
