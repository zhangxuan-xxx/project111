
#pragma once

#include <wx/wx.h>
#include <wx/graphics.h>
#include <cmath>

// ============================================================
// 工具类型
// ============================================================
enum ToolType
{
    TOOL_SELECT,
    TOOL_WIRE
};

// ============================================================
// 连线类
// ============================================================
class Wire
{
public:
    wxPoint start;
    wxPoint end;

    int startComponentId;
    int endComponentId;

    int startPortIndex;
    int endPortIndex;

    bool isActive;

    Wire()
        : startComponentId(-1),
        endComponentId(-1),
        startPortIndex(-1),
        endPortIndex(-1),
        isActive(false)
    {
    }

    // 绘制直角折线：水平 -> 垂直 -> 水平
    void Draw(wxDC& dc) const
    {
        dc.SetPen(wxPen(
            isActive ? wxColour(255, 0, 0)
            : wxColour(0, 128, 0),
            2,
            wxPENSTYLE_SOLID
        ));

        int midX = (start.x + end.x) / 2;

        wxPoint p1(midX, start.y);
        wxPoint p2(midX, end.y);

        dc.DrawLine(start, p1);
        dc.DrawLine(p1, p2);
        dc.DrawLine(p2, end);
    }
};

// ============================================================
// 逻辑门类型
// ============================================================
enum GateType
{
    AND,
    OR,
    NOT,
    NAND_GATE,
    XOR_GATE

};

// ============================================================
// 元件基类
// ============================================================
class Component
{
public:
    int id = -1;

    wxPoint center;

    double scale = 1.0;

    bool scaling = false;
    bool moving = false;

    virtual ~Component() = default;
};

// ============================================================
// 逻辑门基类
// ============================================================
class Gate : public Component
{
public:
    GateType type = AND;

    wxPoint pout;

    explicit Gate(wxPoint center)
        : Component()
    {
        this->center = center;
    }

    virtual ~Gate() = default;

    // 当前逻辑门有两个输入端口
    virtual int GetInputCount() const
    {
        return 2;
    }

    // 输入端口坐标
    // 端口 0：上方输入引脚
    // 端口 1：下方输入引脚
    virtual wxPoint GetInputPort(int index) const
    {
        const int inputX =
            center.x - static_cast<int>(40 * scale);

        const int inputOffsetY =
            static_cast<int>(10 * scale);

        if (index == 0)
        {
            return wxPoint(
                inputX,
                center.y - inputOffsetY
            );
        }

        if (index == 1)
        {
            return wxPoint(
                inputX,
                center.y + inputOffsetY
            );
        }

        return center;
    }

    // 输出端口坐标
    virtual wxPoint GetOutputPort() const
    {
        return wxPoint(
            center.x + static_cast<int>(60 * scale),
            center.y
        );
    }

    // 子类负责绘制自身
    virtual void drawSelf(wxMemoryDC& memDC) const = 0;
};

// ============================================================
// AND 与门
// ============================================================
class ANDGate : public Gate
{
public:
    explicit ANDGate(wxPoint center)
        : Gate(center)
    {
        type = AND;
        scale = 1.0;
    }

    void drawSelf(wxMemoryDC& memDC) const override
    {
        wxGraphicsContext* gc =
            wxGraphicsContext::Create(memDC);

        if (gc == nullptr)
            return;

        gc->SetPen(wxPen(wxColour(0, 0, 0), 3));

        wxGraphicsPath path = gc->CreatePath();

        // 门体轮廓
        path.MoveToPoint(
            center.x + 20 * scale,
            center.y - 20 * scale
        );

        path.AddLineToPoint(
            center.x - 20 * scale,
            center.y - 20 * scale
        );

        path.AddLineToPoint(
            center.x - 20 * scale,
            center.y + 20 * scale
        );

        path.AddLineToPoint(
            center.x + 20 * scale,
            center.y + 20 * scale
        );

        path.AddArc(
            center.x + 20 * scale,
            center.y,
            20 * scale,
            static_cast<double>(M_PI / 2.0),
            static_cast<double>(-M_PI / 2.0),
            false
        );

        // 上方输入引脚
        path.MoveToPoint(
            center.x - 40 * scale,
            center.y - 10 * scale
        );

        path.AddLineToPoint(
            center.x - 20 * scale,
            center.y - 10 * scale
        );

        // 下方输入引脚
        path.MoveToPoint(
            center.x - 40 * scale,
            center.y + 10 * scale
        );

        path.AddLineToPoint(
            center.x - 20 * scale,
            center.y + 10 * scale
        );

        // 输出引脚
        path.MoveToPoint(
            center.x + 40 * scale,
            center.y
        );

        path.AddLineToPoint(
            center.x + 60 * scale,
            center.y
        );

        gc->StrokePath(path);

        delete gc;
    }
};

