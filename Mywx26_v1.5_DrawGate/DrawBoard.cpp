
#include "DrawBoard.h"

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
    Bind(wxEVT_MOUSE_CAPTURE_LOST,
        [this](wxMouseCaptureLostEvent& event)
        {
            m_draggingGate = false;
        });

    // Delete 键删除选中的元件
    Bind(wxEVT_CHAR_HOOK,
        [this](wxKeyEvent& event)
        {
            if (event.GetKeyCode() == WXK_DELETE)
            {
                DeleteSelectedGate();
            }
            else
            {
                event.Skip();
            }
        });

    st1 = new wxStaticText(
        this, wxID_ANY, wxT("x: 0"), wxPoint(10, 10));

    st2 = new wxStaticText(
        this, wxID_ANY, wxT("y: 0"), wxPoint(10, 30));

    m_mousePos = wxPoint(0, 0);
    m_wireStartPos = wxPoint(0, 0);
    m_wireEndPos = wxPoint(0, 0);

    m_wiring = false;
    m_draggingGate = false;
    m_selectedGate = nullptr;
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

    // 绘制已有连线
    for (const Wire& wire : m_wires)
    {
        wire.Draw(memDC);
    }

    // 绘制逻辑门
    for (const Gate* gate : Gates)
    {
        if (gate != nullptr)
        {
            gate->drawSelf(memDC);
        }
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
    wxSize size = GetClientSize();
    const int gridSize = 20;

    gc->SetPen(wxPen(
        wxColour(180, 180, 180),
        1,
        wxPENSTYLE_DOT_DASH));

    for (int x = 0; x < size.x; x += gridSize)
    {
        gc->StrokeLine(x, 0, x, size.y);
    }

    for (int y = 0; y < size.y; y += gridSize)
    {
        gc->StrokeLine(0, y, size.x, y);
    }
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

    if (st1 != nullptr)
    {
        st1->SetLabel(
            wxString::Format(wxT("x: %d"), m_mousePos.x));
    }

    if (st2 != nullptr)
    {
        st2->SetLabel(
            wxString::Format(wxT("y: %d"), m_mousePos.y));
    }

    // 拖动元件
    if (m_draggingGate && m_selectedGate != nullptr)
    {
        if (event.LeftIsDown())
        {
            m_selectedGate->center =
                m_mousePos - m_dragOffset;

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

    // 更新临时连线终点
    if (m_wiring)
    {
        m_wireEndPos = m_mousePos;
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
    SetFocus();

    // -------------------------------------------------
    // 1. 放置逻辑门
    // -------------------------------------------------
    Gate* newGate = nullptr;

    if (isDrawingAndGate)
    {
        newGate = new ANDGate(m_mousePos);
    }
    else if (isDrawingOrGate)
    {
        newGate = new ORGate(m_mousePos);
    }
    else if (isDrawingNotGate)
    {
        newGate = new NOTGate(m_mousePos);
    }
    else if (isDrawingNandGate)
    {
        newGate = new NANDGate(m_mousePos);
    }
    else if (isDrawingXorGate)
    {
        newGate = new XORGate(m_mousePos);
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

        m_wiring = false;
        m_wireStartComponentId = -1;

        SetCurrentTool(TOOL_SELECT);
        Refresh(false);
        return;
    }

    // -------------------------------------------------
    // 2. 连线模式
    // -------------------------------------------------
    if (m_currentTool == TOOL_WIRE)
    {
        m_draggingGate = false;

        // 第一次点击：选择输出端口
        if (!m_wiring)
        {
            Gate* startGate = nullptr;

            if (FindOutputPortAt(m_mousePos, startGate))
            {
                m_wiring = true;
                m_wireStartComponentId = startGate->id;
                m_wireStartPos = startGate->GetOutputPort();
                m_wireEndPos = m_wireStartPos;

                Refresh(false);
            }

            return;
        }

        // 第二次点击：选择输入端口
        Gate* endGate = nullptr;
        int inputIndex = -1;

        if (!FindInputPortAt(
            m_mousePos, endGate, inputIndex))
        {
            // 未点中输入端口，继续等待正确的输入端口
            Refresh(false);
            return;
        }

        // 不允许连接到同一个逻辑门
        if (endGate->id == m_wireStartComponentId)
        {
            return;
        }

        // 创建连线
        Wire wire;

        wire.startComponentId = m_wireStartComponentId;
        wire.endComponentId = endGate->id;
        wire.startPortIndex = -1;
        wire.endPortIndex = inputIndex;
        wire.start = m_wireStartPos;
        wire.end = endGate->GetInputPort(inputIndex);

        m_wires.push_back(wire);

        // 结束临时连线
        m_wiring = false;
        m_wireStartComponentId = -1;
        m_wireEndPos = wxPoint(0, 0);

        // 关键：连线完成后切换到选择模式
        SetCurrentTool(TOOL_SELECT);

        UpdateWirePositions();
        Refresh(false);
        return;
    }

    // -------------------------------------------------
    // 3. 选择和拖动模式
    // -------------------------------------------------
    if (m_currentTool == TOOL_SELECT)
    {
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

    if (HasCapture())
    {
        ReleaseMouse();
    }

    event.Skip();
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
