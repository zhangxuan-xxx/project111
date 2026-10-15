#include "DrawBoard.h"
#include <cmath>

DrawBoard::DrawBoard(wxFrame* parent) : wxPanel(parent) {

    SetBackgroundStyle(wxBG_STYLE_PAINT);
	Bind(wxEVT_PAINT, &DrawBoard::OnPaint, this);
    Bind(wxEVT_MOTION, &DrawBoard::OnMouseMove, this);
    Bind(wxEVT_LEFT_DOWN, &DrawBoard::OnMouseDown, this);
    Bind(wxEVT_LEFT_UP, &DrawBoard::OnMouseUp, this);

    st1 = new wxStaticText(this, -1, wxT(""), wxPoint(10, 10));
    st2 = new wxStaticText(this, -1, wxT(""), wxPoint(10, 30));
    m_wiring = false;
    SetCurrentTool(TOOL_SELECT);
}

DrawBoard::~DrawBoard(){
    for (Gate* gate : Gates) {
        delete gate;
    }
}


void DrawBoard::OnPaint(wxPaintEvent& event) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();

        wxBitmap bufferBitmap(GetClientSize());
        wxMemoryDC memDC;
        memDC.SelectObject(bufferBitmap);
        memDC.SetBackground(*wxWHITE_BRUSH);
        memDC.Clear();
        wxGraphicsContext* gc = wxGraphicsContext::Create(memDC);
        if (gc) {
            //gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
            //gc->SetInterpolationQuality(wxINTERPOLATION_DEFAULT);
            drawGrid(gc);
            gc->SetBrush(wxBrush(wxColour(0, 0, 0)));
            gc->SetPen(wxPen(wxColour(0, 0, 0), 1));
            gc->StrokeLine(m_mousePos.x, 0, m_mousePos.x, GetSize().y);
            gc->StrokeLine(0, m_mousePos.y, GetSize().x, m_mousePos.y);

            for (const auto& wire : m_wires) {
                wire.Draw(memDC);
            }
            if (isDrawingAndGate || isDrawingOrGate)

                for (const Gate* gate : Gates) {
                    gate->drawSelf(memDC);
                }

            if (m_wiring) {
                memDC.SetPen(wxPen(wxColour(255, 0, 0), 2, wxPENSTYLE_DOT));
                memDC.DrawLine(m_wireStartPos, m_wireEndPos);
            }


            delete gc;
        }

        dc.Blit(0, 0, GetClientSize().GetWidth(), GetClientSize().GetHeight(), &memDC, 0, 0);

}


void DrawBoard::drawGrid(wxGraphicsContext* gc) {

    wxSize size = GetSize();
    int gridSize = 20;

    gc->SetPen(wxPen(wxColour(180, 180, 180), 1, wxPENSTYLE_DOT_DASH));
    for (int x = 0; x < size.x; x += gridSize) {
        gc->StrokeLine(x, 0, x, size.y);
    }

    for (int y = 0; y < size.y; y += gridSize) {
        gc->StrokeLine(0, y, size.x, y);
    }
}

void DrawBoard::OnMouseMove(wxMouseEvent& event) {
    m_mousePos = event.GetPosition();
    st1->SetLabel(wxString::Format(wxT("x: %d"), event.GetX()));
    st2->SetLabel(wxString::Format(wxT("y: %d"), event.GetY()));
    if (m_wiring) {
        int dx = m_mousePos.x - m_wireStartPos.x;
        int dy = m_mousePos.y - m_wireStartPos.y;

        // 哪个方向偏移大，就吸附到哪个方向
        if (std::abs(dx) >= std::abs(dy)) {
            // 水平线：y 固定为起点 y
            m_wireEndPos = wxPoint(m_mousePos.x, m_wireStartPos.y);
        }
        else {
            // 竖直线：x 固定为起点 x
            m_wireEndPos = wxPoint(m_wireStartPos.x, m_mousePos.y);
        }
        Refresh();
    }
}

void DrawBoard::OnMouseDown(wxMouseEvent& event) {
    m_mousePos = event.GetPosition();

    switch (m_currentTool) {
    case TOOL_SELECT:
        break;

    case TOOL_WIRE: 
        if (!m_wiring) {
            m_wiring = true;
            m_wireEndPos = m_wireStartPos = m_mousePos;
            break;
        }
    }
    if (isDrawingAndGate) {
        Gates.push_back(new ANDGate(m_mousePos));

    }

    else if (isDrawingOrGate) {
        Gates.push_back(new ORGate(m_mousePos));

    }

  }

void DrawBoard::OnMouseUp(wxMouseEvent& event) {
    m_mousePos = event.GetPosition();

    if (m_wiring && m_currentTool == TOOL_WIRE) {
        int dx = m_mousePos.x - m_wireStartPos.x;
        int dy = m_mousePos.y - m_wireStartPos.y;

        if (std::abs(dx) >= std::abs(dy)) {
            m_wireEndPos = wxPoint(m_mousePos.x, m_wireStartPos.y);
        }
        else {
            m_wireEndPos = wxPoint(m_wireStartPos.x, m_mousePos.y);
        }

        Wire wire;
        wire.start = m_wireStartPos;
        wire.end = m_wireEndPos;
        m_wires.push_back(wire);
    }

    m_wiring = false;
    Refresh();
}
