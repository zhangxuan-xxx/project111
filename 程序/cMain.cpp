#include "cMain.h"

#include <wx/listctrl.h>
#include <wx/statline.h>
#include <wx/treectrl.h>

#include "DrawBoard.h"
#include "ComponentLibrary.h"
#include "IconFactory.h"

namespace
{
    class ComponentTreeItemData : public wxTreeItemData
    {
    public:
        explicit ComponentTreeItemData(LibraryComponentType componentType)
            : type(componentType)
        {
        }

        LibraryComponentType type;
    };
}

// ============================================================
// 把 GateType 翻译成中文名称（属性对话框用）
// ============================================================
static wxString GateTypeName(GateType type)
{
    switch (type)
    {
    case AND:       return wxT("与门 (AND)");
    case OR:        return wxT("或门 (OR)");
    case NOT:       return wxT("非门 (NOT)");
    case NAND_GATE: return wxT("与非门 (NAND)");
    case XOR_GATE:  return wxT("异或门 (XOR)");
    case NOR_GATE:  return wxT("或非门 (NOR)");
    case XNOR_GATE: return wxT("同或门 (XNOR)");
    default:        return wxT("逻辑门");
    }
}

// ============================================================
// 构造：先建状态栏，再建菜单和工具栏，最后拼左右布局
// ============================================================
cMain::cMain()
    : wxFrame(
        nullptr,
        wxID_ANY,
        wxT("DrawGate - 逻辑门绘图"),
        wxDefaultPosition,
        wxSize(1120, 720)
    )
{
    CreateStatusBarFields();
    CreateMenuBar();
    CreateAppToolBar();
    CreateLeftPanel();

    drawborad = new DrawBoard(this);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->Add(m_leftPanel, 0, wxEXPAND);
    mainSizer->Add(drawborad, 1, wxEXPAND);

    SetSizer(mainSizer);
    SetMinSize(wxSize(900, 560));
    Centre();

    // 画布坐标实时显示在状态栏第 3 格
    drawborad->Bind(
        wxEVT_MOTION,
        [this](wxMouseEvent& event)
        {
            SetStatusText(
                wxString::Format(
                    wxT("X: %d    Y: %d"),
                    event.GetX(),
                    event.GetY()),
                2);

            event.Skip();
        });

    SetToolText(wxT("选择"));

    // Esc 取消操作后，画布会通知主窗口刷新状态栏
    Bind(
        EVT_DRAWBOARD_TOOL_CHANGED,
        [this](wxCommandEvent& event)
        {
            SetToolText(wxT("选择"));
            SetStatusText(
                event.GetInt() == 1 ? wxT("已取消当前操作") : wxT("就绪"),
                0);

            UpdateToolUi(ID_TOOL_SELECT);
        });
}

cMain::~cMain()
{
}

// ============================================================
// 状态栏：就绪提示 / 当前工具 / 坐标 / 缩放
// ============================================================
void cMain::CreateStatusBarFields()
{
    CreateStatusBar(4);

    const int widths[4] = { -1, 150, 190, 130 };
    SetStatusWidths(4, widths);

    SetStatusText(wxT("就绪"), 0);
    SetStatusText(wxT("工具: 选择"), 1);
    SetStatusText(wxT("X: 0    Y: 0"), 2);
    SetStatusText(wxT("缩放: 100%"), 3);
}