// ============================================================
// OR 或门
// ============================================================
class ORGate : public Gate
{
public:
    explicit ORGate(wxPoint center)
        : Gate(center)
    {
        type = OR;
        scale = 1.0;
    }

    void drawSelf(wxMemoryDC& memDC) const override
    {
        wxGraphicsContext* gc =
            wxGraphicsContext::Create(memDC);

        if (gc == nullptr)
            return;

        gc->SetPen(wxPen(wxColour(0, 0, 0), 3));

        wxGraphicsPath path = gc->CreatePath();

        // 门体轮廓
        path.MoveToPoint(
            center.x,
            center.y - 20 * scale
        );

        path.AddLineToPoint(
            center.x - 20 * scale,
            center.y - 20 * scale
        );

        path.AddQuadCurveToPoint(
            center.x,
            center.y,
            center.x - 20 * scale,
            center.y + 20 * scale
        );

        path.AddLineToPoint(
            center.x - 10 * scale,
            center.y + 20 * scale
        );

        path.AddQuadCurveToPoint(
            center.x + 20 * scale,
            center.y + 20 * scale,
            center.x + 40 * scale,
            center.y
        );

        path.AddQuadCurveToPoint(
            center.x + 20 * scale,
            center.y - 20 * scale,
            center.x,
            center.y - 20 * scale
        );

        // 上方输入引脚
        path.MoveToPoint(
            center.x - 40 * scale,
            center.y - 10 * scale
        );

        path.AddLineToPoint(
            center.x - 13 * scale,
            center.y - 10 * scale
        );

        // 下方输入引脚
        path.MoveToPoint(
            center.x - 40 * scale,
            center.y + 10 * scale
        );

        path.AddLineToPoint(
            center.x - 13 * scale,
            center.y + 10 * scale
        );

        // 输出引脚
        path.MoveToPoint(
            center.x + 40 * scale,
            center.y
        );

        path.AddLineToPoint(
            center.x + 60 * scale,
            center.y
        );

        gc->StrokePath(path);

        delete gc;
    }
};

// ============================================================
// NOT 非门
// ============================================================
class NOTGate : public Gate
{
public:
    explicit NOTGate(wxPoint center)
        : Gate(center)
    {
        type = NOT;
        scale = 1.0;
    }

    // 非门只有一个输入端
    int GetInputCount() const override
    {
        return 1;
    }

    // 输入端位于左侧
    wxPoint GetInputPort(int index) const override
    {
        if (index == 0)
        {
            return wxPoint(
                center.x - static_cast<int>(40 * scale),
                center.y
            );
        }

        return center;
    }

    // 输出端位于右侧
    wxPoint GetOutputPort() const override
    {
        return wxPoint(
            center.x + static_cast<int>(60 * scale),
            center.y
        );
    }

    // 绘制非门
    void drawSelf(wxMemoryDC& memDC) const override
    {
        wxGraphicsContext* gc =
            wxGraphicsContext::Create(memDC);

        if (gc == nullptr)
            return;

        gc->SetPen(wxPen(wxColour(0, 0, 0), 3));
        gc->SetBrush(*wxTRANSPARENT_BRUSH);

        // 绘制三角形主体
        wxGraphicsPath path = gc->CreatePath();

        path.MoveToPoint(
            center.x - 20 * scale,
            center.y - 20 * scale
        );

        path.AddLineToPoint(
            center.x - 20 * scale,
            center.y + 20 * scale
        );

        path.AddLineToPoint(
            center.x + 25 * scale,
            center.y
        );

        path.CloseSubpath();
        gc->StrokePath(path);

        // 绘制输入引脚
        gc->StrokeLine(
            center.x - 40 * scale,
            center.y,
            center.x - 20 * scale,
            center.y
        );

        delete gc;

        // 使用 wxDC 绘制反相小圆圈和输出引脚
        memDC.SetPen(wxPen(wxColour(0, 0, 0), 3));
        memDC.SetBrush(*wxTRANSPARENT_BRUSH);

        int circleX =
            center.x + static_cast<int>(25 * scale);

        int circleY =
            center.y - static_cast<int>(5 * scale);

        int circleSize =
            static_cast<int>(10 * scale);

        memDC.DrawEllipse(
            circleX,
            circleY,
            circleSize,
            circleSize
        );

        // 输出引脚
        memDC.DrawLine(
            center.x + static_cast<int>(35 * scale),
            center.y,
            center.x + static_cast<int>(60 * scale),
            center.y
        );
    }
};
// ============================================================
// NAND 与非门
// ============================================================
class NANDGate : public Gate
{
public:
    explicit NANDGate(wxPoint center)
        : Gate(center)
    {
        type = NAND_GATE;
        scale = 1.0;
    }

