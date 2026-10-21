
#include "DrawBoard.h"
#include "CommandIds.h"

#include <wx/textdlg.h>

#include <cmath>

wxDEFINE_EVENT(EVT_DRAWBOARD_TOOL_CHANGED, wxCommandEvent);

// =====================================================
// 构造函数
// =====================================================
DrawBoard::DrawBoard(wxFrame* parent)
    : wxPanel(parent)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    Bind(wxEVT_PAINT, &DrawBoard::OnPaint, this);
    Bind(wxEVT_MOTION, &DrawBoard::OnMouseMove, this);
    Bind(wxEVT_LEFT_DOWN, &DrawBoard::OnMouseDown, this);
    Bind(wxEVT_LEFT_UP, &DrawBoard::OnMouseUp, this);
    Bind(wxEVT_RIGHT_UP, &DrawBoard::OnRightClick, this);
    Bind(wxEVT_MOUSE_CAPTURE_LOST,
        [this](wxMouseCaptureLostEvent& event)
        {
            m_draggingGate = false;
        });

    // Delete 删除选中元件，Esc 取消当前操作
    Bind(wxEVT_CHAR_HOOK,
        [this](wxKeyEvent& event)
        {
            const int key = event.GetKeyCode();

            if (key == WXK_DELETE)
            {
                DeleteSelectedGate();
            }
            else if (key == WXK_ESCAPE)
            {
                CancelCurrentAction();
            }
            else
            {
                event.Skip();
            }
        });

    // 坐标显示改由状态栏负责，画布上不再叠加静态文本
    m_mousePos = wxPoint(0, 0);
    m_wireStartPos = wxPoint(0, 0);
    m_wireEndPos = wxPoint(0, 0);

    m_wiring = false;
    m_draggingGate = false;
    m_selectedGate = nullptr;
    m_selectedWire = -1;
    m_wireStartComponentId = -1;

    isDrawingAndGate = false;
    isDrawingOrGate = false;
    isDrawingNotGate = false;
    isDrawingNandGate = false;
    isDrawingXorGate = false;

    SetCurrentTool(TOOL_SELECT);
}

// =====================================================
// 析构函数
// =====================================================
DrawBoard::~DrawBoard()
{
    if (HasCapture())
    {
        ReleaseMouse();
    }

    for (Gate* gate : Gates)
    {
        delete gate;
    }

    Gates.clear();
    m_selectedGate = nullptr;
}