// ============================================================
// 菜单栏
// ============================================================
void cMain::CreateMenuBar()
{
    wxMenuBar* menuBar = new wxMenuBar();

    auto item = [](wxMenu* menu, int id,
                   const wxString& text,
                   const wxString& accel = wxString())
    {
        wxString label = text;

        if (!accel.empty())
        {
            label += wxT("\t") + accel;
        }

        menu->Append(id, label);
    };

    // -------- 文件 --------
    wxMenu* fileMenu = new wxMenu();

    item(fileMenu, ID_FILE_NEW, wxT("新建"), wxT("Ctrl+N"));
    item(fileMenu, ID_FILE_OPEN, wxT("打开..."), wxT("Ctrl+O"));
    fileMenu->AppendSeparator();
    item(fileMenu, ID_FILE_SAVE, wxT("保存"), wxT("Ctrl+S"));
    item(fileMenu, ID_FILE_SAVEAS, wxT("另存为..."), wxT("Ctrl+Shift+S"));
    fileMenu->AppendSeparator();

    wxMenu* exportMenu = new wxMenu();
    item(exportMenu, ID_FILE_EXPORT_PNG, wxT("导出为 PNG 图片"));
    item(exportMenu, ID_FILE_EXPORT_SVG, wxT("导出为 SVG 矢量图"));
    item(exportMenu, ID_FILE_EXPORT_TRUTH, wxT("导出真值表 (CSV)"));
    fileMenu->AppendSubMenu(exportMenu, wxT("导出"));

    fileMenu->AppendSeparator();
    item(fileMenu, ID_FILE_EXIT, wxT("退出"), wxT("Alt+F4"));

    // -------- 编辑 --------
    wxMenu* editMenu = new wxMenu();

    item(editMenu, ID_EDIT_UNDO, wxT("撤销"), wxT("Ctrl+Z"));
    item(editMenu, ID_EDIT_REDO, wxT("重做"), wxT("Ctrl+Y"));
    editMenu->AppendSeparator();
    item(editMenu, ID_EDIT_CUT, wxT("剪切"), wxT("Ctrl+X"));
    item(editMenu, ID_EDIT_COPY, wxT("复制"), wxT("Ctrl+C"));
    item(editMenu, ID_EDIT_PASTE, wxT("粘贴"), wxT("Ctrl+V"));
    item(editMenu, ID_EDIT_DELETE, wxT("删除"), wxT("Del"));
    editMenu->AppendSeparator();
    item(editMenu, ID_EDIT_SELECTALL, wxT("全选"), wxT("Ctrl+A"));
    item(editMenu, ID_EDIT_PROPERTIES, wxT("属性..."), wxT("E"));

    // -------- 视图 --------
    wxMenu* viewMenu = new wxMenu();

    item(viewMenu, ID_VIEW_ZOOMIN, wxT("放大"));
    item(viewMenu, ID_VIEW_ZOOMOUT, wxT("缩小"));
    viewMenu->AppendSeparator();
    item(viewMenu, ID_VIEW_FIT, wxT("适应窗口"));
    item(viewMenu, ID_VIEW_ACTUAL, wxT("实际大小 (100%)"), wxT("Ctrl+0"));
    viewMenu->AppendSeparator();
    viewMenu->AppendCheckItem(ID_VIEW_GRID, wxT("显示网格\tG"));
    viewMenu->AppendCheckItem(ID_VIEW_SNAP, wxT("对齐到网格\tS"));
    viewMenu->Check(ID_VIEW_GRID, true);
    viewMenu->Check(ID_VIEW_SNAP, true);

    wxMenu* gridSizeMenu = new wxMenu();
    gridSizeMenu->AppendRadioItem(ID_VIEW_GRIDSIZE_10, wxT("10 px"));
    gridSizeMenu->AppendRadioItem(ID_VIEW_GRIDSIZE_20, wxT("20 px"));
    gridSizeMenu->AppendRadioItem(ID_VIEW_GRIDSIZE_50, wxT("50 px"));
    gridSizeMenu->Check(ID_VIEW_GRIDSIZE_20, true);
    viewMenu->AppendSubMenu(gridSizeMenu, wxT("网格大小"));

    // -------- 放置 --------
    wxMenu* placeMenu = new wxMenu();

    item(placeMenu, ID_PLACE_AND, wxT("与门 (AND)"), wxT("A"));
    item(placeMenu, ID_PLACE_OR, wxT("或门 (OR)"), wxT("O"));
    item(placeMenu, ID_PLACE_NOT, wxT("非门 (NOT)"), wxT("N"));
    item(placeMenu, ID_PLACE_NAND, wxT("与非门 (NAND)"), wxT("D"));
    item(placeMenu, ID_PLACE_XOR, wxT("异或门 (XOR)"), wxT("X"));
    item(placeMenu, ID_PLACE_NOR, wxT("或非门 (NOR)"));
    item(placeMenu, ID_PLACE_XNOR, wxT("同或门 (XNOR)"));
    placeMenu->AppendSeparator();
    item(placeMenu, ID_PLACE_PIN_IN, wxT("输入引脚"));
    item(placeMenu, ID_PLACE_PIN_OUT, wxT("输出引脚"));
    item(placeMenu, ID_PLACE_CONST0, wxT("常量 0"));
    item(placeMenu, ID_PLACE_CONST1, wxT("常量 1"));
    placeMenu->AppendSeparator();
    item(placeMenu, ID_PLACE_WIRE, wxT("连线"), wxT("W"));
    item(placeMenu, ID_PLACE_TEXT, wxT("文字标注"), wxT("T"));

    // -------- 设计 --------
    wxMenu* designMenu = new wxMenu();

    item(designMenu, ID_DESIGN_ROTATE, wxT("顺时针旋转 90°"), wxT("R"));
    item(designMenu, ID_DESIGN_ROTATE_CCW, wxT("逆时针旋转 90°"));
    designMenu->AppendSeparator();
    item(designMenu, ID_DESIGN_MIRROR_H, wxT("水平镜像"));
    item(designMenu, ID_DESIGN_MIRROR_V, wxT("垂直镜像"));
    designMenu->AppendSeparator();

    wxMenu* alignMenu = new wxMenu();
    item(alignMenu, ID_DESIGN_ALIGN_LEFT, wxT("左对齐"));
    item(alignMenu, ID_DESIGN_ALIGN_HCENTER, wxT("水平居中"));
    item(alignMenu, ID_DESIGN_ALIGN_RIGHT, wxT("右对齐"));
    alignMenu->AppendSeparator();
    item(alignMenu, ID_DESIGN_ALIGN_TOP, wxT("上对齐"));
    item(alignMenu, ID_DESIGN_ALIGN_VCENTER, wxT("垂直居中"));
    item(alignMenu, ID_DESIGN_ALIGN_BOTTOM, wxT("下对齐"));
    designMenu->AppendSubMenu(alignMenu, wxT("对齐"));

    wxMenu* distMenu = new wxMenu();
    item(distMenu, ID_DESIGN_DIST_H, wxT("水平等距"));
    item(distMenu, ID_DESIGN_DIST_V, wxT("垂直等距"));
    designMenu->AppendSubMenu(distMenu, wxT("分布"));

    designMenu->AppendSeparator();
    item(designMenu, ID_DESIGN_GROUP, wxT("组合"));
    item(designMenu, ID_DESIGN_UNGROUP, wxT("取消组合"));

    // -------- 工具 --------
    wxMenu* toolsMenu = new wxMenu();

    item(toolsMenu, ID_TOOLS_LIBRARY, wxT("元件库管理..."));
    item(toolsMenu, ID_TOOLS_PREFS, wxT("首选项..."));
    item(toolsMenu, ID_TOOLS_SHORTCUTS, wxT("快捷键设置..."));

    // -------- 帮助 --------
    wxMenu* helpMenu = new wxMenu();

    item(helpMenu, ID_HELP_MANUAL, wxT("使用手册"), wxT("F1"));
    item(helpMenu, ID_HELP_SHORTCUTS, wxT("快捷键一览"), wxT("Ctrl+F1"));
    helpMenu->AppendSeparator();
    item(helpMenu, ID_HELP_ABOUT, wxT("关于 DrawGate"));

    menuBar->Append(fileMenu, wxT("文件(&F)"));
    menuBar->Append(editMenu, wxT("编辑(&E)"));
    menuBar->Append(viewMenu, wxT("视图(&V)"));
    menuBar->Append(placeMenu, wxT("放置(&P)"));
    menuBar->Append(designMenu, wxT("设计(&D)"));
    menuBar->Append(toolsMenu, wxT("工具(&T)"));
    menuBar->Append(helpMenu, wxT("帮助(&H)"));

    SetMenuBar(menuBar);
}