    void drawSelf(wxMemoryDC& memDC) const override
    {
        wxGraphicsContext* gc =
            wxGraphicsContext::Create(memDC);

        if (gc == nullptr)
            return;

        gc->SetPen(wxPen(wxColour(0, 0, 0), 3));
        gc->SetBrush(*wxTRANSPARENT_BRUSH);

        wxGraphicsPath path = gc->CreatePath();

        // 与门主体的左侧和上下边
        path.MoveToPoint(
            center.x - 20 * scale,
            center.y - 20 * scale
        );

        path.AddLineToPoint(
            center.x + 5 * scale,
            center.y - 20 * scale
        );

        // 右侧半圆轮廓
        path.AddArc(
            center.x + 5 * scale,
            center.y,
            20 * scale,
            static_cast<double>(M_PI / 2.0),
            static_cast<double>(-M_PI / 2.0),
            false
        );

        path.AddLineToPoint(
            center.x - 20 * scale,
            center.y + 20 * scale
        );

        path.CloseSubpath();

        // 输入引脚
        path.MoveToPoint(
            center.x - 40 * scale,
            center.y - 10 * scale
        );
        path.AddLineToPoint(
            center.x - 20 * scale,
            center.y - 10 * scale
        );

        path.MoveToPoint(
            center.x - 40 * scale,
            center.y + 10 * scale
        );
        path.AddLineToPoint(
            center.x - 20 * scale,
            center.y + 10 * scale
        );

        gc->StrokePath(path);
        delete gc;

        // 输出端反相小圆圈
        memDC.SetPen(wxPen(wxColour(0, 0, 0), 3));
        memDC.SetBrush(*wxTRANSPARENT_BRUSH);

        int circleX =
            center.x + static_cast<int>(25 * scale);

        int circleY =
            center.y - static_cast<int>(5 * scale);

        int circleSize =
            static_cast<int>(10 * scale);

        memDC.DrawEllipse(
            circleX,
            circleY,
            circleSize,
            circleSize
        );

        // 输出引脚
        memDC.DrawLine(
            center.x + static_cast<int>(35 * scale),
            center.y,
            center.x + static_cast<int>(60 * scale),
            center.y
        );
    }
};


// ============================================================
// XOR 异或门
// ============================================================
class XORGate : public Gate
{
public:
    explicit XORGate(wxPoint center)
        : Gate(center)
    {
        type = XOR_GATE;
        scale = 1.0;
    }

    void drawSelf(wxMemoryDC& memDC) const override
    {
        wxGraphicsContext* gc =
            wxGraphicsContext::Create(memDC);

        if (gc == nullptr)
            return;

        gc->SetPen(wxPen(wxColour(0, 0, 0), 3));
        gc->SetBrush(*wxTRANSPARENT_BRUSH);

        wxGraphicsPath path = gc->CreatePath();

        // 异或门主体轮廓
        path.MoveToPoint(
            center.x - 15 * scale,
            center.y - 20 * scale
        );

        path.AddQuadCurveToPoint(
            center.x + 15 * scale,
            center.y - 20 * scale,
            center.x + 40 * scale,
            center.y
        );

        path.AddQuadCurveToPoint(
            center.x + 15 * scale,
            center.y + 20 * scale,
            center.x - 15 * scale,
            center.y + 20 * scale
        );

        path.AddQuadCurveToPoint(
            center.x,
            center.y,
            center.x - 15 * scale,
            center.y - 20 * scale
        );

        // 异或门左侧额外弧线
        path.MoveToPoint(
            center.x - 25 * scale,
            center.y - 20 * scale
        );

        path.AddQuadCurveToPoint(
            center.x - 10 * scale,
            center.y,
            center.x - 25 * scale,
            center.y + 20 * scale
        );

        // 上方输入引脚
        path.MoveToPoint(
            center.x - 40 * scale,
            center.y - 10 * scale
        );

        path.AddLineToPoint(
            center.x - 20 * scale,
            center.y - 10 * scale
        );

        // 下方输入引脚
        path.MoveToPoint(
            center.x - 40 * scale,
            center.y + 10 * scale
        );

        path.AddLineToPoint(
            center.x - 20 * scale,
            center.y + 10 * scale
        );

        // 输出引脚
        path.MoveToPoint(
            center.x + 40 * scale,
            center.y
        );

        path.AddLineToPoint(
            center.x + 60 * scale,
            center.y
        );

        gc->StrokePath(path);
        delete gc;
    }
};