// =====================================================
// 绘图
// =====================================================
void DrawBoard::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this);

    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();

    wxSize size = GetClientSize();

    if (size.x <= 0 || size.y <= 0)
    {
        return;
    }

    wxBitmap bufferBitmap(size);
    wxMemoryDC memDC;

    memDC.SelectObject(bufferBitmap);
    memDC.SetBackground(*wxWHITE_BRUSH);
    memDC.Clear();

    // 绘制网格
    wxGraphicsContext* gc = wxGraphicsContext::Create(memDC);

    if (gc != nullptr)
    {
        drawGrid(gc);

        // 鼠标十字参考线
        gc->SetPen(wxPen(wxColour(0, 0, 0), 1));

        gc->StrokeLine(
            m_mousePos.x, 0,
            m_mousePos.x, size.y);

        gc->StrokeLine(
            0, m_mousePos.y,
            size.x, m_mousePos.y);

        delete gc;
    }

    // 绘制已有连线（右键选中的线用现有活动色高亮）
    for (int i = 0; i < static_cast<int>(m_wires.size()); ++i)
    {
        m_wires[i].isActive = (i == m_selectedWire);
        m_wires[i].Draw(memDC);
    }

    // 绘制逻辑门
    for (const Gate* gate : Gates)
    {
        if (gate != nullptr)
        {
            gate->drawSelf(memDC);
        }
    }

    // 绘制元件标号（为空时自动显示 U<id>）
    memDC.SetFont(wxFont(
        9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_BOLD));
    memDC.SetTextForeground(wxColour(30, 60, 140));

    for (const Gate* gate : Gates)
    {
        if (gate == nullptr)
        {
            continue;
        }

        wxString name = gate->label;

        if (name.empty())
        {
            name = wxString::Format(wxT("U%d"), gate->id);
        }

        memDC.DrawText(
            name,
            gate->center.x - static_cast<int>(42 * gate->scale),
            gate->center.y - static_cast<int>(40 * gate->scale));
    }

    // 绘制文字标注
    memDC.SetFont(wxFont(
        11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL));
    memDC.SetTextForeground(wxColour(40, 40, 40));

    for (const BoardText& item : m_texts)
    {
        memDC.DrawText(item.text, item.pos);
    }

    // 选中文字的虚线框
    if (m_selectedText >= 0 &&
        m_selectedText < static_cast<int>(m_texts.size()))
    {
        const BoardText& item = m_texts[m_selectedText];
        const wxSize extent = memDC.GetTextExtent(item.text);

        memDC.SetPen(wxPen(
            wxColour(0, 120, 215),
            1,
            wxPENSTYLE_SHORT_DASH));
        memDC.SetBrush(*wxTRANSPARENT_BRUSH);
        memDC.DrawRectangle(
            item.pos.x - 3, item.pos.y - 3,
            extent.x + 6, extent.y + 6);
    }

    // 绘制选中框
    if (m_selectedGate != nullptr)
    {
        wxPoint c = m_selectedGate->center;
        double s = m_selectedGate->scale;

        memDC.SetPen(wxPen(
            wxColour(0, 120, 215),
            1,
            wxPENSTYLE_SHORT_DASH));

        memDC.SetBrush(*wxTRANSPARENT_BRUSH);

        wxRect selectionRect(
            c.x - static_cast<int>(45 * s),
            c.y - static_cast<int>(25 * s),
            static_cast<int>(110 * s),
            static_cast<int>(50 * s));

        memDC.DrawRectangle(selectionRect);
    }

    // 绘制临时连线
    if (m_wiring)
    {
        memDC.SetPen(wxPen(
            wxColour(255, 0, 0),
            2,
            wxPENSTYLE_SHORT_DASH));

        memDC.SetBrush(*wxTRANSPARENT_BRUSH);

        int midX =
            (m_wireStartPos.x + m_wireEndPos.x) / 2;

        wxPoint p1(midX, m_wireStartPos.y);
        wxPoint p2(midX, m_wireEndPos.y);

        memDC.DrawLine(m_wireStartPos, p1);
        memDC.DrawLine(p1, p2);
        memDC.DrawLine(p2, m_wireEndPos);
    }

    // 将内存画布绘制到窗口
    dc.Blit(
        0, 0,
        size.x, size.y,
        &memDC,
        0, 0);

    memDC.SelectObject(wxNullBitmap);
}

// =====================================================
// 绘制网格
// =====================================================
void DrawBoard::drawGrid(wxGraphicsContext* gc)
{
    if (!m_gridVisible)
    {
        return;
    }

    wxSize size = GetClientSize();
    const int gridSize = m_gridSize > 0 ? m_gridSize : 20;

    gc->SetPen(wxPen(
        wxColour(190, 192, 196),
        1,
        wxPENSTYLE_DOT));

    for (int x = 0; x < size.x; x += gridSize)
    {
        gc->StrokeLine(x, 0, x, size.y);
    }

    for (int y = 0; y < size.y; y += gridSize)
    {
        gc->StrokeLine(0, y, size.x, y);
    }

    // 每 5 格画一条稍深的线，方便对齐
    gc->SetPen(wxPen(wxColour(205, 207, 212), 1));

    for (int x = 0; x < size.x; x += gridSize * 5)
    {
        gc->StrokeLine(x, 0, x, size.y);
    }

    for (int y = 0; y < size.y; y += gridSize * 5)
    {
        gc->StrokeLine(0, y, size.x, y);
    }
}

