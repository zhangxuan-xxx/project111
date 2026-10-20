#pragma once

#include <wx/wx.h>
#include <wx/listbox.h>

#include "DrawBoard.h"
#include "ComponentLibrary.h"
#include "CommandIds.h"
class cMain : public wxFrame
{
public:
    cMain();
    ~cMain();

    wxListBox* m_list1 = nullptr;
    DrawBoard* drawborad = nullptr;

    void OnLibraryItemDoubleClick(wxCommandEvent& evt);

private:
    wxPanel* m_leftPanel = nullptr;

    void CreateStatusBarFields();
    void CreateMenuBar();
    void CreateAppToolBar();
    void CreateLeftPanel();

    // 左栏元件库 -> 画布放置模式
    void PlaceGate(LibraryComponentType type);

    // 统一的“尚未实现”占位处理，只在状态栏给一条提示
    void OnPlaceholder(wxCommandEvent& evt);

    void OnExit(wxCommandEvent& evt);
    void OnDelete(wxCommandEvent& evt);
    void OnPlaceComponent(wxCommandEvent& evt);
    void OnPlaceWire(wxCommandEvent& evt);
    void OnSelectTool(wxCommandEvent& evt);
    void OnShowShortcuts(wxCommandEvent& evt);
    void OnShowManual(wxCommandEvent& evt);
    void OnNewDocument(wxCommandEvent& evt);
    void OnToggleGrid(wxCommandEvent& evt);
    void OnToggleSnap(wxCommandEvent& evt);
    void OnSetGridSize(wxCommandEvent& evt);
    void OnPlaceText(wxCommandEvent& evt);
    void OnProperties(wxCommandEvent& evt);
    void OnAbout(wxCommandEvent& evt);

    void BindCommand(int id, void (cMain::*handler)(wxCommandEvent&));
    void SetToolText(const wxString& toolName);
    void UpdateToolUi(int activeCommandId);
};