// ============================================================
// 工具栏：文件 / 编辑 / 视图 / 绘图 四组
// ============================================================
void cMain::CreateAppToolBar()
{
    wxToolBar* toolBar = CreateToolBar(
        wxTB_FLAT | wxTB_HORIZONTAL | wxTB_NODIVIDER);

    toolBar->SetToolBitmapSize(wxSize(24, 24));

    auto add = [this, toolBar](int id, const wxString& label,
                               IconId icon, const wxString& tip)
    {
        toolBar->AddTool(id, label, MakeIcon(icon), tip);
    };

    add(ID_FILE_NEW, wxT("新建"), IconId::New, wxT("新建 (Ctrl+N)"));
    add(ID_FILE_OPEN, wxT("打开"), IconId::Open, wxT("打开 (Ctrl+O)"));
    add(ID_FILE_SAVE, wxT("保存"), IconId::Save, wxT("保存 (Ctrl+S)"));

    toolBar->AddSeparator();

    add(ID_EDIT_UNDO, wxT("撤销"), IconId::Undo, wxT("撤销 (Ctrl+Z)"));
    add(ID_EDIT_REDO, wxT("重做"), IconId::Redo, wxT("重做 (Ctrl+Y)"));
    add(ID_EDIT_CUT, wxT("剪切"), IconId::Cut, wxT("剪切 (Ctrl+X)"));
    add(ID_EDIT_COPY, wxT("复制"), IconId::Copy, wxT("复制 (Ctrl+C)"));
    add(ID_EDIT_PASTE, wxT("粘贴"), IconId::Paste, wxT("粘贴 (Ctrl+V)"));
    add(ID_EDIT_DELETE, wxT("删除"), IconId::Delete, wxT("删除选中元件 (Del)"));

    toolBar->AddSeparator();

    add(ID_VIEW_ZOOMIN, wxT("放大"), IconId::ZoomIn, wxT("放大"));
    add(ID_VIEW_ZOOMOUT, wxT("缩小"), IconId::ZoomOut, wxT("缩小"));
    add(ID_VIEW_FIT, wxT("适应窗口"), IconId::Fit, wxT("适应窗口"));

    toolBar->AddCheckTool(
        ID_VIEW_GRID, wxT("网格"), MakeIcon(IconId::Grid),
        wxNullBitmap, wxT("显示/隐藏网格 (G)"));
    toolBar->ToggleTool(ID_VIEW_GRID, true);

    toolBar->AddSeparator();

    toolBar->AddCheckTool(
        ID_TOOL_SELECT, wxT("选择"), MakeIcon(IconId::Select),
        wxNullBitmap, wxT("选择工具 (Esc)"));

    toolBar->AddCheckTool(
        ID_PLACE_WIRE, wxT("连线"), MakeIcon(IconId::Wire),
        wxNullBitmap, wxT("连线 (W)"));
    toolBar->AddCheckTool(
        ID_PLACE_AND, wxT("放置门"), MakeIcon(IconId::Gate),
        wxNullBitmap, wxT("放置逻辑门 (A)"));
    toolBar->AddCheckTool(
        ID_PLACE_TEXT, wxT("文字"), MakeIcon(IconId::Text),
        wxNullBitmap, wxT("文字标注 (T)"));

    toolBar->Realize();
    UpdateToolUi(ID_TOOL_SELECT);

    // -------- 事件绑定 --------
    // 尚无实现的命令：点一下只在状态栏提示
    const int placeholders[] = {
        ID_FILE_OPEN, ID_FILE_SAVE, ID_FILE_SAVEAS,
        ID_FILE_EXPORT_PNG, ID_FILE_EXPORT_SVG, ID_FILE_EXPORT_TRUTH,
        ID_EDIT_UNDO, ID_EDIT_REDO, ID_EDIT_CUT, ID_EDIT_COPY,
        ID_EDIT_PASTE, ID_EDIT_SELECTALL,
        ID_VIEW_ZOOMIN, ID_VIEW_ZOOMOUT, ID_VIEW_FIT, ID_VIEW_ACTUAL,
        ID_PLACE_PIN_IN, ID_PLACE_PIN_OUT,
        ID_PLACE_CONST0, ID_PLACE_CONST1,
        ID_DESIGN_ROTATE, ID_DESIGN_ROTATE_CCW, ID_DESIGN_MIRROR_H,
        ID_DESIGN_MIRROR_V, ID_DESIGN_ALIGN_LEFT, ID_DESIGN_ALIGN_HCENTER,
        ID_DESIGN_ALIGN_RIGHT, ID_DESIGN_ALIGN_TOP, ID_DESIGN_ALIGN_VCENTER,
        ID_DESIGN_ALIGN_BOTTOM, ID_DESIGN_DIST_H, ID_DESIGN_DIST_V,
        ID_DESIGN_GROUP, ID_DESIGN_UNGROUP,
        ID_TOOLS_LIBRARY, ID_TOOLS_PREFS, ID_TOOLS_SHORTCUTS
    };

    for (int id : placeholders)
    {
        BindCommand(id, &cMain::OnPlaceholder);
    }

    // 已有实现的命令
    BindCommand(ID_FILE_EXIT, &cMain::OnExit);
    BindCommand(ID_EDIT_DELETE, &cMain::OnDelete);
    BindCommand(ID_PLACE_WIRE, &cMain::OnPlaceWire);
    BindCommand(ID_TOOL_SELECT, &cMain::OnSelectTool);
    BindCommand(ID_HELP_ABOUT, &cMain::OnAbout);
    BindCommand(ID_HELP_SHORTCUTS, &cMain::OnShowShortcuts);
    BindCommand(ID_PLACE_TEXT, &cMain::OnPlaceText);
    BindCommand(ID_EDIT_PROPERTIES, &cMain::OnProperties);

    // S 级功能
    BindCommand(ID_FILE_NEW, &cMain::OnNewDocument);
    BindCommand(ID_HELP_MANUAL, &cMain::OnShowManual);
    BindCommand(ID_VIEW_GRID, &cMain::OnToggleGrid);
    BindCommand(ID_VIEW_SNAP, &cMain::OnToggleSnap);

    for (int id : { ID_VIEW_GRIDSIZE_10, ID_VIEW_GRIDSIZE_20,
                    ID_VIEW_GRIDSIZE_50 })
    {
        BindCommand(id, &cMain::OnSetGridSize);
    }

    for (int id : { ID_PLACE_AND, ID_PLACE_OR, ID_PLACE_NOT,
                    ID_PLACE_NAND, ID_PLACE_XOR,
                    ID_PLACE_NOR, ID_PLACE_XNOR })
    {
        BindCommand(id, &cMain::OnPlaceComponent);
    }
}