// =====================================================
// 网格 / 吸附 / 新建 / 取消操作
// =====================================================
void DrawBoard::SetGridVisible(bool visible)
{
    m_gridVisible = visible;
    Refresh(false);
}

void DrawBoard::SetGridSize(int size)
{
    if (size < 2)
    {
        return;
    }

    m_gridSize = size;
    Refresh(false);
}

void DrawBoard::SetSnapToGrid(bool snap)
{
    m_snapToGrid = snap;
}

wxPoint DrawBoard::SnapPoint(const wxPoint& pos) const
{
    if (!m_snapToGrid || m_gridSize <= 0)
    {
        return pos;
    }

    const double g = static_cast<double>(m_gridSize);

    const int x = static_cast<int>(
        std::lround(pos.x / g) * g);
    const int y = static_cast<int>(
        std::lround(pos.y / g) * g);

    return wxPoint(x, y);
}

void DrawBoard::ClearAll()
{
    for (Gate* gate : Gates)
    {
        delete gate;
    }

    Gates.clear();
    m_wires.clear();
    m_texts.clear();

    m_selectedGate = nullptr;
    m_selectedText = -1;
    m_selectedWire = -1;
    m_draggingGate = false;
    m_wiring = false;
    m_wireStartComponentId = -1;
    m_nextGateId = 1;

    isDrawingAndGate = false;
    isDrawingOrGate = false;
    isDrawingNotGate = false;
    isDrawingNandGate = false;
    isDrawingXorGate = false;
    isDrawingNorGate = false;
    isDrawingXnorGate = false;

    SetCurrentTool(TOOL_SELECT);

    Refresh(false);
}

void DrawBoard::NotifyToolChanged(int reason)
{
    wxCommandEvent event(EVT_DRAWBOARD_TOOL_CHANGED);
    event.SetEventObject(this);
    event.SetInt(reason);
    wxPostEvent(GetParent(), event);
}

void DrawBoard::CancelCurrentAction()
{
    if (HasCapture())
    {
        ReleaseMouse();
    }

    m_draggingGate = false;
    m_wiring = false;
    m_wireDragging = false;
    m_wireStartComponentId = -1;

    isDrawingAndGate = false;
    isDrawingOrGate = false;
    isDrawingNotGate = false;
    isDrawingNandGate = false;
    isDrawingXorGate = false;
    isDrawingNorGate = false;
    isDrawingXnorGate = false;

    SetCurrentTool(TOOL_SELECT);
    Refresh(false);

    NotifyToolChanged(1);
}

// =====================================================
// 查找鼠标位置对应的逻辑门
// =====================================================
Gate* DrawBoard::FindGateAt(const wxPoint& pos)
{
    for (auto it = Gates.rbegin(); it != Gates.rend(); ++it)
    {
        Gate* gate = *it;

        if (gate == nullptr)
        {
            continue;
        }

        double s = gate->scale;

        int left = gate->center.x
            - static_cast<int>(45 * s);

        int right = gate->center.x
            + static_cast<int>(65 * s);

        int top = gate->center.y
            - static_cast<int>(25 * s);

        int bottom = gate->center.y
            + static_cast<int>(25 * s);

        if (pos.x >= left && pos.x <= right &&
            pos.y >= top && pos.y <= bottom)
        {
            return gate;
        }
    }

    return nullptr;
}

