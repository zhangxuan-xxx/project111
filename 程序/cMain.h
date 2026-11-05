#pragma once

#include <wx/wx.h>
#include <wx/treectrl.h>

#include "DrawBoard.h"
#include "ComponentLibrary.h"
#include "CommandIds.h"
#include "Netlist.h"
class cMain : public wxFrame
{
public:
    cMain();
    ~cMain();

    wxTreeCtrl* m_componentTree = nullptr;
    DrawBoard* drawborad = nullptr;

    // 供命令行 / 双击文件打开图纸
    bool OpenDocument(const wxString& path);

private:
    wxPanel* m_leftPanel = nullptr;

    void CreateStatusBarFields();
    void CreateMenuBar();
    void CreateAppToolBar();
    void CreateLeftPanel();
    void PopulateComponentTree();
    void OnLibraryItemActivated(wxTreeEvent& evt);

    // 左栏元件库 -> 画布放置模式
    void PlaceGate(LibraryComponentType type);
    void PlaceCustomComponent(
        const CustomGateDefinition& definition);
    void OnManageLibrary(wxCommandEvent& evt);

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

    // -------- 文件：打开 / 保存 / 导出 --------
    void OnOpenFile(wxCommandEvent& evt);
    void OnImportDgate(wxCommandEvent& evt);
    void OnSaveFile(wxCommandEvent& evt);
    void OnSaveAsFile(wxCommandEvent& evt);
    void OnExportPng(wxCommandEvent& evt);
    void OnExportVerilog(wxCommandEvent& evt);
    void OnExportBlif(wxCommandEvent& evt);
    void OnClose(wxCloseEvent& evt);

    void OpenWithDialog(const wxString& title);
    bool LoadFromFile(const wxString& path);
    bool SaveToFile(const wxString& path);
    bool DoSave(bool forceDialog = false);
    bool DoSaveAs();
    bool MaybeSaveChanges();
    void SetDirty(bool dirty);
    void UpdateTitle();
    void SyncViewState();
    void ShowExportSummary(const wxString& kind,
                           const NetlistSummary& summary,
                           const wxString& path);
    wxString DefaultFileName(const wxString& extension) const;

    // 当前工程文件；为空表示「未命名」
    wxString m_currentPath;
    bool m_dirty = false;

    void BindCommand(int id, void (cMain::*handler)(wxCommandEvent&));
    void SetToolText(const wxString& toolName);
    void UpdateToolUi(int activeCommandId);
};
