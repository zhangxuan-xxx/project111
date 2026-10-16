
#include "cMain.h"
#include "DrawBoard.h"
#include "ComponentLibrary.h"

cMain::cMain()
    : wxFrame(
        nullptr,
        wxID_ANY,
        "Hello wxWidgets!",
        wxPoint(30, 30),
        wxSize(800, 600)
    )
{
    // 元件库标题
    wxStaticText* libraryTitle = new wxStaticText(
        this,
        wxID_ANY,
        "Component Library"
    );

    // 文本框
    m_txt1 = new wxTextCtrl(
        this,
        wxID_ANY,
        wxEmptyString,
        wxDefaultPosition,
        wxDefaultSize
    );

    // 元件列表
    m_list1 = new wxListBox(
        this,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize
    );

    // 加载元件名称
    const auto& components = ComponentLibrary::GetComponents();

    for (const auto& component : components)
    {
        m_list1->Append(component.name);
    }

    // 绑定元件库双击事件
    m_list1->Bind(
        wxEVT_LISTBOX_DCLICK,
        &cMain::OnLibraryItemDoubleClick,
        this
    );

    // AND 门按钮
    m_AND = new wxButton(
        this,
        wxID_ANY,
        "Draw And Gate",
        wxDefaultPosition,
        wxSize(120, 40)
    );

    // OR 门按钮
    m_OR = new wxButton(
        this,
        wxID_ANY,
        "Draw Or Gate",
        wxDefaultPosition,
        wxSize(120, 40)
    );

    // 删除按钮
    m_Delete = new wxButton(
        this,
        wxID_ANY,
        "Delete",
        wxDefaultPosition,
        wxSize(120, 40)
    );

    m_AND->Bind(
        wxEVT_BUTTON,
        &cMain::OnEnableDrawAndGate,
        this
    );

    m_OR->Bind(
        wxEVT_BUTTON,
        &cMain::OnEnableDrawOrGate,
        this
    );

    m_Delete->Bind(
        wxEVT_BUTTON,
        &cMain::OnDeleteButtonClick,
        this
    );

    // 初始化图片处理器
    wxInitAllImageHandlers();

    // 创建连线按钮
    wxBitmap bitmapForDrawLine;

    bool imageLoaded = bitmapForDrawLine.LoadFile(
        "resource/image/drawLine.png",
        wxBITMAP_TYPE_PNG
    );

    if (imageLoaded && bitmapForDrawLine.IsOk())
    {
        wxImage image = bitmapForDrawLine.ConvertToImage();

        image = image.Scale(
            24,
            24,
            wxIMAGE_QUALITY_HIGH
        );

        bitmapForDrawLine = wxBitmap(image);

        drawLineButton = new wxBitmapButton(
            this,
            wxID_ANY,
            bitmapForDrawLine,
            wxDefaultPosition,
            wxDefaultSize
        );
    }
    else
    {
        drawLineButton = new wxButton(
            this,
            wxID_ANY,
            "Draw Wire",
            wxDefaultPosition,
            wxSize(120, 40)
        );
    }

    drawLineButton->Bind(
        wxEVT_BUTTON,
        &cMain::OnDrawLineButtonClick,
        this
    );

    drawLineButton->SetToolTip("Draw line");

    // 左侧工具栏布局
    wxBoxSizer* controlboard = new wxBoxSizer(wxVERTICAL);

    controlboard->Add(
        drawLineButton,
        0,
        wxALL | wxEXPAND,
        5
    );

    controlboard->Add(
        libraryTitle,
        0,
        wxALL | wxEXPAND,
        5
    );

    controlboard->Add(
        m_list1,
        1,
        wxALL | wxEXPAND,
        5
    );

    controlboard->Add(
        m_AND,
        0,
        wxALL | wxEXPAND,
        5
    );

    controlboard->Add(
        m_OR,
        0,
        wxALL | wxEXPAND,
        5
    );

    controlboard->Add(
        m_Delete,
        0,
        wxALL | wxEXPAND,
        5
    );

    // 创建绘图画布
    drawborad = new DrawBoard(this);

    // 主窗口布局
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    mainSizer->Add(
        controlboard,
        0,
        wxALL | wxEXPAND,
        5
    );

    mainSizer->Add(
        drawborad,
        1,
        wxALL | wxEXPAND,
        5
    );

    SetSizer(mainSizer);
    SetMinSize(wxSize(700, 450));
}

// 析构函数
cMain::~cMain()
{
}
// 启用或取消 AND 门绘制
void cMain::OnEnableDrawAndGate(wxCommandEvent& evt)
{
    drawborad->isDrawingAndGate =
        !drawborad->isDrawingAndGate;

    drawborad->isDrawingOrGate = false;
    drawborad->isDrawingNotGate = false;

    drawborad->SetFocus();
    drawborad->Refresh(false);
}

// 启用或取消 OR 门绘制
void cMain::OnEnableDrawOrGate(wxCommandEvent& evt)
{
    drawborad->isDrawingOrGate =
        !drawborad->isDrawingOrGate;

    drawborad->isDrawingAndGate = false;
    drawborad->isDrawingNotGate = false;

    drawborad->SetFocus();
    drawborad->Refresh(false);
}
// 原有文本输入功能
void cMain::OnButtoClicked(wxCommandEvent& evt)
{
    if (m_txt1 != nullptr && m_list1 != nullptr)
    {
        m_list1->Append(m_txt1->GetValue());
    }

    evt.Skip();
}

// 连线工具
void cMain::OnDrawLineButtonClick(wxCommandEvent& event)
{
    drawborad->SetCurrentTool(TOOL_WIRE);
    drawborad->SetFocus();
}

// 删除选中的元件
void cMain::OnDeleteButtonClick(wxCommandEvent& evt)
{
    drawborad->DeleteSelectedComponent();
    drawborad->SetFocus();
}
// 双击元件库，进入对应元件的放置模式
void cMain::OnLibraryItemDoubleClick(wxCommandEvent& evt)
{
    if (drawborad == nullptr || m_list1 == nullptr)
        return;

    int selection = m_list1->GetSelection();

    if (selection == wxNOT_FOUND)
        return;

    const auto& components = ComponentLibrary::GetComponents();

    if (static_cast<size_t>(selection) >= components.size())
        return;

    // 清除所有旧的放置状态
    drawborad->isDrawingAndGate = false;
    drawborad->isDrawingOrGate = false;
    drawborad->isDrawingNotGate = false;
    drawborad->isDrawingNandGate = false;
    drawborad->isDrawingXorGate = false;

    // 根据元件库的选择设置放置状态
    switch (components[selection].type)
    {
    case LibraryComponentType::AndGate:
        drawborad->isDrawingAndGate = true;
        wxLogMessage("Library selected: AND Gate");
        break;

    case LibraryComponentType::OrGate:
        drawborad->isDrawingOrGate = true;
        wxLogMessage("Library selected: OR Gate");
        break;

    case LibraryComponentType::NotGate:
        drawborad->isDrawingNotGate = true;
        wxLogMessage("Library selected: NOT Gate");
        break;

    case LibraryComponentType::NandGate:
        drawborad->isDrawingNandGate = true;
        wxLogMessage("Library selected: NAND Gate");
        break;

    case LibraryComponentType::XorGate:
        drawborad->isDrawingXorGate = true;
        wxLogMessage("Library selected: XOR Gate");
        break;

    default:
        return;
    }

    drawborad->SetFocus();
    drawborad->Refresh(false);
}