// =====================================================
// 鼠标移动：更新坐标、拖动元件和临时连线
// =====================================================
void DrawBoard::OnMouseMove(wxMouseEvent& event)
{
    m_mousePos = event.GetPosition();

    // 拖动元件
    if (m_draggingGate && m_selectedGate != nullptr)
    {
        if (event.LeftIsDown())
        {
            wxPoint target = m_mousePos - m_dragOffset;

            // 按住 Alt 时临时关闭吸附，便于微调
            if (!event.AltDown())
            {
                target = SnapPoint(target);
            }

            m_selectedGate->center = target;

            // 同步更新连线
            UpdateWirePositions();

            Refresh(false);
        }
        else
        {
            // 防止鼠标状态异常导致拖动一直持续
            m_draggingGate = false;

            if (HasCapture())
            {
                ReleaseMouse();
            }
        }
    }

    // 更新临时连线终点：靠近输入端口时吸附到端口，否则吸附到网格
    if (m_wiring)
    {
        Gate* hoverGate = nullptr;
        int hoverPort = -1;

        if (FindInputPortAt(m_mousePos, hoverGate, hoverPort))
        {
            m_wireEndPos = hoverGate->GetInputPort(hoverPort);
        }
        else
        {
            m_wireEndPos = SnapPoint(m_mousePos);
        }

        Refresh(false);
    }

    event.Skip();
}

// =====================================================
// 鼠标按下：放置元件、选择元件或创建连线
// =====================================================
void DrawBoard::OnMouseDown(wxMouseEvent& event)
{
    m_mousePos = event.GetPosition();
    m_selectedWire = -1;
    SetFocus();

    // -------------------------------------------------
    // 1. 放置逻辑门
    // -------------------------------------------------
    Gate* newGate = nullptr;

    // 放置位置吸附到网格（按住 Alt 可临时关闭）
    const wxPoint placePos =
        event.AltDown() ? m_mousePos : SnapPoint(m_mousePos);

    if (isDrawingAndGate)
    {
        newGate = new ANDGate(placePos);
    }
    else if (isDrawingOrGate)
    {
        newGate = new ORGate(placePos);
    }
    else if (isDrawingNotGate)
    {
        newGate = new NOTGate(placePos);
    }
    else if (isDrawingNandGate)
    {
        newGate = new NANDGate(placePos);
    }
    else if (isDrawingXorGate)
    {
        newGate = new XORGate(placePos);
    }
    else if (isDrawingNorGate)
    {
        newGate = new NORGate(placePos);
    }
    else if (isDrawingXnorGate)
    {
        newGate = new XNORGate(placePos);
    }

    if (newGate != nullptr)
    {
        newGate->id = m_nextGateId++;
        Gates.push_back(newGate);

        m_selectedGate = newGate;
        m_draggingGate = false;

        isDrawingAndGate = false;
        isDrawingOrGate = false;
        isDrawingNotGate = false;
        isDrawingNandGate = false;
        isDrawingXorGate = false;
        isDrawingNorGate = false;
        isDrawingXnorGate = false;

        m_wiring = false;
        m_wireStartComponentId = -1;

        SetCurrentTool(TOOL_SELECT);
        Refresh(false);
        NotifyToolChanged(0);
        return;
    }

    // -------------------------------------------------
    // 1.5 文字标注：点击后弹窗输入
    // -------------------------------------------------
    if (m_currentTool == TOOL_TEXT)
    {
        wxString value;

        if (PromptForText(value, wxEmptyString))
        {
            BoardText item;
            item.pos = placePos;
            item.text = value;

            m_texts.push_back(item);

            m_selectedText = static_cast<int>(m_texts.size()) - 1;
            m_selectedGate = nullptr;
        }

        SetCurrentTool(TOOL_SELECT);
        Refresh(false);
        NotifyToolChanged(0);
        return;
    }

    // -------------------------------------------------
    // 2. 连线模式
    // -------------------------------------------------
    if (m_currentTool == TOOL_WIRE)
    {
        m_draggingGate = false;

        // 第一次按下：从输出端口或空白处起笔
        if (!m_wiring)
        {
            StartWire(m_mousePos);
            return;
        }

        // 已经处于连线中：第二次点击完成落笔
        CompleteWire(m_mousePos);
        return;
    }

    // -------------------------------------------------
    // 3. 选择和拖动模式
    // -------------------------------------------------
    if (m_currentTool == TOOL_SELECT)
    {
        // 先判断是否点中文字标注
        const int textIndex = FindTextAt(m_mousePos);

        if (textIndex >= 0)
        {
            m_selectedText = textIndex;
            m_selectedGate = nullptr;
            m_draggingGate = false;

            Refresh(false);
            return;
        }

        m_selectedText = -1;

        Gate* clickedGate = FindGateAt(m_mousePos);

        if (clickedGate != nullptr)
        {
            // 点中元件：选中并准备拖动
            m_selectedGate = clickedGate;
            m_draggingGate = true;

            m_dragOffset =
                m_mousePos - m_selectedGate->center;

            if (!HasCapture())
            {
                CaptureMouse();
            }
        }
        else
        {
            // 点击空白处：取消选中
            m_selectedGate = nullptr;
            m_draggingGate = false;

            // 取消未完成的临时连线
            m_wiring = false;
            m_wireStartComponentId = -1;

            if (HasCapture())
            {
                ReleaseMouse();
            }
        }

        Refresh(false);
        return;
    }

    event.Skip();
}