// ============================================================
// 左侧面板：元件库 + 属性占位
// ============================================================
void cMain::CreateLeftPanel()
{
    m_leftPanel = new wxPanel(this, wxID_ANY);
    m_leftPanel->SetMinSize(wxSize(250, -1));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxFont titleFont = m_leftPanel->GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleFont.SetPointSize(titleFont.GetPointSize() + 1);

    wxStaticText* libraryTitle =
        new wxStaticText(m_leftPanel, wxID_ANY, wxT("元件库"));
    libraryTitle->SetFont(titleFont);

    sizer->Add(libraryTitle, 0, wxLEFT | wxRIGHT | wxTOP, 14);

    m_componentTree = new wxTreeCtrl(//创建树控件
        m_leftPanel,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxTR_DEFAULT_STYLE | wxTR_HAS_BUTTONS);

    const wxTreeItemId root = m_componentTree->AddRoot(wxT("元件库"));//根节点
    const wxTreeItemId gates =
        m_componentTree->AppendItem(root, wxT("逻辑门"));//分类节点

    const auto& components = ComponentLibrary::GetComponents();

    for (const auto& component : components)
    {
        GateType gateType = AND;
        switch (component.type)//挨个加入
        {
        case LibraryComponentType::AndGate:  gateType = AND; break;
        case LibraryComponentType::OrGate:   gateType = OR; break;
        case LibraryComponentType::NotGate:  gateType = NOT; break;
        case LibraryComponentType::NandGate: gateType = NAND_GATE; break;
        case LibraryComponentType::XorGate:  gateType = XOR_GATE; break;
        case LibraryComponentType::NorGate:  gateType = NOR_GATE; break;
        case LibraryComponentType::XnorGate: gateType = XNOR_GATE; break;
        }

        m_componentTree->AppendItem(
            gates,
            GateTypeName(gateType),
            -1,
            -1,
            new ComponentTreeItemData(component.type));
    }

    m_componentTree->Expand(root);//根节点和分类节点默认展开
    m_componentTree->Expand(gates);

    m_componentTree->Bind(
        wxEVT_TREE_ITEM_ACTIVATED,
        &cMain::OnLibraryItemActivated,
        this);

    sizer->Add(m_componentTree, 1, wxEXPAND | wxALL, 12);

    wxStaticText* hint = new wxStaticText(
        m_leftPanel,
        wxID_ANY,
        wxT("双击元件即可放置到画布"));

    hint->SetForegroundColour(wxColour(120, 124, 130));

    sizer->Add(hint, 0, wxLEFT | wxRIGHT | wxBOTTOM, 14);

    sizer->Add(
        new wxStaticLine(m_leftPanel),
        0, wxEXPAND | wxLEFT | wxRIGHT, 12);

    wxStaticText* propTitle =
        new wxStaticText(m_leftPanel, wxID_ANY, wxT("属性"));
    propTitle->SetFont(titleFont);

    sizer->Add(propTitle, 0, wxLEFT | wxRIGHT | wxTOP, 14);

    wxStaticText* propValue = new wxStaticText(
        m_leftPanel, wxID_ANY, wxT("未选中元件"));

    propValue->SetForegroundColour(wxColour(120, 124, 130));

    sizer->Add(propValue, 0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 14);

    m_leftPanel->SetSizer(sizer);
}