// =====================================================
// 鼠标松开：结束元件拖动
// =====================================================
void DrawBoard::OnMouseUp(wxMouseEvent& event)
{
    m_mousePos = event.GetPosition();

    if (m_draggingGate)
    {
        m_draggingGate = false;

        UpdateWirePositions();
        Refresh(false);
    }
    // 按住拖动完成的连线
    if (m_wireDragging && m_wiring)
    {
        m_wireDragging = false;

        const int dx = event.GetPosition().x - m_wirePressPos.x;
        const int dy = event.GetPosition().y - m_wirePressPos.y;

        if (dx * dx + dy * dy > 36)
        {
            CompleteWire(event.GetPosition());
        }
        else if (HasCapture())
        {
            // 只是点了一下：保留连线状态，等第二次点击落笔
            ReleaseMouse();
        }
    }

    if (HasCapture())
    {
        ReleaseMouse();
    }

    event.Skip();
}

// =====================================================
// 文字标注：命中判断 / 删除 / 编辑
// =====================================================
int DrawBoard::FindTextAt(const wxPoint& pos) const
{
    wxClientDC dc(const_cast<DrawBoard*>(this));

    dc.SetFont(wxFont(
        11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL));

    for (int i = static_cast<int>(m_texts.size()) - 1; i >= 0; --i)
    {
        const wxSize extent = dc.GetTextExtent(m_texts[i].text);

        const wxRect rect(
            m_texts[i].pos.x - 3,
            m_texts[i].pos.y - 3,
            extent.x + 8,
            extent.y + 8);

        if (rect.Contains(pos))
        {
            return i;
        }
    }

    return -1;
}

// =====================================================
// 连线命中判断：检查鼠标是否落在折线的任一段附近
// =====================================================
int DrawBoard::FindWireAt(const wxPoint& pos) const
{
    const int hitTolerance = 6;

    auto between = [](int value, int first, int second, int tolerance)
    {
        const int low = std::min(first, second) - tolerance;
        const int high = std::max(first, second) + tolerance;
        return value >= low && value <= high;
    };

    for (int i = static_cast<int>(m_wires.size()) - 1; i >= 0; --i)
    {
        const Wire& wire = m_wires[i];
        const int midX = (wire.start.x + wire.end.x) / 2;

        const wxPoint points[] =
        {
            wire.start,
            wxPoint(midX, wire.start.y),
            wxPoint(midX, wire.end.y),
            wire.end
        };

        for (int segment = 0; segment < 3; ++segment)
        {
            const wxPoint& first = points[segment];
            const wxPoint& second = points[segment + 1];

            if (first.y == second.y &&
                between(pos.y, first.y, second.y, hitTolerance) &&
                between(pos.x, first.x, second.x, hitTolerance))
            {
                return i;
            }

            if (first.x == second.x &&
                between(pos.x, first.x, second.x, hitTolerance) &&
                between(pos.y, first.y, second.y, hitTolerance))
            {
                return i;
            }
        }
    }

    return -1;
}

void DrawBoard::DeleteSelectedText()
{
    if (m_selectedText < 0 ||
        m_selectedText >= static_cast<int>(m_texts.size()))
    {
        return;
    }

    m_texts.erase(m_texts.begin() + m_selectedText);
    m_selectedText = -1;

    Refresh(false);
}

void DrawBoard::EditText(int index)
{
    if (index < 0 ||
        index >= static_cast<int>(m_texts.size()))
    {
        return;
    }

    wxString value = m_texts[index].text;

    if (PromptForText(value, m_texts[index].text))
    {
        m_texts[index].text = value;
        Refresh(false);
    }
}

bool DrawBoard::PromptForText(
    wxString& value, const wxString& initial)
{
    wxTextEntryDialog dialog(
        this,
        wxT("请输入文字标注内容："),
        wxT("文字标注"),
        initial);

    if (dialog.ShowModal() != wxID_OK)
    {
        return false;
    }

    value = dialog.GetValue();

    if (value.Trim().Trim(false).IsEmpty())
    {
        return false;
    }

    return true;
}

// =====================================================
// 右键上下文菜单
// =====================================================
void DrawBoard::OnRightClick(wxMouseEvent& event)
{
    m_mousePos = event.GetPosition();
    SetFocus();

    const int textIndex = FindTextAt(m_mousePos);
    Gate* const gate = FindGateAt(m_mousePos);
    const int wireIndex = FindWireAt(m_mousePos);

    if (gate != nullptr)
    {
        m_selectedGate = gate;
        m_selectedText = -1;
        m_selectedWire = -1;
    }
    else if (textIndex >= 0)
    {
        m_selectedText = textIndex;
        m_selectedGate = nullptr;
        m_selectedWire = -1;
    }
    else if (wireIndex >= 0)
    {
        m_selectedWire = wireIndex;
        m_selectedGate = nullptr;
        m_selectedText = -1;
    }
    else
    {
        m_selectedWire = -1;
    }

    Refresh(false);

    wxMenu menu;

    if (gate != nullptr)
    {
        menu.Append(ID_EDIT_PROPERTIES, wxT("属性..."));
        menu.AppendSeparator();

        wxMenuItem* rotateItem =
            menu.Append(ID_DESIGN_ROTATE, wxT("顺时针旋转 90°"));
        rotateItem->Enable(false);

        wxMenuItem* mirrorItem =
            menu.Append(ID_DESIGN_MIRROR_H, wxT("水平镜像"));
        mirrorItem->Enable(false);

        menu.AppendSeparator();
        menu.Append(ID_EDIT_DELETE, wxT("删除元件"));
    }
    else if (textIndex >= 0)
    {
        menu.Append(ID_CTX_EDIT_TEXT, wxT("编辑文字..."));
        menu.Append(ID_CTX_DELETE_TEXT, wxT("删除文字"));
    }
    else if (wireIndex >= 0)
    {
        menu.Append(ID_CTX_DELETE_WIRE, wxT("删除连线"));
    }
    else
    {
        wxMenuItem* pasteItem =
            menu.Append(ID_EDIT_PASTE, wxT("粘贴"));
        pasteItem->Enable(false);

        wxMenuItem* selectAllItem =
            menu.Append(ID_EDIT_SELECTALL, wxT("全选"));
        selectAllItem->Enable(false);

        menu.AppendSeparator();

        wxMenuItem* gridItem =
            menu.AppendCheckItem(ID_VIEW_GRID, wxT("显示网格"));
        gridItem->Check(m_gridVisible);

        wxMenu* sizeMenu = new wxMenu();
        sizeMenu->AppendRadioItem(ID_VIEW_GRIDSIZE_10, wxT("10 px"));
        sizeMenu->AppendRadioItem(ID_VIEW_GRIDSIZE_20, wxT("20 px"));
        sizeMenu->AppendRadioItem(ID_VIEW_GRIDSIZE_50, wxT("50 px"));

        const int sizeId =
            m_gridSize == 10 ? ID_VIEW_GRIDSIZE_10
            : m_gridSize == 50 ? ID_VIEW_GRIDSIZE_50
            : ID_VIEW_GRIDSIZE_20;

        sizeMenu->Check(sizeId, true);
        menu.AppendSubMenu(sizeMenu, wxT("网格大小"));

        menu.AppendSeparator();

        wxMenuItem* snapItem =
            menu.AppendCheckItem(ID_VIEW_SNAP, wxT("对齐到网格"));
        snapItem->Check(m_snapToGrid);
    }

    const int command = GetPopupMenuSelectionFromUser(
        menu, event.GetPosition());

    if (command == wxID_NONE)
    {
        return;
    }

    switch (command)
    {
    case ID_CTX_EDIT_TEXT:
        EditText(m_selectedText);
        return;

    case ID_CTX_DELETE_TEXT:
        DeleteSelectedText();
        return;

    case ID_CTX_DELETE_WIRE:
        if (m_selectedWire >= 0 &&
            m_selectedWire < static_cast<int>(m_wires.size()))
        {
            m_wires.erase(m_wires.begin() + m_selectedWire);
        }
        m_selectedWire = -1;
        Refresh(false);
        return;

    case ID_EDIT_DELETE:
        DeleteSelectedGate();
        return;

    default:
        break;
    }

    // 其余命令（属性、网格开关等）交给主窗口统一处理
    wxCommandEvent commandEvent(wxEVT_MENU, command);
    commandEvent.SetEventObject(this);
    wxPostEvent(GetParent(), commandEvent);
}