// ============================================================
// 命令绑定：菜单和工具栏共用同一套 ID，两个事件都绑
// ============================================================
void cMain::BindCommand(int id, void (cMain::*handler)(wxCommandEvent&))
{
    Bind(wxEVT_MENU, handler, this, id);
    Bind(wxEVT_TOOL, handler, this, id);
}

void cMain::SetToolText(const wxString& toolName)
{
    SetStatusText(wxT("工具: ") + toolName, 1);
}

// ============================================================
// 占位命令
// ============================================================
void cMain::OnPlaceholder(wxCommandEvent& evt)
{
    wxString name;

    if (wxMenuBar* menuBar = GetMenuBar())
    {
        if (wxMenuItem* menuItem = menuBar->FindItem(evt.GetId()))
        {
            name = menuItem->GetItemLabelText();
        }
    }

    if (name.empty())
    {
        name = wxT("该功能");
    }

    SetStatusText(wxString::Format(wxT("「%s」尚未实现"), name), 0);
}

// ============================================================
// 退出 / 删除 / 关于
// ============================================================
void cMain::OnExit(wxCommandEvent&)
{
    Close(true);
}

void cMain::OnDelete(wxCommandEvent&)
{
    if (drawborad != nullptr)
    {
        drawborad->DeleteSelectedComponent();
        drawborad->SetFocus();
    }

    SetStatusText(wxT("已删除选中元件"), 0);
}

// ============================================================
// 文件 -> 新建
// ============================================================
void cMain::OnNewDocument(wxCommandEvent&)
{
    if (drawborad == nullptr)
    {
        return;
    }

    drawborad->ClearAll();

    SetToolText(wxT("选择"));
    SetStatusText(wxT("已新建空白图纸"), 0);
    SetTitle(wxT("DrawGate - 未命名"));
}

// ============================================================
// 视图 -> 显示网格 / 对齐到网格
// ============================================================
void cMain::OnToggleGrid(wxCommandEvent&)
{
    if (drawborad == nullptr)
    {
        return;
    }

    const bool show = !drawborad->IsGridVisible();
    drawborad->SetGridVisible(show);

    if (wxMenuBar* bar = GetMenuBar())
    {
        if (wxMenuItem* item = bar->FindItem(ID_VIEW_GRID))
        {
            item->Check(show);
        }
    }

    if (wxToolBar* bar = GetToolBar())
    {
        bar->ToggleTool(ID_VIEW_GRID, show);
    }

    SetStatusText(show ? wxT("显示网格") : wxT("隐藏网格"), 0);
}

void cMain::OnToggleSnap(wxCommandEvent&)
{
    if (drawborad == nullptr)
    {
        return;
    }

    const bool on = !drawborad->IsSnapToGrid();
    drawborad->SetSnapToGrid(on);

    if (wxMenuBar* bar = GetMenuBar())
    {
        if (wxMenuItem* item = bar->FindItem(ID_VIEW_SNAP))
        {
            item->Check(on);
        }
    }

    SetStatusText(
        on ? wxT("已开启网格吸附（拖动时按住 Alt 可临时关闭）")
           : wxT("已关闭网格吸附"),
        0);
}

void cMain::OnSetGridSize(wxCommandEvent& evt)
{
    if (drawborad == nullptr)
    {
        return;
    }

    int size = 20;

    switch (evt.GetId())
    {
    case ID_VIEW_GRIDSIZE_10:
        size = 10;
        break;

    case ID_VIEW_GRIDSIZE_20:
        size = 20;
        break;

    case ID_VIEW_GRIDSIZE_50:
        size = 50;
        break;

    default:
        break;
    }

    drawborad->SetGridSize(size);

    SetStatusText(
        wxString::Format(wxT("网格大小：%d px"), size), 0);
}