// =====================================================
// 删除当前选中的逻辑门
// =====================================================
void DrawBoard::DeleteSelectedGate()
{
    if (m_selectedGate == nullptr)
    {
        return;
    }

    auto it = std::find(
        Gates.begin(),
        Gates.end(),
        m_selectedGate);

    if (it != Gates.end())
    {
        int deletedId = (*it)->id;

        // 删除与元件相连的连线
        DeleteWiresForGate(deletedId);

        delete* it;
        Gates.erase(it);
    }

    m_selectedGate = nullptr;
    m_draggingGate = false;

    if (HasCapture())
    {
        ReleaseMouse();
    }

    Refresh(false);
}

// =====================================================
// 根据 ID 查找逻辑门
// =====================================================
Gate* DrawBoard::FindGateById(int id)
{
    for (Gate* gate : Gates)
    {
        if (gate != nullptr && gate->id == id)
        {
            return gate;
        }
    }

    return nullptr;
}

// =====================================================
// 查找输入端口
// =====================================================
bool DrawBoard::FindInputPortAt(
    const wxPoint& pos,
    Gate*& gate,
    int& portIndex)
{
    const int hitRadius = 18;

    for (auto it = Gates.rbegin(); it != Gates.rend(); ++it)
    {
        Gate* current = *it;

        if (current == nullptr)
        {
            continue;
        }

        for (int i = 0; i < current->GetInputCount(); ++i)
        {
            wxPoint port = current->GetInputPort(i);

            int dx = pos.x - port.x;
            int dy = pos.y - port.y;

            if (dx * dx + dy * dy <=
                hitRadius * hitRadius)
            {
                gate = current;
                portIndex = i;
                return true;
            }
        }
    }

    gate = nullptr;
    portIndex = -1;
    return false;
}

// =====================================================
// 查找输出端口
// =====================================================
bool DrawBoard::FindOutputPortAt(
    const wxPoint& pos,
    Gate*& gate)
{
    const int hitRadius = 10;

    for (auto it = Gates.rbegin(); it != Gates.rend(); ++it)
    {
        Gate* current = *it;

        if (current == nullptr)
        {
            continue;
        }

        wxPoint port = current->GetOutputPort();

        int dx = pos.x - port.x;
        int dy = pos.y - port.y;

        if (dx * dx + dy * dy <=
            hitRadius * hitRadius)
        {
            gate = current;
            return true;
        }
    }

    gate = nullptr;
    return false;
}

// =====================================================
// 连线：起笔 / 落笔 / 取消
// 起笔既可以从输出端口开始，也可以从空白处开始（自由连线）
// =====================================================
void DrawBoard::StartWire(const wxPoint& pos)
{

    m_wirePressPos = pos;
    Gate* startGate = nullptr;

    if (FindOutputPortAt(pos, startGate))
    {
        m_wireStartComponentId = startGate->id;
        m_wireStartPos = startGate->GetOutputPort();
    }
    else
    {
        m_wireStartComponentId = -1;
        m_wireStartPos = SnapPoint(pos);
    }

    m_wiring = true;
    m_wireDragging = true;
    m_wireEndPos = m_wireStartPos;

    if (!HasCapture())
    {
        CaptureMouse();
    }

    Refresh(false);
}

void DrawBoard::CompleteWire(const wxPoint& pos)
{
    Gate* endGate = nullptr;
    int inputIndex = -1;

    int endComponentId = -1;
    wxPoint endPoint;

    if (FindInputPortAt(pos, endGate, inputIndex))
    {
        // 不允许把线连回自己
        if (m_wireStartComponentId != -1 &&
            endGate->id == m_wireStartComponentId)
        {
            CancelWire();
            return;
        }

        endComponentId = endGate->id;
        endPoint = endGate->GetInputPort(inputIndex);
    }
    else
    {
        // 落笔在空白处：生成一段两端悬空的自由连线
        endPoint = SnapPoint(pos);
    }

    // 长度太短视为误触，不生成连线
    const int dx = endPoint.x - m_wireStartPos.x;
    const int dy = endPoint.y - m_wireStartPos.y;

    if (dx * dx + dy * dy < 225)
    {
        CancelWire();
        return;
    }

    Wire wire;

    wire.startComponentId = m_wireStartComponentId;
    wire.endComponentId = endComponentId;
    wire.startPortIndex = -1;
    wire.endPortIndex = inputIndex;
    wire.start = m_wireStartPos;
    wire.end = endPoint;

    m_wires.push_back(wire);

    CancelWire();
}

void DrawBoard::CancelWire()
{
    m_wiring = false;
    m_wireDragging = false;
    m_wireStartComponentId = -1;
    m_wireEndPos = wxPoint(0, 0);

    if (HasCapture())
    {
        ReleaseMouse();
    }

    Refresh(false);
}

// =====================================================
// 更新连线位置：拖动元件时同步更新端点
// =====================================================
void DrawBoard::UpdateWirePositions()
{
    for (Wire& wire : m_wires)
    {
        // 更新连线起点
        if (wire.startComponentId != -1)
        {
            Gate* gate =
                FindGateById(wire.startComponentId);

            if (gate != nullptr)
            {
                wire.start = gate->GetOutputPort();
            }
        }

        // 更新连线终点
        if (wire.endComponentId != -1)
        {
            Gate* gate =
                FindGateById(wire.endComponentId);

            if (gate != nullptr &&
                wire.endPortIndex >= 0 &&
                wire.endPortIndex < gate->GetInputCount())
            {
                wire.end =
                    gate->GetInputPort(wire.endPortIndex);
            }
        }
    }
}

// =====================================================
// 删除连接到指定逻辑门的所有连线
// =====================================================
void DrawBoard::DeleteWiresForGate(int gateId)
{
    m_wires.erase(
        std::remove_if(
            m_wires.begin(),
            m_wires.end(),
            [gateId](const Wire& wire)
            {
                return wire.startComponentId == gateId ||
                    wire.endComponentId == gateId;
            }),
        m_wires.end());
}