// ============================================================
// 帮助 -> 使用手册
// ============================================================
void cMain::OnShowManual(wxCommandEvent&)
{
    wxDialog dialog(
        this, wxID_ANY, wxT("使用手册"),
        wxDefaultPosition, wxSize(660, 600));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxTextCtrl* text = new wxTextCtrl(
        &dialog, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxBORDER_THEME);

    text->SetValue(
        wxT("DrawGate 使用手册\n")
        wxT("====================\n\n")
        wxT("一、放置元件\n")
        wxT("  · 在左侧「元件库」双击 AND / OR / NOT / NAND / XOR，\n")
        wxT("    光标进入放置模式，在画布上单击落点即可。\n")
        wxT("  · 也可以直接用快捷键：A / O / N / D / X。\n")
        wxT("  · 放置完成后自动回到选择工具；按 Esc 可取消放置。\n")
        wxT("  · 按住 Alt 放置时，临时关闭网格吸附，便于微调位置。\n\n")
        wxT("二、连线\n")
        wxT("  · 按 W 进入连线工具，或点工具栏的「连线」按钮。\n")
        wxT("  · 在元件的输出端口（右侧圆点）按下鼠标，拖到目标元件的\n")
        wxT("    输入端口（左侧圆点）松开，完成一条连线。\n")
        wxT("  · 拖动过程中显示红色虚线预览。\n")
        wxT("  · 连线会跟随元件移动而自动更新。\n\n")
        wxT("三、编辑\n")
        wxT("  · 单击选中元件，出现蓝色虚线选择框，拖动可移动。\n")
        wxT("  · Del 删除选中元件，与它相连的线一并删除。\n")
        wxT("  · Esc 取消当前操作（连线 / 放置）并回到选择工具。\n\n")
        wxT("四、视图\n")
        wxT("  · 视图 → 显示网格 (G)：开关网格背景。\n")
        wxT("  · 视图 → 对齐到网格 (S)：开关吸附，移动/放置时坐标取整。\n")
        wxT("  · 视图 → 网格大小：10 / 20 / 50 像素三档。\n")
        wxT("  · 状态栏右下角显示当前缩放比例（本版本固定 100%）。\n\n")
        wxT("五、文件\n")
        wxT("  · 新建 (Ctrl+N)：清空画布，开始一张新图。\n")
        wxT("  · 打开 / 保存 / 导出：界面已就位，功能开发中。\n\n")
        wxT("六、帮助\n")
        wxT("  · 快捷键一览 (Ctrl+F1)：查看全部快捷键及实现状态。\n\n")
        wxT("提示：鼠标移到工具栏按钮上会显示按钮名称和快捷键。\n"));

    text->SetInsertionPoint(0);

    sizer->Add(text, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    wxBoxSizer* buttonRow = new wxBoxSizer(wxHORIZONTAL);

    wxButton* closeButton =
        new wxButton(&dialog, wxID_CANCEL, wxT("关闭"));

    closeButton->SetDefault();
    buttonRow->Add(closeButton, 0);

    sizer->Add(buttonRow, 0, wxALIGN_RIGHT | wxALL, 12);

    dialog.SetSizer(sizer);
    dialog.CentreOnParent();
    dialog.ShowModal();
}

// ============================================================
// 帮助 -> 快捷键一览
// ============================================================
void cMain::OnShowShortcuts(wxCommandEvent&)
{
    struct ShortcutRow
    {
        const wxChar* group;
        const wxChar* keys;
        const wxChar* action;
        bool available;
    };

    static const ShortcutRow rows[] = {
        { wxT("文件"), wxT("Ctrl+N"),       wxT("新建原理图"),            true },
        { wxT("文件"), wxT("Ctrl+O"),       wxT("打开工程"),              false },
        { wxT("文件"), wxT("Ctrl+S"),       wxT("保存"),                  false },
        { wxT("文件"), wxT("Ctrl+Shift+S"), wxT("另存为"),                false },
        { wxT("文件"), wxT("Alt+F4"),       wxT("退出"),                  true  },

        { wxT("编辑"), wxT("Ctrl+Z"),       wxT("撤销"),                  false },
        { wxT("编辑"), wxT("Ctrl+Y"),       wxT("重做"),                  false },
        { wxT("编辑"), wxT("Ctrl+X"),       wxT("剪切"),                  false },
        { wxT("编辑"), wxT("Ctrl+C"),       wxT("复制"),                  false },
        { wxT("编辑"), wxT("Ctrl+V"),       wxT("粘贴"),                  false },
        { wxT("编辑"), wxT("Del"),          wxT("删除选中元件"),          true  },
        { wxT("编辑"), wxT("Ctrl+A"),       wxT("全选"),                  false },
        { wxT("编辑"), wxT("E"),            wxT("打开属性对话框"),        true  },

        { wxT("视图"), wxT("G"),            wxT("显示 / 隐藏网格"),       true },
        { wxT("视图"), wxT("S"),            wxT("对齐到网格"),            true },
        { wxT("视图"), wxT("Ctrl+0"),       wxT("实际大小 (100%)"),       false },

        { wxT("放置"), wxT("A"),            wxT("放置与门 (AND)"),        true  },
        { wxT("放置"), wxT("O"),            wxT("放置或门 (OR)"),         true  },
        { wxT("放置"), wxT("N"),            wxT("放置非门 (NOT)"),        true  },
        { wxT("放置"), wxT("D"),            wxT("放置与非门 (NAND)"),     true  },
        { wxT("放置"), wxT("X"),            wxT("放置异或门 (XOR)"),      true  },
        { wxT("放置"), wxT("W"),            wxT("连线工具"),              true  },
        { wxT("放置"), wxT("T"),            wxT("文字标注"),              true  },

        { wxT("设计"), wxT("R"),            wxT("顺时针旋转 90°"),        false },

        { wxT("帮助"), wxT("F1"),           wxT("使用手册"),              true },
        { wxT("帮助"), wxT("Ctrl+F1"),      wxT("快捷键一览"),            true  },

        { wxT("鼠标"), wxT("滚轮"),          wxT("以光标为中心缩放"),      false },
        { wxT("鼠标"), wxT("中键拖动"),      wxT("平移画布"),              false },
        { wxT("鼠标"), wxT("双击元件"),      wxT("打开属性"),              false },
        { wxT("鼠标"), wxT("双击元件库"),    wxT("放置该元件"),            true  },
        { wxT("鼠标"), wxT("拖动端口"),      wxT("从输出端口连到输入端口"), true  },
        { wxT("鼠标"), wxT("Esc"),          wxT("取消当前工具"),          true }
    };

    wxDialog dialog(
        this, wxID_ANY, wxT("快捷键一览"),
        wxDefaultPosition, wxSize(700, 600));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxListCtrl* list = new wxListCtrl(
        &dialog, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_THEME);

    list->InsertColumn(0, wxT("分类"), wxLIST_FORMAT_LEFT, 80);
    list->InsertColumn(1, wxT("快捷键"), wxLIST_FORMAT_LEFT, 150);
    list->InsertColumn(2, wxT("功能"), wxLIST_FORMAT_LEFT, 320);
    list->InsertColumn(3, wxT("状态"), wxLIST_FORMAT_LEFT, 90);

    const wxColour readyColour(0, 128, 64);
    const wxColour todoColour(150, 150, 150);

    long index = 0;

    for (const ShortcutRow& row : rows)
    {
        const long item = list->InsertItem(index++, row.group);

        list->SetItem(item, 1, row.keys);
        list->SetItem(item, 2, row.action);
        list->SetItem(item, 3, row.available ? wxT("可用") : wxT("待实现"));

        list->SetItemTextColour(
            item, row.available ? readyColour : todoColour);
    }

    sizer->Add(list, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    wxStaticText* note = new wxStaticText(
        &dialog, wxID_ANY,
        wxT("绿色「可用」= 已生效   灰色「待实现」= 规划中，后续版本逐步接入") );

    note->SetForegroundColour(wxColour(110, 114, 120));

    sizer->Add(note, 0, wxLEFT | wxRIGHT | wxTOP, 14);

    wxSizer* buttonRow = new wxBoxSizer(wxHORIZONTAL);

    wxButton* closeButton =
        new wxButton(&dialog, wxID_CANCEL, wxT("关闭"));

    closeButton->SetDefault();
    buttonRow->Add(closeButton, 0);

    sizer->Add(buttonRow, 0, wxALIGN_RIGHT | wxALL, 12);

    dialog.SetSizer(sizer);
    dialog.CentreOnParent();
    list->SetFocus();
    dialog.ShowModal();
}

void cMain::OnAbout(wxCommandEvent&)
{
    wxMessageBox(
        wxT("DrawGate - 逻辑门绘图\n\n")
        wxT("wxWidgets + MSVC 课程项目\n")
        wxT("当前版本：界面框架"),
        wxT("关于 DrawGate"),
        wxOK | wxICON_INFORMATION,
        this);
}

// ============================================================
// 连线工具
// ============================================================
void cMain::OnPlaceWire(wxCommandEvent&)
{
    drawborad->SetCurrentTool(TOOL_WIRE);

    UpdateToolUi(ID_PLACE_WIRE);
    drawborad->SetFocus();

    SetToolText(wxT("连线"));
    SetStatusText(wxT("连线：可从输出端口或空白处起笔，拖动或点击两次落笔"), 0);
}

// ============================================================
// 选择工具
// ============================================================
void cMain::OnSelectTool(wxCommandEvent&)
{
    if (drawborad == nullptr)
    {
        return;
    }

    drawborad->isDrawingAndGate = false;
    drawborad->isDrawingOrGate = false;
    drawborad->isDrawingNotGate = false;
    drawborad->isDrawingNandGate = false;
    drawborad->isDrawingXorGate = false;
    drawborad->isDrawingNorGate = false;
    drawborad->isDrawingXnorGate = false;

    drawborad->SetCurrentTool(TOOL_SELECT);
    drawborad->SetFocus();
    drawborad->Refresh(false);

    UpdateToolUi(ID_TOOL_SELECT);

    SetToolText(wxT("选择"));
    SetStatusText(wxT("就绪"), 0);
}

// ============================================================
// 放置逻辑门（带快捷键 A / O / N / D / X）
// ============================================================
void cMain::OnPlaceComponent(wxCommandEvent& evt)//放置元件
{
    switch (evt.GetId())
    {
    case ID_PLACE_AND:
        PlaceGate(LibraryComponentType::AndGate);
        break;

    case ID_PLACE_OR:
        PlaceGate(LibraryComponentType::OrGate);
        break;

    case ID_PLACE_NOT:
        PlaceGate(LibraryComponentType::NotGate);
        break;

    case ID_PLACE_NAND:
        PlaceGate(LibraryComponentType::NandGate);
        break;

    case ID_PLACE_XOR:
        PlaceGate(LibraryComponentType::XorGate);
        break;

    case ID_PLACE_NOR:
        PlaceGate(LibraryComponentType::NorGate);
        break;

    case ID_PLACE_XNOR:
        PlaceGate(LibraryComponentType::XnorGate);
        break;

    default:
        break;
    }
}

void cMain::PlaceGate(LibraryComponentType type)
{
    if (drawborad == nullptr)
    {
        return;
    }

    // 先清掉所有旧的放置状态
    drawborad->isDrawingAndGate = false;
    drawborad->isDrawingOrGate = false;
    drawborad->isDrawingNotGate = false;
    drawborad->isDrawingNandGate = false;
    drawborad->isDrawingXorGate = false;
    drawborad->isDrawingNorGate = false;
    drawborad->isDrawingXnorGate = false;

    wxString toolName;

    switch (type)
    {
    case LibraryComponentType::AndGate:
        drawborad->isDrawingAndGate = true;
        toolName = wxT("放置 AND");
        break;

    case LibraryComponentType::OrGate:
        drawborad->isDrawingOrGate = true;
        toolName = wxT("放置 OR");
        break;

    case LibraryComponentType::NotGate:
        drawborad->isDrawingNotGate = true;
        toolName = wxT("放置 NOT");
        break;

    case LibraryComponentType::NandGate:
        drawborad->isDrawingNandGate = true;
        toolName = wxT("放置 NAND");
        break;

    case LibraryComponentType::XorGate:
        drawborad->isDrawingXorGate = true;
        toolName = wxT("放置 XOR");
        break;

    case LibraryComponentType::NorGate:
        drawborad->isDrawingNorGate = true;
        toolName = wxT("放置 NOR");
        break;

    case LibraryComponentType::XnorGate:
        drawborad->isDrawingXnorGate = true;
        toolName = wxT("放置 XNOR");
        break;

    default:
        return;
    }

    UpdateToolUi(ID_PLACE_AND);

    SetToolText(toolName);
    SetStatusText(wxT("在画布上点击以放置元件"), 0);

    drawborad->SetFocus();
    drawborad->Refresh(false);
}

// ============================================================
// 双击元件库
// ============================================================
void cMain::OnLibraryItemActivated(wxTreeEvent& evt)
{
    if (m_componentTree == nullptr)
    {
        return;
    }

    auto* itemData = dynamic_cast<ComponentTreeItemData*>(
        m_componentTree->GetItemData(evt.GetItem()));
    if (itemData == nullptr)
    {
        return;
    }

    PlaceGate(itemData->type);
}

// ============================================================
// 文字标注工具
// ============================================================
void cMain::OnPlaceText(wxCommandEvent&)
{
    if (drawborad == nullptr)
    {
        return;
    }

    drawborad->isDrawingAndGate = false;
    drawborad->isDrawingOrGate = false;
    drawborad->isDrawingNotGate = false;
    drawborad->isDrawingNandGate = false;
    drawborad->isDrawingXorGate = false;
    drawborad->isDrawingNorGate = false;
    drawborad->isDrawingXnorGate = false;

    drawborad->SetCurrentTool(TOOL_TEXT);
    drawborad->SetFocus();

    UpdateToolUi(ID_PLACE_TEXT);

    SetToolText(wxT("文字"));
    SetStatusText(wxT("在画布上点击，输入文字标注"), 0);
}

// ============================================================
// 元件 / 文字属性
// ============================================================
void cMain::OnProperties(wxCommandEvent&)
{
    if (drawborad == nullptr)
    {
        return;
    }

    Gate* gate = drawborad->GetSelectedGate();

    if (gate == nullptr)
    {
        if (drawborad->GetSelectedTextIndex() >= 0)
        {
            drawborad->EditSelectedText();
            return;
        }

        SetStatusText(wxT("请先选中一个元件或文字标注"), 0);
        return;
    }

    wxDialog dialog(
        this, wxID_ANY, wxT("元件属性"),
        wxDefaultPosition, wxSize(420, 320));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 10, 14);
    grid->AddGrowableCol(1, 1);

    auto addRow = [&](const wxString& title, wxWindow* control)
    {
        grid->Add(
            new wxStaticText(&dialog, wxID_ANY, title),
            0, wxALIGN_CENTER_VERTICAL);
        grid->Add(control, 1, wxEXPAND);
    };

    wxTextCtrl* labelCtrl = new wxTextCtrl(
        &dialog, wxID_ANY,
        gate->label.empty()
            ? wxString::Format(wxT("U%d"), gate->id)
            : gate->label);

    addRow(wxT("类型："),
        new wxStaticText(&dialog, wxID_ANY, GateTypeName(gate->type)));

    addRow(wxT("标号："), labelCtrl);

    addRow(wxT("位置："),
        new wxStaticText(
            &dialog, wxID_ANY,
            wxString::Format(
                wxT("X = %d    Y = %d"),
                gate->center.x, gate->center.y)));

    addRow(wxT("输入端口："),
        new wxStaticText(
            &dialog, wxID_ANY,
            wxString::Format(wxT("%d 个"), gate->GetInputCount())));

    sizer->Add(grid, 1, wxEXPAND | wxALL, 16);

    wxBoxSizer* buttonRow = new wxBoxSizer(wxHORIZONTAL);
    buttonRow->Add(
        new wxButton(&dialog, wxID_CANCEL, wxT("取消")),
        0, wxRIGHT, 8);

    wxButton* okButton =
        new wxButton(&dialog, wxID_OK, wxT("确定"));
    okButton->SetDefault();
    buttonRow->Add(okButton, 0);

    sizer->Add(buttonRow, 0, wxALIGN_RIGHT | wxALL, 12);

    dialog.SetSizer(sizer);
    dialog.CentreOnParent();
    labelCtrl->SetFocus();

    if (dialog.ShowModal() != wxID_OK)
    {
        return;
    }

    gate->label = labelCtrl->GetValue().Trim();

    drawborad->Refresh(false);
    SetStatusText(wxT("已更新元件属性"), 0);
}

// ============================================================
// 工具栏当前工具高亮
// ============================================================
void cMain::UpdateToolUi(int activeCommandId)
{
    wxToolBar* toolBar = GetToolBar();

    if (toolBar == nullptr)
    {
        return;
    }

    const int tools[] = {
        ID_TOOL_SELECT,
        ID_PLACE_WIRE,
        ID_PLACE_AND,
        ID_PLACE_TEXT
    };

    for (int id : tools)
    {
        toolBar->ToggleTool(id, id == activeCommandId);
    }
}
